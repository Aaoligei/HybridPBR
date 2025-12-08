#include "FileIO.h"
#include "Logger.h"
#include <filesystem>

namespace HybridPBR {
    
    std::string FileIO::ReadTextFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            LOG_ERROR("FileIO", "Failed to open file: " + filepath);
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), 
                           std::istreambuf_iterator<char>());
        return content;
    }
    
    std::vector<char> FileIO::ReadBinaryFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open binary file: " + filepath);
            return {};
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            LOG_ERROR("Failed to read binary file: " + filepath);
            return {};
        }
        
        return buffer;
    }
    
    bool FileIO::FileExists(const std::string& filepath) {
        return std::filesystem::exists(filepath);
    }
    
    std::string FileIO::GetAssetsPath() {
        return "../../assets/";
    }

} // namespace HybridPBR