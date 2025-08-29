//
// Created by 51092 on 25-8-28.
//

#include "ShaderParse.h"

#include "Logger.h"

FrameWork::ShaderInfo FrameWork::ShaderParse::GetShaderInfo(const std::string &code) {
    ShaderInfo info;
    info.shaderState = GetShaderStateSet(code);

    std::string vertCode, fragCode;
    ParseShaderCode(code, vertCode, fragCode);
    info.vertProperties = GetShaderProperties(vertCode);
    info.fragProperties = GetShaderProperties(fragCode);
    info.shaderTypeFlags = ShaderType::Vertex | ShaderType::Frag;
    SetUpPropertiesStd140(info);
    return info;//
}

void FrameWork::ShaderParse::ParseShaderCode(const std::string &code, std::string &vert, std::string &frag) {
    vert = GetCodeBlock(code, "Vertex");
    frag = GetCodeBlock(code, "Fragment");
}

FrameWork::ShaderPropertiesInfo FrameWork::ShaderParse::GetShaderProperties(const std::string &code) {
    ShaderPropertiesInfo propertiesInfo{};
    auto propertiesBlock = GetCodeBlock(code, "Properties");
    if (propertiesBlock.empty()) {
        WARNING("the property is empty!");
        return propertiesInfo;
    }
    auto lines = SplitString(propertiesBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);

        if (words.size() == 0) {
            continue;
        }else if (words[0] == "//") {
            //注释
            continue;
        }else {
            auto iter = shaderPropertyMap.find(words[0]);
            if (iter == shaderPropertyMap.end()) {
                ERROR("Invalid shader property type of {}", words[0]);
            }else {
                auto type = iter->second;
                ShaderProperty shaderProperty = {};
                GetPropertyNameAndArrayLength(words[1], shaderProperty.name, shaderProperty.arrayLength);
                shaderProperty.type = type;
                if (IsBaseProperty(type)) {
                    propertiesInfo.baseProperties.push_back(shaderProperty);
                }else {
                    propertiesInfo.textureProperties.push_back(shaderProperty);
                }
            }
        }
    }
    return propertiesInfo;
}

FrameWork::ShaderStateSet FrameWork::ShaderParse::GetShaderStateSet(const std::string &code) {
    ShaderStateSet shaderStateSet{};
    std::string settingBlock = GetCodeBlock(code, "Settings");
    if (settingBlock.empty()) {
        return shaderStateSet;//传回默认设置
    }
    auto lines = SplitString(settingBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (words.size() == 0) {
            continue;
        }else if (words[0] == "//") {
            continue;
        }else if (words[0] == "Blend") {
            if (blendFactorMap.find(words[1]) == blendFactorMap.end() ||
                blendFactorMap.find(words[2]) == blendFactorMap.end()) {
                WARNING("The blend factor is wrong, use default state set");
                return shaderStateSet;
            }else {
                shaderStateSet.srcBlendFactor = blendFactorMap[words[1]];
                shaderStateSet.dstBlendFactor = blendFactorMap[words[2]];
            }
        }else if (words[0] == "BlendOp") {
            if (blendOptionMap.find(words[1]) == blendOptionMap.end()) {
                WARNING("The blend option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.blendOp = blendOptionMap[words[1]];
        }else if (words[0] == "Cull") {
            if (faceCullOptionMap.find(words[1]) == faceCullOptionMap.end()) {
                WARNING("The cull option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.faceCullOp = faceCullOptionMap[words[1]];
        }else if (words[0] == "ZTest") {
            if (depthTestOptionMap.find(words[1]) == depthTestOptionMap.end()) {
                WARNING("The ztest option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.depthCompareOp = depthTestOptionMap[words[1]];
        }else if (words[0] == "ZWrite") {
            if (words[1] != "Off" && words[1] != "On") {
                WARNING("The zwrite option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.depthWrite = words[1] == "On";
        }else {
            WARNING("Can't find suitable operation for {}", words[0]);
        }
    }
    return shaderStateSet;
}

std::string FrameWork::ShaderParse::GetCodeBlock(const std::string &code, const std::string &blockName) {
    auto begin = code.find(blockName);
    if (begin == std::string::npos)
        return "";

    //保证开头无有效字符，保证结尾无有效字符
    if ((begin > 0 && isValidChar(code[begin - 1])) ||
        (begin + blockName.size() < code.size() && isValidChar(code[begin + blockName.size()]))) {
        return "";
    }
    int level = 0; //表示括号层级
    uint32_t s = 0;
    uint32_t e = 0;
    for (size_t i = begin; i < code.size(); ++i) {
        if (code[i] == '{') {
            level++;
            if (level == 1) {
                s = i;
            }
        }else if (code[i] == '}') {
            level--;
            if (level == 0) {
                e = i;
                break;
            }
        }
    }
    return code.substr(s + 1, e - s - 1); //不要前后花括号
}

void FrameWork::ShaderParse::SetUpPropertiesStd140(ShaderInfo &shaderInfo) {
    uint32_t binding = 0;//这里的思路是binding用来绑定单帧之内的所有资源，所以每帧使用一个set

    if (!shaderInfo.vertProperties.baseProperties.empty()) {
        uint32_t offset = 0;
        for (auto& property : shaderInfo.vertProperties.baseProperties) {
            auto alignInfo = GetPropertyAlignInfoStd140(property.type, property.arrayLength);
            property.binding = binding;
            property.offset = offset;
            property.align = alignInfo.alignment;
            property.arrayOffset = alignInfo.arrayOffset;
            uint32_t padding = offset % alignInfo.alignment;
            if (padding == 0) {
                property.offset = offset;
            }else {
                property.offset = offset - padding + alignInfo.alignment;
            }
            offset += property.offset + property.size;
        }
        binding++;
    }
    for (auto& property : shaderInfo.vertProperties.textureProperties)
    {
        property.binding = binding; //这边一个texture绑定在一个binding //我的代码可以以set为单位，这个没区别
        binding++;
    }

    if (!shaderInfo.fragProperties.baseProperties.empty()) {
        uint32_t offset = 0;
        for (auto& property : shaderInfo.fragProperties.baseProperties) {
            auto alignInfo = GetPropertyAlignInfoStd140(property.type, property.arrayLength);
            property.binding = binding;
            property.offset = offset;
            property.align = alignInfo.alignment;
            property.arrayOffset = alignInfo.arrayOffset;
            uint32_t padding = offset % alignInfo.alignment;
            if (padding == 0) {
                property.offset = offset;
            }else {
                property.offset = offset - padding + alignInfo.alignment; //offset + align  然后按align取整
            }
            offset += property.offset + property.size;
        }
        binding++;
    }
    for (auto& property : shaderInfo.fragProperties.textureProperties)
    {
        property.binding = binding; //这边一个texture绑定在一个binding //我的代码可以以set为单位，这个没区别
        binding++;
    }

}

FrameWork::PropertyAlignInfo FrameWork::ShaderParse::GetPropertyAlignInfoStd140(ShaderPropertyType type,
    uint32_t arrayLength) {
    uint32_t std_size = sizeof(float);
    if (type == ShaderPropertyType::BOOL || type == ShaderPropertyType::INT || type == ShaderPropertyType::UINT
        || type == ShaderPropertyType::FLOAT) {
        if (arrayLength == 0) {
           return {.size = std_size, .alignment = std_size };
        }else {
            return { .size = (std_size * 4) * (arrayLength - 1) + std_size,
                .alignment = std_size * 4, .arrayOffset = std_size * 4 };//最后一个元素无需padding
        }
    }else if (type == ShaderPropertyType::VEC2) {
        if (arrayLength == 0) {
            return {.size = std_size * 2, .alignment = std_size * 2};
        }
        else
            return { .size = (std_size * 4) * (arrayLength - 1) + std_size * 2,
                .alignment = std_size * 4, .arrayOffset = std_size * 4 };
    }else if (type == ShaderPropertyType::VEC3) {
        if (arrayLength == 0)
            return { .size = std_size * 3, .alignment = std_size * 4 };
        else
            return { .size = (std_size * 4) * (arrayLength - 1) + std_size * 3,
                .alignment = std_size * 4, .arrayOffset = std_size * 4 };

    }else if (type == ShaderPropertyType::VEC4) {
        if (arrayLength == 0)
            return { .size = std_size * 4, .alignment = std_size * 4 };
        else
            return { .size = (std_size * 4) * arrayLength, .alignment = std_size * 4,
                .arrayOffset = std_size * 4 };

    }else if (type == ShaderPropertyType::MAT2) {
        if (arrayLength == 0)
            return { .size = std_size * (4 + 2), .alignment = std_size * 4 };
        else
            return { .size = std_size * ((arrayLength * 2 - 1) * 4 + 2),
                .alignment = std_size * 4, .arrayOffset = std_size * 4 * 2 };

    }else if (type == ShaderPropertyType::MAT3) {
        if (arrayLength == 0)
            return { .size = std_size * (4 * 2 + 3),
                .alignment = std_size * 4 };
        else
            return { .size = std_size * ((arrayLength * 3 - 1) * 4 + 3),
                .alignment = std_size * 4, .arrayOffset = std_size * 4 * 3 };

    }else if (type == ShaderPropertyType::MAT4) {
        if (arrayLength == 0)
            return { .size = std_size * 16,
                .alignment = std_size * 4 };
        else
            return { .size = std_size * ((arrayLength * 4) * 4),
                .alignment = std_size * 4, .arrayOffset = std_size * 4 * 4 };
    }else {
        ERROR("Invalid shader property type!");
        return {};
    }
}


bool FrameWork::ShaderParse::isValidChar(const char &c) {
    return isalnum(c) || c == '_';
}

size_t FrameWork::ShaderParse::FindWord(const std::string &code, const std::string &word, size_t offset) {
    auto pos = code.find(word, offset);
    auto strSize = code.size();
    auto wordSize = word.size();
    while (pos != std::string::npos) {
        //前后没字符
        if ((pos == 0 || (pos > 0 && !isValidChar(code[pos - 1]))) && ((pos + wordSize == strSize) || !isValidChar(code[pos + wordSize]))) {
            return pos;
        }
        offset = pos + wordSize;
        pos = code.find(word, offset);
    }
    return std::string::npos;
}

void FrameWork::ShaderParse::RemoveCRLF(std::string &code) {
    code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
    code.erase(std::remove(code.begin(), code.end(), '\r'), code.end());
    code.erase(std::remove(code.begin(), code.end(), '\t'), code.end());
}

void FrameWork::ShaderParse::RemoveSpaces(std::string &code) {
    code.erase(std::remove(code.begin(), code.end(), ' '), code.end());
}

std::vector<std::string> FrameWork::ShaderParse::SplitString(const std::string &str, char p) {
    //这里唯一的区别是没有空字符串
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token,p)) {
       tokens.push_back(token);
    }
    tokens.erase(std::remove(tokens.begin(), tokens.end(), ""), tokens.end());
    return tokens;
}

std::vector<std::string> FrameWork::ShaderParse::ExtractWords(const std::string &str) {
    std::vector<std::string> words;

    size_t s = 0, e = 0;
    bool record = false;//是否在记录
    for (size_t i = 0; i < str.size(); i++)
    {
        if (record && (str[i] == ' '))
        {
            words.push_back(str.substr(s, i - s));//不包括括号
            record = false;
        }
        else if (record && (i == str.size() - 1))
        {
            words.push_back(str.substr(s, i - s + 1));//已经到结尾
            record = false;
        }
        else if (!record && str[i] != ' ')
        {
            s = i;
            record = true;//是char开始记录
        }
    }

    for (auto& word : words)
        RemoveCRLF(word);

    return words;
}

void FrameWork::ShaderParse::GetPropertyNameAndArrayLength(const std::string &propertyStr, std::string &name,
    uint32_t &arrayLength) {
    size_t s = 0, e =0;
    arrayLength = 0;
    for (size_t i = 0; i < propertyStr.size(); ++i) {
        if (propertyStr[i] == '[') {
            s = i;
        }else if (propertyStr[i] == ']') {
            e = i;
        }
    }
    if (s == 0 || e == 0) {
        name = propertyStr;
        return;
    }
    name = propertyStr.substr(0, s);
    std::string lenStr = propertyStr.substr(s + 1, e - s - 1);

    arrayLength = std::stoi(lenStr);
}

bool FrameWork::ShaderParse::IsBaseProperty(ShaderPropertyType type) {
    return !(type == ShaderPropertyType::SAMPLER || type == ShaderPropertyType::SAMPLER_2D || type == ShaderPropertyType::SAMPLER_CUBE);
}

std::string FrameWork::ShaderParse::TranslateToVulkan(const std::string &code) {

}
