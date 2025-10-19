//
// Created by 51092 on 2025/10/16.
//

#ifndef CAIENGINE_MATERIAL_H
#define CAIENGINE_MATERIAL_H
#include<map>
#include<string>
#include<glm/glm.hpp>
#include<filesystem>
#include<nlohmann/json.hpp>

struct MaterialAsset {
    std::string name;
    std::string contentHash;
    std::filesystem::file_time_type fileTime; //shader加载时间
    std::string shaderPath;

    template<class T>
    using ParamMap = std::map<std::string, T>;
    //Uniform
    ParamMap<float> floatParams{};
    ParamMap<uint32_t> uintParams{};
    ParamMap<int> intParams{};
    ParamMap<glm::vec2> vec2Params{};
    ParamMap<glm::vec3> vec3Params{};
    ParamMap<glm::vec4> vec4Params{};
    ParamMap<std::string> textures; //对应其路径
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    MaterialAsset, name, contentHash, fileTime,shaderPath, floatParams, uintParams, intParams, vec2Params,
    vec3Params, vec4Params, textures
    )


#endif //CAIENGINE_MATERIAL_H
