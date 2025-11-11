#pragma once
#include <string>
#include <vector>
#include <fstream>

namespace HybridPBR {
    
    class FileIO {
    public:
        static std::string ReadTextFile(const std::string& filepath);
        static std::vector<char> ReadBinaryFile(const std::string& filepath);
        static bool FileExists(const std::string& filepath);
        static std::string GetAssetsPath();
    };

} // namespace HybridPBR