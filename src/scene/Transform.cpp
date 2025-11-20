#include "Transform.h"

namespace HybridPBR {

    Transform::Transform() {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        parent = nullptr;
        dirty = true;
        worldMatrix = glm::mat4(1.0f);
    }

    void Transform::SetPosition(const glm::vec3& newPosition) {
        position = newPosition;
        MarkDirty();
    }

    void Transform::SetRotation(const glm::vec3& newRotation) {
        rotation = newRotation;
        MarkDirty();
    }

    void Transform::SetScale(const glm::vec3& scale) {
        // 防止零缩放导致法线矩阵奇异
        this->scale = glm::vec3(
            std::max(scale.x, 0.0001f),
            std::max(scale.y, 0.0001f),
            std::max(scale.z, 0.0001f)
        );
        MarkDirty();
    }

    void Transform::Translate(const glm::vec3& translation) {
        position += translation;
        MarkDirty();
    }

    void Transform::Rotate(const glm::vec3& rotationDelta) {
        rotation += rotationDelta;
        MarkDirty();
    }

    void Transform::Scale(const glm::vec3& scaleDelta) {
        scale *= scaleDelta;
        MarkDirty();
    }

    glm::mat4 Transform::GetWorldMatrix() const {
        if (dirty) {
            worldMatrix = GetLocalMatrix();
            
            if (parent) {
                // 如果有父对象，将父对象的世界矩阵与本地矩阵相乘
                worldMatrix = parent->GetWorldMatrix() * worldMatrix;
            }
            
            dirty = false;
        }
        
        return worldMatrix;
    }

    glm::mat4 Transform::GetLocalMatrix() const {
        return MathUtils::CreateTransformMatrix(position, rotation, scale);
    }

    glm::vec3 Transform::GetForward() const {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, MathUtils::ToRadians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, MathUtils::ToRadians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        return glm::normalize(rotationMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    }

    glm::vec3 Transform::GetRight() const {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, MathUtils::ToRadians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        return glm::normalize(rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    }

    glm::vec3 Transform::GetUp() const {
        return glm::cross(GetRight(), GetForward());
    }

    void Transform::SetParent(Transform* newParent) {
        parent = newParent;
        MarkDirty();
    }

    void Transform::MarkDirty() {
        dirty = true;
    }

} // namespace HybridPBR