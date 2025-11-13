#include "Texture.h"
#include "utils/Logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace HybridPBR {

    Texture::Texture() {
        glGenTextures(1, &textureID);
    }

    Texture::~Texture() {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
        }
    }

    bool Texture::Create2D(int w, int h, GLenum internalFormat, 
                          GLenum format, GLenum dataType,
                          const void* data) {
        width = w;
        height = h;
        isCubemap = false;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
        
        // 设置默认参数
        SetWrapMode(TextureWrap::REPEAT, TextureWrap::REPEAT);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        return true;
    }

    bool Texture::LoadFromFile(const std::string& filepath, TextureType textureType) {
        this->filePath = filepath;
        type = textureType;
        isCubemap = false;
        
        int channels;
        void* data = nullptr;
        
        if (!LoadImageData(filepath, width, height, channels, &data)) {
            return false;
        }
        
        // 确定格式
        GLenum format = GL_RGB;
        GLenum internalFormat = GL_RGB8;
        
        if (channels == 1) {
            format = GL_RED;
            internalFormat = GL_R8;
        } else if (channels == 3) {
            format = GL_RGB;
            internalFormat = type == TextureType::DIFFUSE ? GL_SRGB8 : GL_RGB8;
        } else if (channels == 4) {
            format = GL_RGBA;
            internalFormat = type == TextureType::DIFFUSE ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        }
        
        bool success = Create2D(width, height, internalFormat, format, GL_UNSIGNED_BYTE, data);
        
        FreeImageData(data);
        return success;
    }

    bool Texture::LoadHDR(const std::string& filepath) {
        this->filePath = filepath;
        type = TextureType::HDR;
        isCubemap = false;
        
        stbi_set_flip_vertically_on_load(true);
        
        int channels;
        float* data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);
        
        if (!data) {
            LOG_ERROR("Failed to load HDR image: " + filepath);
            return false;
        }
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        
        SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        stbi_image_free(data);
        return true;
    }

    bool Texture::CreateCubemap(int size, GLenum internalFormat) {
        width = size;
        height = size;
        isCubemap = true;
        type = TextureType::CUBEMAP;
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, 
                        width, height, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        
        SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return true;
    }

    bool Texture::LoadCubemap(const std::vector<std::string>& faces) {
        if (faces.size() != 6) {
            LOG_ERROR("Cubemap requires exactly 6 faces");
            return false;
        }
        
        isCubemap = true;
        type = TextureType::CUBEMAP;
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        
        for (unsigned int i = 0; i < 6; ++i) {
            int w, h, channels;
            void* data = nullptr;
            
            if (!LoadImageData(faces[i], w, h, channels, &data, false)) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                return false;
            }
            
            GLenum format = GL_RGB;
            if (channels == 1) format = GL_RED;
            else if (channels == 3) format = GL_RGB;
            else if (channels == 4) format = GL_RGBA;
            
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
            
            FreeImageData(data);
            
            if (i == 0) {
                width = w;
                height = h;
            }
        }
        
        SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return true;
    }

    void Texture::SetWrapMode(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) {
        GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        
        glBindTexture(target, textureID);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapS));
        glTexParameteri(target, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapT));
        
        if (isCubemap) {
            glTexParameteri(target, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrapR));
        }
        
        glBindTexture(target, 0);
    }

    void Texture::SetFilter(TextureFilter minFilter, TextureFilter magFilter) {
        GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        
        glBindTexture(target, textureID);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
        glBindTexture(target, 0);
    }

    void Texture::GenerateMipmaps() {
        GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        
        glBindTexture(target, textureID);
        glGenerateMipmap(target);
        glBindTexture(target, 0);
    }

    void Texture::Bind(uint32_t unit) const {
        GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(target, textureID);
    }

    void Texture::Unbind() const {
        GLenum target = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        glBindTexture(target, 0);
    }

    GLenum Texture::GetGLInternalFormat(GLenum format, bool sRGB) {
        switch (format) {
            case GL_RED: return sRGB ? GL_SRGB8 : GL_R8;
            case GL_RG: return sRGB ? GL_SRGB8 : GL_RG8;
            case GL_RGB: return sRGB ? GL_SRGB8 : GL_RGB8;
            case GL_RGBA: return sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            case GL_DEPTH_COMPONENT: return GL_DEPTH_COMPONENT24;
            case GL_DEPTH_STENCIL: return GL_DEPTH24_STENCIL8;
            default: return sRGB ? GL_SRGB8 : GL_RGB8;
        }
    }

    GLenum Texture::GetGLFormat(int channels) {
        switch (channels) {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_RGB;
            case 4: return GL_RGBA;
            default: return GL_RGB;
        }
    }

    bool Texture::LoadImageData(const std::string& filepath, int& w, int& h, 
                               int& channels, void** data, bool flipY) {
        stbi_set_flip_vertically_on_load(flipY);
        
        *data = stbi_load(filepath.c_str(), &w, &h, &channels, 0);
        
        if (!*data) {
            LOG_ERROR("Failed to load image: " + filepath);
            return false;
        }
        
        return true;
    }

    void Texture::FreeImageData(void* data) {
        if (data) {
            stbi_image_free(data);
        }
    }

} // namespace HybridPBR