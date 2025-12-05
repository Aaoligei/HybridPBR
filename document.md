这是一个非常典型的光线追踪性能瓶颈问题。目前你的代码是一个“功能性”的实现，但在数据结构和内存访问模式上完全未优化。

**现状分析：**
1.  **BVH构建算法太弱：** `SplitNode` 使用的是“空间中点分割”（Spatial Median）。这种方法构建的树非常不平衡，导致很多光线需要遍历大量节点，性能极差。**这是 6FPS 的主要原因。**
2.  **内存布局缓存极不友好：** 你的 `Triangle` 结构体高达 144+ 字节（包含法线、UV、切线等）。在 GPU 进行求交测试时，只需要顶点位置（`v0, v1, v2`），但显卡不得不把整个 144 字节读入 L1 缓存。这意味着 80% 的带宽被浪费了。
3.  **Shader 寄存器压力：** `stack[32]` 这种局部数组在 Shader 中会导致寄存器溢出到显存（Local Memory），速度极慢。

以下是达到 60FPS 的**三步走终极优化指南**。

---

### 第一步：CPU端 - 实现 SAH (表面积启发式算法)

这是提升最显著的一步（通常能带来 3-5 倍提升）。不要在空间中心切分，而是基于“光线击中概率”切分。

**修改 `BVH.cpp` 中的 `SplitNode` 方法：**

使用 "Binning" (分桶) 技术来实现快速 SAH。

```cpp
// 在 BVH.cpp 中替换原有的 SplitNode
// 需要包含 <algorithm> 和 <iostream>

struct Bin {
    AABB bounds;
    int primCount = 0;
};

int BVH::SplitNode(std::vector<BuildPrimitive>& prims, int start, int end) {
    int count = end - start;
    if (count <= 1) return -1; // 只有一个图元，不分割

    // 1. 计算当前节点的整体包围盒
    AABB centroidBounds;
    for (int i = start; i < end; ++i) {
        centroidBounds.min = glm::min(centroidBounds.min, prims[i].center);
        centroidBounds.max = glm::max(centroidBounds.max, prims[i].center);
    }
    glm::vec3 extent = centroidBounds.max - centroidBounds.min;

    // 2. 只有当范围足够大时才分割
    if (glm::length(extent) < 0.0001f) return -1;

    // ================== SAH 核心代码开始 ==================
    const int BINS = 16; // 桶的数量，16 通常是最佳平衡点
    float bestCost = FLT_MAX;
    int bestAxis = -1;
    float bestSplitPos = 0.0f;

    // 对三个轴进行测试
    for (int axis = 0; axis < 3; ++axis) {
        if (extent[axis] < 0.00001f) continue;

        Bin bins[BINS];
        float scale = BINS / extent[axis];
        float minVal = centroidBounds.min[axis];

        // 2.1 将图元分配到桶中
        for (int i = start; i < end; ++i) {
            int binIdx = std::min(BINS - 1, static_cast<int>((prims[i].center[axis] - minVal) * scale));
            bins[binIdx].primCount++;
            bins[binIdx].bounds.min = glm::min(bins[binIdx].bounds.min, prims[i].bounds.min);
            bins[binIdx].bounds.max = glm::max(bins[binIdx].bounds.max, prims[i].bounds.max);
        }

        // 2.2 计算每个分割面的代价
        // 扫描线算法：从左到右累积，从右到左累积
        float leftArea[BINS - 1];
        float rightArea[BINS - 1];
        int leftCount[BINS - 1];
        int rightCount[BINS - 1];
        AABB leftBox, rightBox;
        int leftSum = 0, rightSum = 0;

        for (int i = 0; i < BINS - 1; ++i) {
            leftSum += bins[i].primCount;
            leftCount[i] = leftSum;
            leftBox.min = glm::min(leftBox.min, bins[i].bounds.min);
            leftBox.max = glm::max(leftBox.max, bins[i].bounds.max);
            leftArea[i] = leftBox.SurfaceArea();
        }

        for (int i = BINS - 1; i > 0; --i) {
            rightSum += bins[i].primCount;
            rightCount[i - 1] = rightSum;
            rightBox.min = glm::min(rightBox.min, bins[i].bounds.min);
            rightBox.max = glm::max(rightBox.max, bins[i].bounds.max);
            rightArea[i - 1] = rightBox.SurfaceArea();
        }

        // 2.3 寻找最小代价
        // Cost = (LeftCount * LeftArea + RightCount * RightArea)
        for (int i = 0; i < BINS - 1; ++i) {
            float cost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                // 分割位置在第 i 个桶的右边界
                bestSplitPos = minVal + (i + 1) * (extent[axis] / BINS); 
            }
        }
    }
    
    // 2.4 如果分割代价比不分割还高（或者没找到有效分割），则停止
    // 父节点包围盒面积 * 图元数量 (近似计算)
    // 这里简单处理：如果 bestAxis 依然是 -1，或者 bestCost 极大，说明无法有效分割
    if (bestAxis == -1) return -1;

    // 3. 执行分割 (std::partition)
    auto midIter = std::partition(prims.begin() + start, prims.begin() + end,
        [bestAxis, bestSplitPos](const BuildPrimitive& prim) {
            return prim.center[bestAxis] < bestSplitPos;
        });

    int mid = static_cast<int>(midIter - prims.begin());

    // 防止甚至 SAH 也导致的一侧为空的情况
    if (mid == start || mid == end) {
        mid = start + (end - start) / 2;
    }

    return mid; // 注意：此时不再返回 axis，而是返回分割点索引，你需要修改 BuildRecursive
}
```

**修改 `BuildRecursive` 以适配 SAH 逻辑：**
目前的 `BuildRecursive` 假设 `SplitNode` 返回轴。我们需要稍微修改它。由于上面的 SAH 已经在内部做了 partition，`SplitNode` 不需要返回轴，只需要确保数组被正确排序并告诉我们中间点在哪里。

*修改建议：* 将你的 `SplitNode` 改名为 `PartitionNode`，返回分割点的索引 `int mid`。如果无法分割返回 -1。

---

### 第二步：数据结构重构 - 拆分几何与属性 (Structure of Arrays)

这是显存带宽优化的关键。

**1. 修改 C++ 端 `BVH.h` / `RayTracer.cpp`**

你需要将 `Triangle` 拆分为两个 buffer：
1.  **CompactTriangle**: 仅用于求交，越小越好。
2.  **TriangleAttribute**: 用于着色，只有击中后才读取。

```cpp
// 在 BVH.h 中新增
struct CompactTriangle {
    glm::vec3 v0; float pad0;
    glm::vec3 v1; float pad1;
    glm::vec3 v2; float pad2;
    uint32_t materialIndex; // 材质ID
    uint32_t attrIndex;     // 属性索引（通常等于三角形索引）
    uint32_t pad3, pad4;
};

struct TriangleAttribute {
    glm::vec3 n0, n1, n2;
    glm::vec3 t0, t1, t2;
    glm::vec2 uv0, uv1, uv2;
    // ... 其他属性
};
```

**2. 在 Shader 端 (`path_tracing.comp`) 拆分 Buffer**

修改你的 Buffer 定义，不再使用那个巨大的 `Triangle` 结构。

```glsl
// 只包含求交需要的数据
struct CompactTriangle {
    vec3 v0; float p0;
    vec3 v1; float p1;
    vec3 v2; float p2;
    uint matID;
    uint attrID; 
    uint p3, p4;
};

// 只有在确定击中最近点后，才去查这个表
struct TriangleAttr {
    vec3 n0; float pn0;
    vec3 n1; float pn1;
    vec3 n2; float pn2;
    vec3 t0; float pt0;
    vec3 t1; float pt1;
    vec3 t2; float pt2;
    vec2 uv0; vec2 puv0;
    vec2 uv1; vec2 puv1;
    vec2 uv2; vec2 puv2;
    // ...
};

layout(std430, binding = 5) buffer GeoBuffer { CompactTriangle triangles[]; };
layout(std430, binding = 7) buffer AttrBuffer { TriangleAttr attributes[]; }; // 新增绑定点
```

**修改 Shader 逻辑：**
在 `BVHIntersect` 和 `TracePath` 中：
*   遍历时只读取 `CompactTriangle`。
*   `TracePath` 得到 `hitInfo` 后，不要立即计算 `rec.normal` 等。
*   等到遍历结束，确定了这个是 `ClosestHit`，再根据 `hitInfo.triIndex` 去 `AttrBuffer` 读取法线和UV。

这会大幅减少遍历过程中的显存带宽压力。

---

### 第三步：Shader 遍历循环优化

优化 `path_tracing.comp` 中的遍历逻辑，减少寄存器使用。

1.  **压缩 BVH Node**
    目前的 Node 是 `vec4 min, vec4 max`。虽然对齐了，但浪费了空间。
    可以改为两个 `vec4`：
    *   `Data1`: `vec3 min`, `float leftChildIndex` (int转换float)
    *   `Data2`: `vec3 max`, `float primitiveCount` (如果是0则是内部节点，存右孩子索引)

2.  **移除 `stack[32]` (Short Stack 优化)**
    虽然完全消除 stack 很难，但我们可以通过位掩码（Bit Trail）来实现无栈遍历（但这太复杂）。
    目前的优化方案是：**减少 Stack 大小**。对于 10-20 万面片，BVH深度通常在 16-20 层左右。`stack[32]` 是安全的，但可以尝试缩小到 `stack[24]` 看看能否增加 GPU Occupancy。

3.  **提取属性的延迟计算 (关键代码)**

修改 `TracePath` 函数，将属性插值移出循环：

```glsl
// 在循环中只记录索引和重心坐标
struct HitInfo {
    float t;
    vec2 bary; // u, v
    int triIndex;
};

// ... BVHIntersect 只填充 HitInfo ...

vec3 TracePath(...) {
    // ...
    HitInfo info;
    bool hit = BVHIntersect(ray, ..., info);
    
    if (hit) {
        // --- 只有在这里才读取庞大的属性 Buffer ---
        TriangleAttr attr = attributes[info.triIndex];
        
        // 执行插值
        float w = 1.0 - info.bary.x - info.bary.y;
        vec3 normal = normalize(attr.n0 * w + attr.n1 * info.bary.x + attr.n2 * info.bary.y);
        vec2 uv = attr.uv0 * w + attr.uv1 * info.bary.x + attr.uv2 * info.bary.y;
        
        // ... 接下来才是着色计算 ...
    }
}
```

---

### 总结：你的行动清单

1.  **立即执行**：用 **SAH** 重写 `BVH.cpp` 的构建逻辑。这是算法层面的碾压，不换硬件就能质变。
2.  **立即执行**：在 Shader 中，把 `Triangle` 拆分成 `CompactTriangle` (顶点位置) 和 `TriangleAttribute` (法线/UV)。遍历时只查前者，击中后查后者。
3.  **检查**：确保 Shader 中的 `lightCount` 循环逻辑正确。你在 Shader 里使用了随机采样（`CalculateDirectLightStochastic`），这很好。但是 `TracePath` 里的 `samplesPerPixel` 如果是 1，那就没必要在 Shader 里写个循环，直接算一次即可。

做完这三步，你的帧率应该能从 6 FPS 提升到 30-50 FPS 甚至更高。如果还需要进一步提升，就需要引入 **Wavefront Path Tracing** (波前路径追踪) 架构了，但这需要重写整个渲染管线。