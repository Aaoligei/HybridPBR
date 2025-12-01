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
                   const std::vector<std::shared_ptr<Material>>& materials,
                    const std::vector<glm::mat4>& transforms) {
        //LOG_INFO("Building BVH acceleration structure...");
        
        nodes.clear();
        triangles.clear();
        gpuMaterials.clear();
        maxDepth = 0;
        sourceMaterials = materials; 
        
        // 从网格提取三角形
        ExtractTrianglesFromMeshes(meshes, materials,transforms);
        
        if (triangles.empty()) {
            LOG_WARNING("No triangles to build BVH");
            return false;
        }else {
            //LOG_INFO("Triangles extracted: " + std::to_string(triangles.size()));
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
            prim.pad0 = 0; // 初始化填充字段
            prims.push_back(prim);
        }
        
        nodes.reserve(triangles.size() * 2);

        // 构建根节点
        BuildRecursive(prims, 0, static_cast<int>(prims.size()),0);

        // ================== 新增核心修复代码 ==================
    
        // 2. 根据 prims 的最终顺序，重排三角形数组
        std::vector<Triangle> sortedTriangles;
        sortedTriangles.reserve(triangles.size());
        
        for (const auto& prim : prims) {
            // prim.triangleIndex 是该图元在原始 triangles 数组中的索引
            sortedTriangles.push_back(triangles[prim.triangleIndex]);
        }
        
        // 3. 用排序后的三角形替换原始三角形
        triangles = std::move(sortedTriangles);
            
            // LOG_INFO("BVH built: " + std::to_string(nodes.size()) + " nodes, " + 
            //         std::to_string(triangles.size()) + " triangles, max depth: " + 
            //         std::to_string(maxDepth));
            
            return true;
        }

    bool BVH::Intersect(const Ray& ray, float tMin, float tMax, HitRecord& rec) const {
        if (nodes.empty()) return false;
        return IntersectRecursive(0, ray, tMin, tMax, rec);
    }

    int BVH::BuildRecursive(std::vector<BuildPrimitive>& prims, int start, int end, int depth) {
        if (depth > maxDepth) maxDepth = depth;
        
        // 1. 记录当前节点在 vector 中的索引
        int nodeIndex = static_cast<int>(nodes.size());
        nodes.emplace_back(); // 添加节点，此时不要获取引用，因为后续递归会导致引用失效
        
        // 2. 初始化基本信息 (直接通过索引访问，虽然有点丑，但安全)
        nodes[nodeIndex].min = glm::vec3(FLT_MAX);
        nodes[nodeIndex].max = glm::vec3(-FLT_MAX);
        
        int primCount = end - start;
        if (primCount <= 2) {
            // --- 叶子节点逻辑 ---
            nodes[nodeIndex].leftChild = -1;
            nodes[nodeIndex].firstPrim = start;
            nodes[nodeIndex].primCount = primCount;
            
            // 计算叶子节点包围盒
            for (int i = start; i < end; ++i) {
                // 注意：这里要处理一下，因为glm::min/max可能有坑，建议手动展开或确保BuildPrimitive bounds正确
                const auto& pBound = prims[i].bounds;
                nodes[nodeIndex].min = glm::min(nodes[nodeIndex].min, pBound.min);
                nodes[nodeIndex].max = glm::max(nodes[nodeIndex].max, pBound.max);
            }
            return nodeIndex;
        }
        
        // --- 内部节点逻辑 ---
        
        // 分割图元 (注意：SplitNode 可能会改变 prims 的顺序)
        int splitAxis = SplitNode(prims, start, end);
        
        if (splitAxis == -1) {
            // 分割失败，强制转为叶子节点
            nodes[nodeIndex].leftChild = -1;
            nodes[nodeIndex].firstPrim = start;
            nodes[nodeIndex].primCount = primCount;
            
            for (int i = start; i < end; ++i) {
                const auto& pBound = prims[i].bounds;
                nodes[nodeIndex].min = glm::min(nodes[nodeIndex].min, pBound.min);
                nodes[nodeIndex].max = glm::max(nodes[nodeIndex].max, pBound.max);
            }
            return nodeIndex;
        }
        
        int mid = start + (end - start) / 2;
        
        // --- 递归构建子节点 ---
        // 关键：递归会向 nodes 添加元素，可能导致 vector 重新分配内存
        // 所以 nodeIndex 依然有效，但如果之前有 BVHNode& ref = nodes[nodeIndex] 则会失效
        
        int leftChildIndex = BuildRecursive(prims, start, mid, depth + 1);
        int rightChildIndex = BuildRecursive(prims, mid, end, depth + 1);
        
        // --- 递归返回后，更新当前节点 ---
        // 此时可以通过索引安全访问当前节点
        nodes[nodeIndex].leftChild = leftChildIndex;
        nodes[nodeIndex].firstPrim = rightChildIndex; // 在内部节点中，firstPrim 复用为 rightChild 索引
        nodes[nodeIndex].primCount = 0;
        
        // 利用子节点更新父节点包围盒
        nodes[nodeIndex].min = glm::min(nodes[leftChildIndex].min, nodes[rightChildIndex].min);
        nodes[nodeIndex].max = glm::max(nodes[leftChildIndex].max, nodes[rightChildIndex].max);
        
        return nodeIndex;
    }
    void BVH::CalculateNodeBounds(int nodeIndex, const std::vector<BuildPrimitive>& prims) {
        BVHNode& node = nodes[nodeIndex];
        node.min = glm::vec3(FLT_MAX);
        node.max = glm::vec3(-FLT_MAX);
        
        // 对于内部节点
        if (!node.IsLeaf() && node.leftChild != -1) {
            // 合并子节点的边界框
            if (node.leftChild < nodes.size() && node.firstPrim < nodes.size()) {
                node.min = glm::min(nodes[node.leftChild].min, nodes[node.firstPrim].min);
                node.max = glm::max(nodes[node.leftChild].max, nodes[node.firstPrim].max);
            }
        } 
        // 对于叶节点
        else if (node.IsLeaf()) {
            for (int i = node.firstPrim; i < node.firstPrim + node.primCount; ++i) {
                node.min = glm::min(node.min, prims[i].bounds.min);
                node.max = glm::max(node.max, prims[i].bounds.max);
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
        AABB bounds;
        bounds.min = node.min;
        bounds.max = node.max;
        if (!bounds.Intersect(ray, tMin, tMax)) {
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
                                        const std::vector<std::shared_ptr<Material>>& materials,
                                        const std::vector<glm::mat4>& transforms) {
        triangles.clear();
        
        for (int meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            const auto& mesh = meshes[meshIndex];
            if (!mesh) continue;

            // 获取当前网格对应的变换矩阵
            const glm::mat4& modelMatrix = transforms[meshIndex];
            // 计算法线矩阵 (模型矩阵逆的转置)，用于正确变换法线
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
            
            const auto& vertices = mesh->GetVertices();
            const auto& indices = mesh->GetIndices();
            
            // 定义一个 Lambda 来处理顶点变换
            auto TransformPoint = [&](const glm::vec3& localPos) {
                return glm::vec3(modelMatrix * glm::vec4(localPos, 1.0f));
            };
            
            auto TransformNormal = [&](const glm::vec3& localNormal) {
                return glm::normalize(normalMatrix * localNormal);
            };
            
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
                    
                    // === 应用变换 ===
                    tri.v0 = TransformPoint(vertices[i0].position);
                    tri.v1 = TransformPoint(vertices[i1].position);
                    tri.v2 = TransformPoint(vertices[i2].position);
                    
                    tri.n0 = TransformNormal(vertices[i0].normal);
                    tri.n1 = TransformNormal(vertices[i1].normal);
                    tri.n2 = TransformNormal(vertices[i2].normal);
                    
                    tri.uv0 = vertices[i0].texcoord;
                    tri.uv1 = vertices[i1].texcoord;
                    tri.uv2 = vertices[i2].texcoord;
                    
                    tri.materialIndex = materialIndex;
                    
                    // 初始化所有填充字段
                    tri.pad0 = tri.pad1 = tri.pad2 = 0.0f;
                    tri.pad3 = tri.pad4 = tri.pad5 = 0.0f;
                    tri.pad6 = tri.pad7 = tri.pad8 = glm::vec2(0.0f);
                    tri.pad9 = tri.pad10 = tri.pad11 = 0;
                    triangles.push_back(tri);
                }
            } else {
                // 非索引网格
                for (size_t i = 0; i < vertices.size(); i += 3) {
                    Triangle tri;
                    tri.v0 = TransformPoint(vertices[i].position);
                    tri.v1 = TransformPoint(vertices[i+1].position);
                    tri.v2 = TransformPoint(vertices[i+2].position);
                    
                    tri.n0 = TransformNormal(vertices[i].normal);
                    tri.n1 = TransformNormal(vertices[i+1].normal);
                    tri.n2 = TransformNormal(vertices[i+2].normal);
                    
                    tri.uv0 = vertices[i].texcoord;
                    tri.uv1 = vertices[i + 1].texcoord;
                    tri.uv2 = vertices[i + 2].texcoord;
                    
                    tri.materialIndex = materialIndex;
                    
                    // 初始化所有填充字段
                    tri.pad0 = tri.pad1 = tri.pad2 = 0.0f;
                    tri.pad3 = tri.pad4 = tri.pad5 = 0.0f;
                    tri.pad6 = tri.pad7 = tri.pad8 = glm::vec2(0.0f);
                    tri.pad9 = tri.pad10 = tri.pad11 = 0;
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
            
            // 设置纹理索引
            gpuMat.albedoTexture = (material->GetTexture(TextureType::DIFFUSE) != nullptr) ? 1 : 0;
            gpuMat.normalTexture = (material->GetTexture(TextureType::NORMAL) != nullptr) ? 1 : 0;
            gpuMat.metallicTexture = (material->GetTexture(TextureType::METALLIC) != nullptr) ? 1 : 0;
            gpuMat.roughnessTexture = (material->GetTexture(TextureType::ROUGHNESS) != nullptr) ? 1 : 0;
            gpuMat.aoTexture = (material->GetTexture(TextureType::AMBIENT_OCCLUSION) != nullptr) ? 1 : 0;
            gpuMat.emissiveTexture = (material->GetTexture(TextureType::EMISSIVE) != nullptr) ? 1 : 0;
            
            gpuMaterials.push_back(gpuMat);
        }
        
        // 如果没有材质，创建默认材质
        if (gpuMaterials.empty()) {
            gpuMaterials.push_back(GPUMaterial());
        }
    }

} // namespace HybridPBR