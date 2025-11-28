#include "BVH.h"
#include "utils/Logger.h"

namespace HybridPBR {

    BVH::BVH() {
    }

    BVH::~BVH() {
        nodes.clear();
        triangles.clear();
        gpuMaterials.clear();
    }

    bool BVH::Build(const std::vector<std::shared_ptr<Mesh>>& meshes,
                   const std::vector<std::shared_ptr<Material>>& materials) {
        LOG_INFO("Building BVH acceleration structure...");
        
        nodes.clear();
        triangles.clear();
        gpuMaterials.clear();
        maxDepth = 0;
        
        // 从网格提取三角形
        ExtractTrianglesFromMeshes(meshes, materials);
        
        if (triangles.empty()) {
            LOG_WARNING("No triangles to build BVH");
            return false;
        }else {
            LOG_INFO("Triangles extracted: " + std::to_string(triangles.size()));
        }
        
        // 创建GPU材质
        CreateGPUMaterials(materials);
        
        // 准备构建图元
        std::vector<BuildPrimitive> prims;
        prims.reserve(triangles.size());
        
        for (int i = 0; i < triangles.size(); ++i) {
            BuildPrimitive prim;
            prim.bounds = triangles[i].GetBounds();
            prim.center = prim.bounds.Center();
            prim.triangleIndex = i;
            prims.push_back(prim);
        }
        nodes.clear();
        nodes.reserve(triangles.size() * 2);
        nodes.resize(1);
        // 构建根节点
        BuildRecursive(prims, 0, static_cast<int>(prims.size()));
        
        LOG_INFO("BVH built: " + std::to_string(nodes.size()) + " nodes, " + 
                std::to_string(triangles.size()) + " triangles, max depth: " + 
                std::to_string(maxDepth));
        
        return true;
    }

    bool BVH::Intersect(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
        if (nodes.empty()) return false;
        return IntersectRecursive(0, ray, tMin, tMax, rec);
    }

    int BVH::BuildRecursive(std::vector<BuildPrimitive>& prims, int start, int end, int depth) {
        if (depth > maxDepth) maxDepth = depth;
        
        int nodeIndex = static_cast<int>(nodes.size());
        nodes.emplace_back();
        
        // 【修改点1】：这里不要长期持有引用的引用，或者只用于初始化基本数据
        // 此时 nodes[nodeIndex] 是安全的，但递归调用后不能保证其地址不变
        
        int primCount = end - start;
        if (primCount <= 2) {
            // 创建叶节点 - 这里不需要递归，引用是安全的，但为了统一建议用 nodes[nodeIndex]
            BVHNode& node = nodes[nodeIndex]; 
            node.leftChild = -1;
            node.firstPrim = start;
            node.primCount = primCount;
            
            CalculateNodeBounds(nodeIndex, prims);
            return nodeIndex;
        }
        
        // 分割节点
        int splitAxis = SplitNode(prims, start, end);
        if (splitAxis == -1) {
            // 分割失败，创建叶节点
            BVHNode& node = nodes[nodeIndex]; // 重新获取引用
            node.leftChild = -1;
            node.firstPrim = start;
            node.primCount = primCount;
            
            CalculateNodeBounds(nodeIndex, prims);
            return nodeIndex;
        }
        
        // 递归构建子节点
        int mid = start + (end - start) / 2;
        
        // 【核心修改点】：先获取子节点的索引
        int leftChildIndex = BuildRecursive(prims, start, mid, depth + 1);
        int rightChildIndex = BuildRecursive(prims, mid, end, depth + 1);
        
        // 【核心修改点】：递归返回后，vector 可能已经扩容，之前的引用失效。
        // 必须通过 nodeIndex 重新访问 nodes 数组
        nodes[nodeIndex].leftChild = leftChildIndex;
        nodes[nodeIndex].firstPrim = rightChildIndex; // 内部节点用 firstPrim 存右子节点索引
        nodes[nodeIndex].primCount = 0;
        
        // 计算边界
        CalculateNodeBounds(nodeIndex, prims);
        
        return nodeIndex;
    }

    void BVH::CalculateNodeBounds(int nodeIndex, const std::vector<BuildPrimitive>& prims) {
        BVHNode& node = nodes[nodeIndex];
        node.bounds = AABB();
        
        // 对于内部节点
        if (!node.IsLeaf() && node.leftChild != -1) {
            // 合并子节点的边界框
            if (node.leftChild < nodes.size() && node.firstPrim < nodes.size()) {
                node.bounds.min = glm::min(nodes[node.leftChild].bounds.min, nodes[node.firstPrim].bounds.min);
                node.bounds.max = glm::max(nodes[node.leftChild].bounds.max, nodes[node.firstPrim].bounds.max);
            }
        } 
        // 对于叶节点
        else if (node.IsLeaf()) {
            for (int i = node.firstPrim; i < node.firstPrim + node.primCount; ++i) {
                node.bounds.min = glm::min(node.bounds.min, prims[i].bounds.min);
                node.bounds.max = glm::max(node.bounds.max, prims[i].bounds.max);
            }
        }
    }

    int BVH::SplitNode(std::vector<BuildPrimitive>& prims, int start, int end) {
        // 计算整体边界
        AABB centroidBounds;
        for (int i = start; i < end; ++i) {
            centroidBounds.min = glm::min(centroidBounds.min, prims[i].center);
            centroidBounds.max = glm::max(centroidBounds.max, prims[i].center);
        }
        
        // 选择最长的轴进行分割
        glm::vec3 extent = centroidBounds.max - centroidBounds.min;
        int axis = 0;
        if (extent.y > extent.x) axis = 1;
        if (extent.z > extent[axis]) axis = 2;
        
        // 如果边界太小，不分割
        if (centroidBounds.max[axis] - centroidBounds.min[axis] < 0.001f) {
            return -1;
        }
        
        // 在中间点分割
        float splitPos = centroidBounds.min[axis] + extent[axis] * 0.5f;
        
        // 分割图元
        auto midIter = std::partition(prims.begin() + start, prims.begin() + end,
            [axis, splitPos](const BuildPrimitive& prim) {
                return prim.center[axis] < splitPos;
            });
        
        int mid = static_cast<int>(midIter - prims.begin());
        
        // 如果分割不均衡，调整
        if (mid == start || mid == end) {
            mid = start + (end - start) / 2;
        }
        
        return axis;
    }

    bool BVH::IntersectRecursive(int nodeIndex, const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
        const BVHNode& node = nodes[nodeIndex];
        
        // 检查节点边界
        if (!node.bounds.Intersect(ray, tMin, tMax)) {
            return false;
        }
        
        bool hit = false;
        
        if (node.IsLeaf()) {
            // 叶节点：检查所有三角形
            for (int i = 0; i < node.primCount; ++i) {
                int triIndex = node.firstPrim + i;
                if (triangles[triIndex].Intersect(ray, tMin, tMax, rec)) {
                    tMax = rec.t;
                    hit = true;
                }
            }
        } else {
            // 内部节点：递归检查子节点
            HitRecord leftRec, rightRec;
            bool hitLeft = IntersectRecursive(node.leftChild, ray, tMin, tMax, leftRec);
            bool hitRight = IntersectRecursive(node.firstPrim, ray, tMin, tMax, rightRec);
            
            if (hitLeft && hitRight) {
                rec = (leftRec.t < rightRec.t) ? leftRec : rightRec;
                hit = true;
            } else if (hitLeft) {
                rec = leftRec;
                hit = true;
            } else if (hitRight) {
                rec = rightRec;
                hit = true;
            }
        }
        
        return hit;
    }

    void BVH::ExtractTrianglesFromMeshes(const std::vector<std::shared_ptr<Mesh>>& meshes,
                                        const std::vector<std::shared_ptr<Material>>& materials) {
        triangles.clear();
        
        for (int meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            const auto& mesh = meshes[meshIndex];
            if (!mesh) continue;
            
            const auto& vertices = mesh->GetVertices();
            const auto& indices = mesh->GetIndices();
            
            // 假设每个网格使用第一个材质
            uint32_t materialIndex = 0;
            if (meshIndex < materials.size()) {
                materialIndex = meshIndex;
            }
            
            if (mesh->HasIndices()) {
                // 索引网格
                for (size_t i = 0; i < indices.size(); i += 3) {
                    Triangle tri;
                    uint32_t i0 = indices[i];
                    uint32_t i1 = indices[i + 1];
                    uint32_t i2 = indices[i + 2];
                    
                    tri.v0 = vertices[i0].position;
                    tri.v1 = vertices[i1].position;
                    tri.v2 = vertices[i2].position;
                    
                    tri.n0 = vertices[i0].normal;
                    tri.n1 = vertices[i1].normal;
                    tri.n2 = vertices[i2].normal;
                    
                    tri.uv0 = vertices[i0].texcoord;
                    tri.uv1 = vertices[i1].texcoord;
                    tri.uv2 = vertices[i2].texcoord;
                    
                    tri.materialIndex = materialIndex;
                    triangles.push_back(tri);
                }
            } else {
                // 非索引网格
                for (size_t i = 0; i < vertices.size(); i += 3) {
                    Triangle tri;
                    tri.v0 = vertices[i].position;
                    tri.v1 = vertices[i + 1].position;
                    tri.v2 = vertices[i + 2].position;
                    
                    tri.n0 = vertices[i].normal;
                    tri.n1 = vertices[i + 1].normal;
                    tri.n2 = vertices[i + 2].normal;
                    
                    tri.uv0 = vertices[i].texcoord;
                    tri.uv1 = vertices[i + 1].texcoord;
                    tri.uv2 = vertices[i + 2].texcoord;
                    
                    tri.materialIndex = materialIndex;
                    triangles.push_back(tri);
                }
            }
        }
    }

    void BVH::CreateGPUMaterials(const std::vector<std::shared_ptr<Material>>& materials) {
        gpuMaterials.clear();
        
        for (const auto& material : materials) {
            if (!material) continue;
            
            GPUMaterial gpuMat;
            const auto& props = material->GetProperties();
            
            gpuMat.albedo = props.albedo;
            gpuMat.emissive = glm::vec4(props.emissiveColor * props.emissiveIntensity, 1.0f);
            gpuMat.metallic = props.metallic;
            gpuMat.roughness = props.roughness;
            gpuMat.ao = props.ambientOcclusion;
            
            // 纹理索引（简化处理，实际需要纹理管理器）
            // 这里我们暂时设置为0，表示无纹理
            gpuMat.albedoTexture = 0;
            gpuMat.normalTexture = 0;
            gpuMat.metallicTexture = 0;
            gpuMat.roughnessTexture = 0;
            gpuMat.aoTexture = 0;
            gpuMat.emissiveTexture = 0;
            
            gpuMaterials.push_back(gpuMat);
        }
        
        // 如果没有材质，创建默认材质
        if (gpuMaterials.empty()) {
            gpuMaterials.push_back(GPUMaterial());
        }
    }

} // namespace HybridPBR