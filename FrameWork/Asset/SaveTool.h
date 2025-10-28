//
// Created by 51092 on 2025/10/26.
//

#ifndef CAIENGINE_SAVETOOL_H
#define CAIENGINE_SAVETOOL_H

#include <ostream>
#include <type_traits>
#include <ranges>
#include <cstdint>
#include<fstream>


namespace AssetHelper {
    inline void SaveString(std::ofstream& file, const std::string& str) {
        uint32_t len = str.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(str.data()), sizeof(char) * len);
    }

    inline void LoadString(std::ifstream& file, std::string& str) {
        uint32_t len = 0;
        file.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
        str.resize(len);
        file.read(reinterpret_cast<char*>(str.data()), sizeof(char) * len);
    }
}



#endif //CAIENGINE_SAVETOOL_H