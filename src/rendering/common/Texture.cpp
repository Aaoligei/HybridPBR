#include "Texture.h"
#include "utils/Logger.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace HybridPBR {
    /* ---------- 工具：把 TextureWrap / TextureFilter 转成 GLenum ---------- */
    static GLenum ToGL(TextureWrap w) {
        switch (w) {
            case TextureWrap::REPEAT:            return GL_REPEAT;
            case TextureWrap::CLAMP_TO_EDGE:     return GL_CLAMP_TO_EDGE;
            case TextureWrap::CLAMP_TO_BORDER:   return GL_CLAMP_TO_BORDER;
            case TextureWrap::MIRRORED_REPEAT:   return GL_MIRRORED_REPEAT;
        }
        return GL_REPEAT;
    }
    static GLenum ToGL(TextureFilter f) {
        switch (f) {
            case TextureFilter::NEAREST: return GL_NEAREST;
            case TextureFilter::LINEAR:  return GL_LINEAR;
            case TextureFilter::NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
            case TextureFilter::LINEAR_MIPMAP_NEAREST:  return GL_LINEAR_MIPMAP_NEAREST;
            case TextureFilter::NEAREST_MIPMAP_LINEAR:  return GL_NEAREST_MIPMAP_LINEAR;
            case TextureFilter::LINEAR_MIPMAP_LINEAR:   return GL_LINEAR_MIPMAP_LINEAR;
        }
        return GL_LINEAR;
    }
    Texture::Texture() {}

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
        
        /* 1. 创建 + 一次性分配存储 */
        if (textureID) glDeleteTextures(1, &textureID);
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
        glTextureStorage2D(textureID, 1, internalFormat, w, h);

        /* 2. 上传数据（如有） */
        if (data)
            glTextureSubImage2D(textureID, 0, 0, 0, w, h, format, dataType, data);
        
        // 设置默认参数
        SetWrapMode(TextureWrap::REPEAT, TextureWrap::REPEAT);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
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
        
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
        glTextureStorage2D(textureID, 1, GL_RGB16F, width, height);
        glTextureSubImage2D(textureID, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);
        
        SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        
        stbi_image_free(data);
        return true;
    }

    bool Texture::CreateCubemap(int size, GLenum internalFormat) {
        width = size;
        height = size;
        isCubemap = true;
        type = TextureType::CUBEMAP;
        
        if (textureID) glDeleteTextures(1, &textureID);
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);
        glTextureStorage2D(textureID, 1, internalFormat, size, size); // 6 面一起分配
        
        SetWrapMode(TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE, TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
       
        return true;
    }

    bool Texture::LoadCubemap(const std::vector<std::string>& faces) {
        if (faces.size() != 6) { LOG_ERROR("Cubemap needs 6 faces"); return false; }
        isCubemap = true;
        type = TextureType::CUBEMAP;

        if (textureID) glDeleteTextures(1, &textureID);
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);

        int w = 0, h = 0;
        for (int i = 0; i < 6; ++i) {
            int channels;
            stbi_set_flip_vertically_on_load(false);
            unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &channels, 0);
            if (!data) { LOG_ERROR("Failed cubemap face: " + faces[i]); return false; }

            GLenum format = GetGLFormat(channels);
            if (i == 0) glTextureStorage2D(textureID, 1, GL_RGBA8, w, h); // 只需一次
            glTextureSubImage3D(textureID, 0, 0, 0, i, w, h, 1, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        width = w; height = h;

        SetWrapMode(TextureWrap::CLAMP_TO_EDGE,
                    TextureWrap::CLAMP_TO_EDGE,
                    TextureWrap::CLAMP_TO_EDGE);
        SetFilter(TextureFilter::LINEAR, TextureFilter::LINEAR);
        return true;
    }

    void Texture::SetWrapMode(TextureWrap wrapS, TextureWrap wrapT, TextureWrap wrapR) {
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, ToGL(wrapS));
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, ToGL(wrapT));
        if (isCubemap)
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, ToGL(wrapR));
        
    }

    void Texture::SetFilter(TextureFilter minFilter, TextureFilter magFilter) {
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, ToGL(minFilter));
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, ToGL(magFilter));
    }

    void Texture::GenerateMipmaps() {
        glGenerateTextureMipmap(textureID);
    }

    void Texture::Bind(uint32_t unit) const {
        glBindTextureUnit(unit, textureID);
    }

    void Texture::Unbind() const {
        glBindTextureUnit(0, 0);
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