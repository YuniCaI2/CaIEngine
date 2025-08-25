//
// Created by 51092 on 25-6-13.
//

#include "Resource.h"

#include "VulkanTool.h"
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <ranges>
#include <DirectXTex.h>
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <filesystem>


void FrameWork::Resource::processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes,  ModelType modelType, std::string directory, TextureTypeFlags textureFlags) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, modelType, scene, directory, textureFlags));
    }
    //接下来重复子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes, modelType, directory, textureFlags);
    }
}

FrameWork::MeshData FrameWork::Resource::processMesh(aiMesh *mesh, ModelType modelType, const aiScene *scene,  std::string directory, TextureTypeFlags textureFlags) {
    MeshData meshData;

    //Vertex
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;

        vertex.tangent.x = mesh->mTangents[i].x;
        vertex.tangent.y = mesh->mTangents[i].y;
        vertex.tangent.z = mesh->mTangents[i].z;

        //一个顶点可以有多个纹理坐标这里取第一个
        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }else {
            //异常
            vertex.texCoord.x = 0;
            vertex.texCoord.y = 0;
            std::cerr << "Warning : Don't have texCoord" << std::endl;
        }
        meshData.vertices.push_back(vertex);
    }

    //index
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
         aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            meshData.indices.push_back(face.mIndices[j]);
        }
    }

    //纹理加载
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    //这里先只加载漫反射贴图--因为我手头的模型除了PBR就是漫反射贴图
    if ((textureFlags & DiffuseColor) == DiffuseColor) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_DIFFUSE,  directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & BaseColor) == BaseColor) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_BASE_COLOR,  directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & Normal) == Normal) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_NORMALS,  directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & MetallicRoughness) == MetallicRoughness) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_METALNESS,  directory);
        if (textureMap.empty()) {
            textureMap = LoadTextureFullDatas(material, scene, aiTextureType_DIFFUSE_ROUGHNESS,  directory);
            if (textureMap.empty()) {
                textureMap = LoadTextureFullDatas(material, scene, aiTextureType_UNKNOWN,  directory);
            }
        }
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & Emissive) == Emissive) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_EMISSIVE, directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & Occlusion) == Occlusion) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_AMBIENT_OCCLUSION, directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }

    return meshData;
}

FrameWork::TextureFullData FrameWork::Resource::CreateDefaultTexture(TextureTypeFlagBits type) {
    int width = 100, height = 100, numChannels = 4;
    uint32_t desireChannels = 4;
    TextureFullData texData;
    texData.width = width;
    texData.height = height;
    texData.numChannels = desireChannels;
    texData.path = "None";
    texData.type = type;
    unsigned char* pixels = new unsigned char[width * height * numChannels];
    if (type == TextureTypeFlagBits::DiffuseColor) {
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 255;//全部置为1
        }
    }
    if (type == TextureTypeFlagBits::MetallicRoughness) {
        for (uint32_t i = 0; i < width * height; i++) {
            pixels[i * numChannels + 0] = 0;
            pixels[i * numChannels + 1] = 0.5 * 255;
            pixels[i * numChannels + 2] = 0;
            pixels[i * numChannels + 3] = 1 * 255;
        }
    }
    if (type == TextureTypeFlagBits::Emissive) {
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 0;
        }
    }
    if (type == TextureTypeFlagBits::Occlusion) {
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 255;
        }
    }
    if (type == TextureTypeFlagBits::Normal) {
        for (uint32_t i = 0; i < width * height; i++) {
            pixels[i * 4] = 255 * 0.5;
            pixels[i * 4 + 1] = 255 * 0.5;
            pixels[i * 4 + 2] = 255;
            pixels[i * 4 + 3] = 255;
        }
    }
    if (type == TextureTypeFlagBits::BaseColor) {
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 1;

        }
    }
    texData.data = pixels;
    return texData;
}

void FrameWork::Resource::SaveCache(const std::string &filePath) const{
    std::ofstream file(shaderTimeCachePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file " + filePath);
    }
    for (const auto& [pathStr, time] : shaderTimeCache) {
        uint32_t pathLen = pathStr.length();
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));

        file.write(pathStr.c_str(), pathLen);

        auto timeRep = time.time_since_epoch().count();
        file.write(reinterpret_cast<const char*>(&timeRep), sizeof(timeRep));
    }
}


void FrameWork::Resource::LoadShaderCache() const{
    ShaderTimeCache cache;
    std::ifstream file(shaderTimeCachePath, std::ios::binary);
    if (!file.is_open()) {
        //未创建直接返回
        std::cerr << "the first use shader time cache , shader cache is empty" << std::endl;
        return;
    }
    uint32_t pathLen = 0;
    while (file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) {
        if (file.eof()) {
            throw std::runtime_error("Failed to read path str in : " + shaderTimeCachePath);
        }
        std::string pathStr(pathLen, '\0');
        file.read(pathStr.data(), pathLen);
        if (file.eof()) {
            throw std::runtime_error("Failed to read time in : " + shaderTimeCachePath);
        }
        long long time;
        file.read(reinterpret_cast<char*>(&time), sizeof(time));
        std::filesystem::file_time_type fileTime{std::filesystem::file_time_type::duration(time)};
        cache[pathStr] = fileTime;
    }
    shaderTimeCache = cache;
}

void FrameWork::Resource::CompileShader(const std::string &filepath) const{
    std::string command = "GLSLANG "+ filepath + " -V -o " + filepath + ".spv";
    std::cout << "Compiling shader:   " << filepath << std::endl;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Error compiling shader: " << filepath << std::endl;
    }else {
        std::cout << "Shader compiled successfully!" << std::endl;
        std::cout << std::endl;
    }
}

void FrameWork::Resource::CompileShaderModify() const {
    std::string vertExtension = ".vert";
    std::string fragExtension = ".frag";
    std::string compExtension = ".comp";
    std::string vertspvExtension  = ".vert.spv";
    std::string fragspvExtension  = ".frag.spv";
    std::string compspvExtension  = ".comp.spv";
    std::filesystem::path allShaderPath = generalShaderPath;
    if (! std::filesystem::exists(allShaderPath) || ! std::filesystem::is_directory(allShaderPath)) {
        std::cerr << "Error loading shader: " << allShaderPath << std::endl;
        throw std::runtime_error("Error loading shader: " + generalShaderPath);
    }
    auto GetExtension =  [](const std::string& path) {
        auto start = path.find_last_of("/\\");
        auto dotstart = path.substr(start).find_first_of('.');
        if (start == std::string::npos || dotstart == std::string::npos) {
            throw std::runtime_error("Error loading shader: " + path);
        }

        return path.substr(start + dotstart);
    };

    auto IfCompile  = [](const std::filesystem::path &filepath1, const std::filesystem::file_time_type& time)->bool {
        if (filepath1.string() == ".") {
            return false;
        }
        auto time1 = std::filesystem::last_write_time(filepath1);
        if (time1 == time) {
            return false;
        }else {
            return true;
        }
    };

    //加载时间缓存
    try {
        LoadShaderCache();
    }catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    bool flag = false; //标记位
    //遍历所有文件夹
    for (auto& singleFile :
        std::filesystem::directory_iterator(allShaderPath)) {
        if (singleFile.is_directory()) {
            std::vector<std::filesystem::path> shaderPaths(6, ".");
            for (auto& shaderPath : std::filesystem::directory_iterator(singleFile)) {
                std::string path = shaderPath.path().string();
                std::string extension = GetExtension(path);
                if (extension == vertExtension) {
                    shaderPaths[0] = shaderPath;
                }
                if (extension == fragExtension) {
                    shaderPaths[1] = shaderPath;
                }
                if (extension == compExtension) {
                    shaderPaths[2] = shaderPath;
                }
                if (extension == vertspvExtension) {
                    shaderPaths[3] = shaderPath;
                }
                if (extension == fragspvExtension) {
                    shaderPaths[4] = shaderPath;
                }
                if (extension == compspvExtension) {
                    shaderPaths[5] = shaderPath;
                }
            }
            //这里3指的是3种
            for (int i = 0; i < 3; i++) {
                if (shaderTimeCache.find(shaderPaths[i].string()) != shaderTimeCache.end()) {
                    if (IfCompile(shaderPaths[i], shaderTimeCache[shaderPaths[i].string()])) {
                        CompileShader(shaderPaths[i].string());
                        flag = true;
                        shaderTimeCache[shaderPaths[i].string()] = last_write_time(shaderPaths[i]);
                    }
                }else {
                        CompileShader(shaderPaths[i].string());
                        flag = true;
                        shaderTimeCache[shaderPaths[i].string()] = last_write_time(shaderPaths[i]);
                }
            }
        }
    }
    if (flag) {
        SaveCache(shaderTimeCachePath);
    }



}

FrameWork::Resource::Resource() {
}

VkShaderModule FrameWork::Resource::getShaderModulFromFile(VkDevice device, const std::string &fileName, VkShaderStageFlags shaderStage) const{
    auto shaderPath = generalShaderPath + fileName + "/" + fileName;
    switch (shaderStage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            shaderPath += ".vert";
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            shaderPath += ".frag";
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            shaderPath += ".comp";
            break;
        default:
            break;
    }
    shaderPath += ".spv";

    VkShaderModule shaderModule;
    try {
        CompileShaderModify();
        shaderModule = VulkanTool::loadShader(shaderPath, device);
    }catch(const std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

//这里默认这里的Obj不支持PBR贴图，等待后续扩展
std::vector<FrameWork::MeshData> FrameWork::Resource::LoadMesh(const std::string &fileName, ModelType modelType, TextureTypeFlags textureFlags, float scale) {
    Assimp::Importer importer;
    std::vector<std::string_view> fsplits;
    std::string path;
    std::string directory;
    if (modelType == ModelType::OBJ) {
        path = generalModelPath + fileName + "/" + fileName + ".obj";
        directory = generalModelPath + fileName + "/";
    } else if (modelType == ModelType::FBX) {
        path = generalModelPath + fileName + "/" + fileName + ".fbx";
        directory = generalModelPath + fileName + "/";
    } else if (modelType == ModelType::GLTF) {
        path = generalModelPath + fileName + "/" + fileName + ".glTF";
        directory = generalModelPath + fileName + "/";
    }else if (modelType == ModelType::GLB) {
        path = generalModelPath + fileName + "/" + fileName + ".glb";
        directory = generalModelPath + fileName + "/";
    }
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, scale);
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals |aiProcess_CalcTangentSpace | aiProcess_FlipUVs |  aiProcess_GlobalScale);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Failed to load model from file " + fileName);
    }
    std::vector<MeshData> meshes;//返回值
    processNode(scene->mRootNode, scene, meshes, modelType, directory, textureFlags);
    return meshes;
}

std::vector<FrameWork::TextureFullData> FrameWork::Resource::LoadTextureFullDatas(aiMaterial *mat, const aiScene* scene,aiTextureType type,
    std::string directory) {
    std::vector<TextureFullData> textures;
    int n = 0;
    n =  mat->GetTextureCount(type);
    for (unsigned int i = 0; i < n; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        if (auto it = textureMap.find(directory + str.C_Str()); it != textureMap.end()) {
            textures.push_back(it->second);
        }else {
            auto texture = scene->GetEmbeddedTexture(str.C_Str());
            if (texture != nullptr) {
                TextureFullData texData;
                texData.data = (unsigned char*)texture->pcData;
                texData.width = texture->mWidth;
                texData.height = texture->mHeight;
                if (type == aiTextureType_DIFFUSE) {
                    texData.type = DiffuseColor;
                }
                else if (type == aiTextureType_NORMALS) {
                    texData.type = Normal;
                }
                else if (type == aiTextureType_UNKNOWN) {
                    texData.type = MetallicRoughness;
                }
                else if (type == aiTextureType_METALNESS) {
                    texData.type = MetallicRoughness;
                }
                else if (type == aiTextureType_DIFFUSE_ROUGHNESS) {
                    texData.type = MetallicRoughness;
                }
                else if (type == aiTextureType_EMISSIVE) {
                    texData.type = Emissive;
                }
                else if (type == aiTextureType_AMBIENT_OCCLUSION) {
                    texData.type = Occlusion;
                }else if (type == aiTextureType_BASE_COLOR) {
                    texData.type = BaseColor;
                }
                else {
                    std::cerr << "this type process has not complete !" << std::endl;
                }
                if (texData.width == 0 || texData.height == 0) {
                    texData.data = stbi_load_from_memory(texData.data, texData.width,
                        &texData.width, &texData.height, &texData.numChannels, 4);
                }
                texData.path = directory + str.C_Str();
                texData.numChannels = 4;
                textures.push_back(texData);
                textureMap[directory + str.C_Str()] = texData;

            }else {
                TextureFullData texData;
                if (type == aiTextureType_DIFFUSE) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::DiffuseColor);
                }
                else if (type == aiTextureType_NORMALS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Normal);
                }
                else if (type == aiTextureType_UNKNOWN) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                }
                else if (type == aiTextureType_METALNESS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                }
                else if (type == aiTextureType_DIFFUSE_ROUGHNESS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                }
                else if (type == aiTextureType_EMISSIVE) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Emissive);
                }
                else if (type == aiTextureType_AMBIENT_OCCLUSION) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Occlusion);
                }
                else if (type == aiTextureType_BASE_COLOR) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::BaseColor);
                }
                else {
                    std::cerr << "this type process has not complete !" << std::endl;
                }
                textures.push_back(texData);
                textureMap[directory + str.C_Str()] = texData;
            }

        }
    }

    if (n <= 0) {
        if (type == aiTextureType_DIFFUSE) {
            textures.push_back(CreateDefaultTexture(DiffuseColor));
        }
        else if (type == aiTextureType_NORMALS) {
            textures.push_back(CreateDefaultTexture(Normal));
        }
        else if (type == aiTextureType_UNKNOWN) {
            textures.push_back(CreateDefaultTexture(MetallicRoughness));
        }
        else if (type == aiTextureType_EMISSIVE) {
            textures.push_back(CreateDefaultTexture(Emissive));
        }
        else if (type == aiTextureType_AMBIENT_OCCLUSION) {
            textures.push_back(CreateDefaultTexture(Occlusion));
        }
        else if (type == aiTextureType_BASE_COLOR) {
            textures.push_back(CreateDefaultTexture(BaseColor));
        }
        else {
            std::cerr << "this type process has not complete !" << std::endl;
        }
    }
    return textures;
}


FrameWork::TextureFullData FrameWork::Resource::LoadTextureFullData(const std::string &filePath, TextureTypeFlagBits type) {
    // 获取文件扩展名
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();

    // 转换为小写进行比较
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".dds") {
        // 使用DirectXTex加载DDS文件
        return LoadDDSTexture(filePath, type);
    } else {
        // 使用STB加载其他格式（保持原有逻辑）
        return LoadSTBTexture(filePath, type);
    }
}

FrameWork::TextureFullData FrameWork::Resource::LoadDDSTexture(const std::string &filePath, TextureTypeFlagBits type) {
    using namespace DirectX;

    TextureFullData texData;
    texData.path = filePath;
    texData.type = type;

    // 转换文件路径为宽字符
    std::wstring wFilePath(filePath.begin(), filePath.end());

    // 加载DDS文件
    TexMetadata metadata;
    ScratchImage image;

    HRESULT hr = LoadFromDDSFile(wFilePath.c_str(), DDS_FLAGS_NONE, &metadata, image);
    if (FAILED(hr)) {
        std::cerr << "Failed to load DDS texture from file: " << filePath << std::endl;
        exit(-1);
    }


    switch (metadata.format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:  // FLOAT32, 4通道
        case DXGI_FORMAT_R16G16B16A16_FLOAT:  // FLOAT16, 4通道
            break;
        default:
            std::cerr << "Unsupported DDS format. Only FLOAT32 and FLOAT16 4-channel formats are supported." << std::endl;
            exit(-1);
    }

    // 获取图像数据
    const Image* img = image.GetImage(0, 0, 0);
    if (!img) {
        std::cerr << "Failed to get image data from DDS file: " << filePath << std::endl;
        exit(-1);
    }

    // 分配内存并复制数据
    size_t dataSize = img->rowPitch * img->height; //rowPitch 是每行字节数
    unsigned char* data = new unsigned char[dataSize];
    memcpy(data, img->pixels, dataSize);

    // 填充原有的TextureFullData结构
    texData.width = static_cast<int>(metadata.width);
    texData.height = static_cast<int>(metadata.height);
    texData.numChannels = 4; // DDS文件我们只支持4通道
    texData.data = data;

    return texData;
}

FrameWork::TextureFullData FrameWork::Resource::LoadSTBTexture(const std::string &filePath, TextureTypeFlagBits type) {
    int width = 100, height = 100, numChannels;
    uint32_t desireChannels = 4;
    unsigned char* data = nullptr;

    data = stbi_load(filePath.c_str(), &width, &height, &numChannels, desireChannels);

    TextureFullData texData;
    texData.width = width;
    texData.height = height;
    texData.numChannels = desireChannels;
    texData.data = data;
    texData.path = filePath;
    texData.type = type;

    if (!data) {
        std::cerr << "Failed to load texture from file, may be the directory was wrong " << filePath << std::endl;
        exit(-1);
    }

    return texData;
}

void FrameWork::Resource::ReleaseTextureFullData(const TextureFullData &textureFullData) {
    //主要是释放图像的指针指向的数据
    for (auto& it : textureMap) {
        if (it.second.path == textureFullData.path) {
            stbi_image_free(it.second.data);
            textureMap.erase(it.first);
            return;
        }
    }
}


FrameWork::Resource& FrameWork::Resource::GetInstance() {
    static Resource instance;
    return instance;
}
