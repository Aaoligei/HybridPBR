#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../utils/MathUtils.h"

namespace HybridPBR {

    class Transform {
    public:
        Transform();
        
        // 变换操作
        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);
        void SetScale(const glm::vec3& scale);
        
        void Translate(const glm::vec3& translation);
        void Rotate(const glm::vec3& rotation);
        void Scale(const glm::vec3& scale);
        
        // 获取变换
        const glm::vec3& GetPosition() const { return position; }
        const glm::vec3& GetRotation() const { return rotation; }
        const glm::vec3& GetScale() const { return scale; }
        
        // 矩阵计算
        glm::mat4 GetWorldMatrix() const;
        glm::mat4 GetLocalMatrix() const;
        
        // 方向向量
        glm::vec3 GetForward() const;
        glm::vec3 GetRight() const;
        glm::vec3 GetUp() const;
        
        // 父子关系
        void SetParent(Transform* parent);
        Transform* GetParent() const { return parent; }

    private:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f); // 欧拉角 (度)
        glm::vec3 scale = glm::vec3(1.0f);
        
        Transform* parent = nullptr;
        mutable bool dirty = true;
        mutable glm::mat4 worldMatrix = glm::mat4(1.0f);
        
        void MarkDirty();
    };

} // namespace HybridPBR