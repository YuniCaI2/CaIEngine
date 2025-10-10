//
// Created by 51092 on 25-8-28.
//

#include "ShaderParse.h"

#include "Logger.h"

FrameWork::ShaderInfo FrameWork::ShaderParse::GetShaderInfo(const std::string &code) {
    ShaderInfo info;
    info.shaderState = GetShaderStateSet(code);
    info.shaderFormatsInfo = GetShaderFormatsInfo(code);
    std::string vertCode, fragCode;
    ParseShaderCode(code, vertCode, fragCode);
    info.vertProperties = GetShaderProperties(vertCode);
    info.fragProperties = GetShaderProperties(fragCode);
    if (! vertCode.empty()) {
        info.shaderTypeFlags |= ShaderType::Vertex;
    }
    if (! fragCode.empty()) {
        info.shaderTypeFlags |= ShaderType::Frag;
    }
    SetUpPropertiesStd140(info);
    return info;
}

void FrameWork::ShaderParse::ParseShaderCode(const std::string &code, std::string &vert, std::string &frag) {
    vert = GetCodeBlock(code, "Vertex");
    frag = GetCodeBlock(code, "Fragment");
}

FrameWork::ShaderPropertiesInfo FrameWork::ShaderParse::GetShaderProperties(const std::string &code) {
    ShaderPropertiesInfo propertiesInfo{};
    auto propertiesBlock = GetCodeBlock(code, "Properties");
    if (propertiesBlock.empty()) {
        LOG_WARNING("the property is empty!");
        return propertiesInfo;
    }
    auto lines = SplitString(propertiesBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);

        if (words.empty()) {
            continue;
        }else if (words[0] == "//") {
            //注释
            continue;
        }else {
            auto iter = shaderPropertyMap.find(words[0]);
            if (iter == shaderPropertyMap.end()) {
                LOG_ERROR("Invalid shader property type of {}", words[0]);
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
        LOG_WARNING("The settings block is empty!, Make Sure Settings not Setting");
        return shaderStateSet;//传回默认设置
    }
    auto lines = SplitString(settingBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (words.empty()) {
            continue;
        }else if (words[0] == "//") {
            continue;
        }else if (words[0] == "Blend") {
            if (blendFactorMap.find(words[1]) == blendFactorMap.end() ||
                blendFactorMap.find(words[2]) == blendFactorMap.end()) {
                LOG_WARNING("The blend factor is wrong, use default state set");
                return shaderStateSet;
            }else {
                shaderStateSet.srcBlendFactor = blendFactorMap[words[1]];
                shaderStateSet.dstBlendFactor = blendFactorMap[words[2]];
            }
        }else if (words[0] == "BlendOp") {
            if (blendOptionMap.find(words[1]) == blendOptionMap.end()) {
                LOG_WARNING("The blend option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.blendOp = blendOptionMap[words[1]];
        }else if (words[0] == "Cull") {
            if (faceCullOptionMap.find(words[1]) == faceCullOptionMap.end()) {
                LOG_WARNING("The cull option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.faceCullOp = faceCullOptionMap[words[1]];
        }else if (words[0] == "ZTest") {
            if (depthTestOptionMap.find(words[1]) == depthTestOptionMap.end()) {
                LOG_WARNING("The ztest option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.depthCompareOp = depthTestOptionMap[words[1]];
        }else if (words[0] == "ZWrite") {
            if (words[1] != "Off" && words[1] != "On") {
                LOG_WARNING("The zwrite option is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.depthWrite = words[1] == "On";
        }else if (words[0] == "PolygonMode") {
            if (words[1] != "Line" && words[1] != "Fill") {
                LOG_WARNING("The polygon mode is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.polygonMode = polygonModeMap[words[1]];
        }else if (words[0] == "InputVertex") {
            if (words[1] != "Off" && words[1] != "On") {
                LOG_WARNING("The InputVertex is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.inputVertex = words[1] == "On";
            //  是否输入顶点
        }else if (words[0] == "MSAA") {
            if (words[1] != "Off" && words[1] != "On") {
                LOG_WARNING("The MSAA is wrong, use default state set");
                return shaderStateSet;
            }
            shaderStateSet.msaa = words[1] == "On";
        }
        else {
            LOG_WARNING("Can't find suitable operation for {}", words[0]);
        }
    }
    //上面是Setting，但是我希望这个结构体可以存储更多信息，有些信息并不是手动填入的比如输出附件的数量
    std::string vertCode, fragCode;
    ParseShaderCode(code, vertCode, fragCode);
    std::string outputBlock = GetCodeBlock(fragCode, "Output");
    lines = SplitString(outputBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (words.size() >= 3 && words[0] != "//") {
            shaderStateSet.outputNums++;
        }
    }

    return shaderStateSet;
}

FrameWork::CompLocalInvocation FrameWork::ShaderParse::GetCompLocalInvocation(const std::string &code) {
    CompLocalInvocation invocation = {};
    auto block = GetCodeBlock(code, "Settings");
    auto lines = SplitString(block, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (words.size() >= 2 && words[0] != "//") {
            if (words[0] == "localX") {
                invocation.x = std::stoi(words[1]);
            }else if (words[0] == "localY") {
                invocation.y = std::stoi(words[1]);
            }else if (words[0] == "localZ") {
                invocation.z = std::stoi(words[1]);
            }else {
                LOG_WARNING("Can't find suitable operation for {}", words[0]);
            }
        }else {
            LOG_ERROR("the settings format is wrong: {}", line);
            return {};
        }
    }
    return invocation;
}

std::vector<FrameWork::SSBO> FrameWork::ShaderParse::GetSSBOs(const std::string &code) {
    std::vector<SSBO> ssbos;
    auto block = GetCodeBlock(code, "SSBO");
    auto lines = SplitString(block, '\n');
    //再得到声明来验证
    auto callingBlock = GetCodeBlock(code, "Calling");

    for (auto& line : lines) {
        auto words = ExtractWords(line);
        SSBO ssbo;
        if (words.size() >= 3 && words[0] != "//") {
            if (ssboOpMap.contains(words[0])) {
                ssbo.ssboOP = ssboOpMap[words[0]];
            }else {
                LOG_ERROR("Can't find suitable operation for {}", words[0]);
                return {};
            }


            if (storageObjectTypeMap.contains(words[1])) {
                ssbo.type = storageObjectTypeMap[words[1]];
                if (words.size() < 4 || !storageImageFormatMap.contains(words[2])) {
                    LOG_ERROR("Can't find suitable storage image format");
                }
                ssbo.storageImageFormat = storageImageFormatMap[words[2]];
                ssbo.name = words[3];

            }else {
                if (callingBlock.find(words[1]) != std::string::npos) {
                    ssbo.structName = words[1];
                    ssbo.type = StorageObjectType::Buffer;
                    ssbo.name = words[2];
                }else {
                    LOG_ERROR("Can't find suitable type for {}", words[1]);
                }
            }
        }
        ssbos.push_back(ssbo);
    }
    return ssbos;
}

void FrameWork::ShaderParse::SetUpCompSSBOAndProperty(CompShaderInfo &info) {
    uint32_t binding = 0;

    //Properties:
    if (!info.shaderProperties.baseProperties.empty()) {
        uint32_t offset = 0;
        for (auto& property : info.shaderProperties.baseProperties) {
            auto alignInfo = GetPropertyAlignInfoStd140(property.type, property.arrayLength);
            property.size = alignInfo.size;
            property.binding = binding;
            property.align = alignInfo.alignment;
            property.arrayOffset = alignInfo.arrayOffset;

            // 正确的对齐计算
            property.offset = (offset + alignInfo.alignment - 1) / alignInfo.alignment * alignInfo.alignment;

            // 更新 offset
            offset = property.offset + property.size;
        }
        binding++;
    }
    for (auto& property : info.shaderProperties.textureProperties) {
        property.binding = binding;
        binding++;
    }

    //SSBOS
    for (auto& ssbo : info.ssbos) {
        ssbo.binding = binding;
        binding++;
    }

}

std::string FrameWork::ShaderParse::GetCodeBlock(const std::string &code, const std::string &blockName) {
    auto begin = code.find(blockName);
    if (begin == std::string::npos)
        return "";

    //保证开头无有效字符，保证结尾无有效字符
    // LOG_DEBUG("blockName: {}", blockName);
    // LOG_DEBUG("code[begin - 1] : {}", code[begin - 1]);
    // LOG_DEBUG("code[begin + size] : {}", code[begin + blockName.size()]);
    while ((begin > 0 && isValidChar(code[begin - 1])) ||
        (begin + blockName.size() < code.size() && isValidChar(code[begin + blockName.size()]))) {
        begin = code.find(blockName, begin + blockName.size());
        if (begin == std::string::npos) {
            return "";
        }
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
    uint32_t binding = 0;

    if (!shaderInfo.vertProperties.baseProperties.empty()) {
        uint32_t offset = 0;
        for (auto& property : shaderInfo.vertProperties.baseProperties) {
            auto alignInfo = GetPropertyAlignInfoStd140(property.type, property.arrayLength);
            property.size = alignInfo.size;
            property.binding = binding;
            property.align = alignInfo.alignment;
            property.arrayOffset = alignInfo.arrayOffset;

            // 正确的对齐计算
            property.offset = (offset + alignInfo.alignment - 1) / alignInfo.alignment * alignInfo.alignment;

            // 更新 offset 到下一个位置
            offset = property.offset + property.size;
        }
        binding++;
    }

    if (!shaderInfo.fragProperties.baseProperties.empty()) {
        uint32_t offset = 0;
        for (auto& property : shaderInfo.fragProperties.baseProperties) {
            auto alignInfo = GetPropertyAlignInfoStd140(property.type, property.arrayLength);
            property.size = alignInfo.size;
            property.binding = binding;
            property.align = alignInfo.alignment;
            property.arrayOffset = alignInfo.arrayOffset;

            // 正确的对齐计算
            property.offset = (offset + alignInfo.alignment - 1) / alignInfo.alignment * alignInfo.alignment;

            // 更新 offset
            offset = property.offset + property.size;
        }
        binding++;
    }

    for (auto& property : shaderInfo.vertProperties.textureProperties) {
        property.binding = binding;
        binding++;
    }

    for (auto& property : shaderInfo.fragProperties.textureProperties) {
        property.binding = binding;
        binding++;
    }
}

FrameWork::PropertyAlignInfo FrameWork::ShaderParse::GetPropertyAlignInfoStd140(ShaderPropertyType type, uint32_t arrayLength) {
    uint32_t std_size = sizeof(float);

    if (type == ShaderPropertyType::BOOL || type == ShaderPropertyType::INT ||
        type == ShaderPropertyType::UINT || type == ShaderPropertyType::FLOAT) {
        if (arrayLength == 0) {
            return {.size = std_size, .alignment = std_size};
        } else {
            // 标量数组：每个元素按 vec4 对齐
            return {.size = std_size * 4 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4};
        }
    } else if (type == ShaderPropertyType::VEC2) {
        if (arrayLength == 0) {
            return {.size = std_size * 2, .alignment = std_size * 2};
        } else {
            return {.size = std_size * 4 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4};
        }
    } else if (type == ShaderPropertyType::VEC3) {
        if (arrayLength == 0) {
            return {.size = std_size * 3, .alignment = std_size * 4};
        } else {
            return {.size = std_size * 4 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4};
        }
    } else if (type == ShaderPropertyType::VEC4) {
        if (arrayLength == 0) {
            return {.size = std_size * 4, .alignment = std_size * 4};
        } else {
            return {.size = std_size * 4 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4};
        }
    } else if (type == ShaderPropertyType::MAT2) {
        if (arrayLength == 0) {
            return {.size = std_size * 4 * 2, .alignment = std_size * 4}; // 2列，每列vec4
        } else {
            return {.size = std_size * 4 * 2 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4 * 2};
        }
    } else if (type == ShaderPropertyType::MAT3) {
        if (arrayLength == 0) {
            return {.size = std_size * 4 * 3, .alignment = std_size * 4}; // 3列，每列vec4
        } else {
            return {.size = std_size * 4 * 3 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4 * 3};
        }
    } else if (type == ShaderPropertyType::MAT4) {
        if (arrayLength == 0) {
            return {.size = std_size * 4 * 4, .alignment = std_size * 4}; // 4列，每列vec4
        } else {
            return {.size = std_size * 4 * 4 * arrayLength,
                    .alignment = std_size * 4,
                    .arrayOffset = std_size * 4 * 4};
        }
    } else {
        LOG_ERROR("Invalid shader property type!");
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

    size_t s = 0;
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

    //防止最后一个字只有一个char，也就是将缓冲区中词完全退出
    if (record) {
        words.push_back(str.substr(s));
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

void FrameWork::ShaderParse::ReplaceAllWordsInBlock(std::string &blockCode, const std::string &src,
    const std::string &dst) {
    size_t offset = 0;
    auto pos = FindWord(blockCode, src, offset);
    auto srcLen = src.size();
    auto dstLen = dst.size();
    while (pos != std::string::npos) {
        //找到对应单词则替换
        blockCode.replace(pos, srcLen, dst);
        offset = pos + dstLen;//查找起始位置更新到替换单词的后一个
        pos = FindWord(blockCode, src, offset);//继续查找后面相同单词
    }
}

bool FrameWork::ShaderParse::IsBaseProperty(ShaderPropertyType type) {
    return !(type == ShaderPropertyType::SAMPLER || type == ShaderPropertyType::SAMPLER_2D || type == ShaderPropertyType::SAMPLER_CUBE);
}

std::string FrameWork::ShaderParse::TranslateToVulkan(const std::string &code, const ShaderPropertiesInfo &properties) {
    if (code.empty()) {
        LOG_WARNING("Empty code in TranslateToVulkan");
        return "";
    }
    //着色器版本
    std::string vulkanCode = "#version 450 core\n\n";
    size_t pos = 0;

    //处理输入内容
    std::string inputBlock = GetCodeBlock(code, "Input");
    auto lines = SplitString(inputBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if ( words.size() >= 3 && words[0] != "//") {
            vulkanCode += "layout (location = " + words[0] + ") in " + words[1] + " " + words[2] + ";\n";
        }
    }
    vulkanCode += "\n";

    //TODO几何着色器的特化处理

    //TODO实例化处理

    //输出内容，例如vertex传递到frag的部分内容
    std::string outputBlock = GetCodeBlock(code, "Output");
    lines = SplitString(outputBlock, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (words.size() >= 3 && words[0] != "//") {
            vulkanCode += "layout(location = " + words[0] + ") out " + words[1] + " " + words[2] + ";\n";
        }
    }
    vulkanCode += "\n";

    //处理UBO，这里的思路是使用uniformObject结构体，到host端中可以使用vulkan内存规则，
    //使用主机可见的内存直接对内存区域映射，不需要创建结构体与着色器对应
    //此处将所有的UniformData塞到一个结构体中，vulkan支持离散的绑定点所以问题不大，
    //并且使用一个整体的UniformBuffer保证了内存的利用效率相较于一一对应绑定点
    if (!properties.baseProperties.empty()) {
        vulkanCode += "layout (binding = " + std::to_string(properties.baseProperties[0].binding) + ") uniform UniformBufferObject {\n";
        for (const auto & property : properties.baseProperties) {
            if (property.arrayLength == 0) {
                //非数组类型
                vulkanCode += "    " + propertyTypeMapToGLSL[property.type] + " " + property.name + ";\n";
            }else {
                //数组类型
                vulkanCode += "    " + propertyTypeMapToGLSL[property.type] + " " + property.name + "["
                + std::to_string(property.arrayLength) + "]" + ";\n";
            }
        }
        vulkanCode += "} _UBO;\n";
    }
    vulkanCode += "\n";

    if (!properties.textureProperties.empty()) {
        for (const auto & texturePropertie : properties.textureProperties) {
            vulkanCode += "layout (binding = " + std::to_string(texturePropertie.binding) +
                ") uniform sampler2D " + texturePropertie.name + ";\n";
        }
    }

    //将code中使用uniformData 的变量名加上前缀
    std::string programBlock = GetCodeBlock(code, "Program");
    if (!properties.baseProperties.empty()) {
        for (auto& property : properties.baseProperties) {
            ReplaceAllWordsInBlock(programBlock, property.name,  "_UBO." + property.name);
        }
    }
    vulkanCode += programBlock;


    //返回
    return vulkanCode;
}

FrameWork::CompShaderInfo FrameWork::ShaderParse::GetCompShaderInfo(const std::string &code) {
    CompShaderInfo info{};
    info.localInvocation = GetCompLocalInvocation(code);
    info.shaderProperties = GetShaderProperties(code);
    info.ssbos = GetSSBOs(code);
    SetUpCompSSBOAndProperty(info);
    return info;
}

std::string FrameWork::ShaderParse::TranslateCompToVulkan(const std::string &code, const CompShaderInfo & shaderInfo) {
    auto ToUp = [](const std::string &code) {
        std::string rt;
        for (auto& c : code) {
            rt.push_back(std::toupper(c));
        }
        return rt;
    };
    if (code.empty()) {
        LOG_WARNING("Empty code in TranslateCompToVulkan");
        return "";
    }

    //着色器版本
    std::string vulkanCode = "#version 450 core\n\n";
    size_t pos = 0;

    //Properties:
    auto properties = shaderInfo.shaderProperties;

    if (!properties.baseProperties.empty()) {
        vulkanCode += "layout (binding = " + std::to_string(properties.baseProperties[0].binding) + ") uniform UniformBufferObject {\n";
        for (const auto & property : properties.baseProperties) {
            if (property.arrayLength == 0) {
                //非数组类型
                vulkanCode += "    " + propertyTypeMapToGLSL[property.type] + " " + property.name + ";\n";
            }else {
                //数组类型
                vulkanCode += "    " + propertyTypeMapToGLSL[property.type] + " " + property.name + "["
                + std::to_string(property.arrayLength) + "]" + ";\n";
            }
        }
        vulkanCode += "} _UBO;\n";
    }
    vulkanCode += "\n";

    if (!properties.textureProperties.empty()) {
        for (auto & texturePropertie : properties.textureProperties) {
            vulkanCode += "layout (binding = " + std::to_string(texturePropertie.binding) +
                ") uniform sampler2D " + texturePropertie.name + ";\n";
        }
    }

    vulkanCode += "\n";

    //Calling
    vulkanCode += GetCodeBlock(code, "Calling");

    vulkanCode += "\n";

    //SSBO
    auto ssbos = shaderInfo.ssbos;
    for (auto& ssbo : ssbos) {
        if (ssbo.type == StorageObjectType::Image2D ||
            ssbo.type == StorageObjectType::Image3D ||
            ssbo.type == StorageObjectType::ImageCube) {
            vulkanCode += "layout(binding = " + std::to_string(ssbo.binding);
            vulkanCode += ", " + storageImageFormatToString[ssbo.storageImageFormat] + ") ";
            vulkanCode += "uniform ";
            vulkanCode += ssboOpToString[ssbo.ssboOP] + " " +
                storageTypeToStringType[ssbo.type] + " " + ssbo.name + ";\n";
        }
        if (ssbo.type == StorageObjectType::Buffer) {
            vulkanCode += "layout(std430, binding = " + std::to_string(ssbo.binding) + ") ";
            vulkanCode += ssboOpToString[ssbo.ssboOP] + " " + storageTypeToStringType[ssbo.type]
            + " " + ssbo.structName + "_SSBO_" + ToUp(ssboOpToString[ssbo.ssboOP]) + " {" + "\n    ";
            vulkanCode += ssbo.structName + " ";
            vulkanCode += ssbo.name + "[];\n};\n";
        }
        vulkanCode += "\n";
    }
    vulkanCode += "\n";

    //Local Invocation
    vulkanCode += "layout (local_size_x = " +
        std::to_string(shaderInfo.localInvocation.x)
        + ", local_size_y = " +
        std::to_string(shaderInfo.localInvocation.y)
            +", local_size_z = " +
        std::to_string(shaderInfo.localInvocation.z) +
        " ) in;";

    vulkanCode += "\n";
    //Program
    std::string programBlock = GetCodeBlock(code, "Program");
    if (!properties.baseProperties.empty()) {
        for (auto& property : properties.baseProperties) {
            ReplaceAllWordsInBlock(programBlock, property.name,  "_UBO." + property.name);
        }
    }
    vulkanCode += programBlock;

    return vulkanCode;
}

FrameWork::ShaderFormatsInfo FrameWork::ShaderParse::GetShaderFormatsInfo(const std::string &code) {
    ShaderFormatsInfo shaderFormatsInfo = {};
    auto block = GetCodeBlock(code, "Formats");
    if (block.empty()) {
        LOG_WARNING("Shader formats not found.");
        return shaderFormatsInfo;
    }
    //清空默认值
    shaderFormatsInfo.shaderFormats.clear();
    auto lines = SplitString(block, '\n');
    for (auto& line : lines) {
        auto words = ExtractWords(line);
        if (!words.empty() && words.size() == 1) {
            shaderFormatsInfo.shaderFormats.push_back(shaderFormatMap[words[0]]);
        }
    }
    return shaderFormatsInfo;
}
