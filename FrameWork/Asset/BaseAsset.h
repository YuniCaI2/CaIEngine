//
// Created by 51092 on 2025/11/2.
//

#ifndef CAIENGINE_BASEASSET_H
#define CAIENGINE_BASEASSET_H
#include <nlohmann/json.hpp>

//基类
struct BaseAsset {
    std::string name{};
    std::string sourcePath{};
    std::string contentHash{};
    std::filesystem::file_time_type fileTime{};
    bool dirt {false};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    BaseAsset, name, sourcePath, contentHash, fileTime
    )


#define SERIALIZE_ASSET(AssetType, ...) \
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AssetType, name, sourcePath, contentHash, fileTime, ##__VA_ARGS__)


#endif //CAIENGINE_BASEASSET_H