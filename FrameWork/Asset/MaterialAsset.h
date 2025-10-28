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
    std::filesystem::file_time_type fileTime; //加载时间
    std::string shaderPath;

    template<class T>
    using ParamMap = std::map<std::string, T>;
    //Uniform
    ParamMap<float> floats{};
    ParamMap<uint32_t> uints{};
    ParamMap<int> ints{};
    ParamMap<glm::vec2> vec2s{};
    ParamMap<glm::vec3> vec3s{};
    ParamMap<glm::vec4> vec4s{};
    ParamMap<glm::mat4> mat4s{};
    ParamMap<std::string> textures{}; //对应其路径 //相当于依赖项

    bool dirty {false}; //记录是否被运行时修改，决定是否写回
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MaterialAsset, name, contentHash, fileTime, shaderPath, floats, uints, ints, vec2s,
    vec3s, vec4s, mat4s,textures
    )

struct MaterialSource {
    std::string name;
    std::string shaderPath;

    template<class T>
    using ParamMap = std::map<std::string, T>;
    //Uniform
    ParamMap<float> floats{};
    ParamMap<uint32_t> uints{};
    ParamMap<int> ints{};
    ParamMap<glm::vec2> vec2s{};
    ParamMap<glm::vec3> vec3s{};
    ParamMap<glm::vec4> vec4s{};
    ParamMap<glm::mat4> mat4s{};
    ParamMap<std::string> textures{}; //对应其路径 //相当于依赖项
};

//使用Default 可以不需要完全Material中的Param参数，Like："vec2" : {}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MaterialSource, name, shaderPath, floats, uints, ints, vec2s,
    vec3s, vec4s, mat4s, textures
    )



#endif //CAIENGINE_MATERIAL_H
