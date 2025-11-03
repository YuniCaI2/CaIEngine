#include"Serialize.h"
#include "ResourceManager.h"
#include "Logger.h"
#include "PublicStruct.h"
#include "VulkanTool.h"
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <cstddef>
#include <exception>
#include <fstream>
#include <future>
#include <mutex>
#include<openssl/sha.h>
#include <vulkan/vulkan_core.h>

#include "ShaderParse.h"
#ifdef _WIN32
#include <DirectXTex.h>
#endif
#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <filesystem>
#include "Logger.h"
#include "Schema.h"
#include "FrameGraph/ThreadPool.h"

void FrameWork::ResourceManager::processNode(aiNode *node, const aiScene *scene, std::vector<MeshData> &meshes,
                                      ModelType modelType, std::string directory, TextureTypeFlags textureFlags) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, modelType, scene, directory, textureFlags));
    }
    //接下来重复子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes, modelType, directory, textureFlags);
    }
}

FrameWork::MeshData FrameWork::ResourceManager::processMesh(aiMesh *mesh, ModelType modelType, const aiScene *scene,
                                                     std::string directory, TextureTypeFlags textureFlags) {
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
        } else {
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
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    //这里先只加载漫反射贴图--因为我手头的模型除了PBR就是漫反射贴图
    if ((textureFlags & DiffuseColor) == DiffuseColor) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_DIFFUSE, directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & BaseColor) == BaseColor) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_BASE_COLOR, directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & Normal) == Normal) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_NORMALS, directory);
        meshData.texData.insert(meshData.texData.end(), textureMap.begin(), textureMap.end());
    }
    if ((textureFlags & MetallicRoughness) == MetallicRoughness) {
        auto textureMap = LoadTextureFullDatas(material, scene, aiTextureType_METALNESS, directory);
        if (textureMap.empty()) {
            textureMap = LoadTextureFullDatas(material, scene, aiTextureType_DIFFUSE_ROUGHNESS, directory);
            if (textureMap.empty()) {
                textureMap = LoadTextureFullDatas(material, scene, aiTextureType_UNKNOWN, directory);
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


FrameWork::TextureFullData FrameWork::ResourceManager::CreateDefaultTexture(TextureTypeFlagBits type) {
    int width = 100, height = 100, numChannels = 4;
    uint32_t desireChannels = 4;
    TextureFullData texData;
    texData.width = width;
    texData.height = height;
    texData.numChannels = desireChannels;
    texData.path = "None";
    texData.type = type;
    unsigned char *pixels = new unsigned char[width * height * numChannels];
    if (type == TextureTypeFlagBits::DiffuseColor) {
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 255; //全部置为1
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

void FrameWork::ResourceManager::SaveCache(const std::string &filePath, const ShaderTimeCache& shaderTimeCache) const {
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file {}", filePath);
    }
    for (const auto &[pathStr, time]: shaderTimeCache) {
        uint32_t pathLen = pathStr.length();
        file.write(reinterpret_cast<const char *>(&pathLen), sizeof(pathLen));

        file.write(pathStr.c_str(), pathLen);

        auto timeRep = time.time_since_epoch().count();
        file.write(reinterpret_cast<const char *>(&timeRep), sizeof(timeRep));
    }
}


FrameWork::ResourceManager::ShaderTimeCache FrameWork::ResourceManager::LoadShaderCache(const std::string& filePath) const {
    ShaderTimeCache cache;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        //未创建直接返回
        std::cerr << "the first use shader time cache , shader cache is empty" << std::endl;
        return {};
    }
    uint32_t pathLen = 0;
    while (file.read(reinterpret_cast<char *>(&pathLen), sizeof(pathLen))) {
        if (file.eof()) {
            throw std::runtime_error("Failed to read path str in : " + filePath);
        }
        std::string pathStr(pathLen, '\0');
        file.read(pathStr.data(), pathLen);
        if (file.eof()) {
            throw std::runtime_error("Failed to read time in : " + filePath);
        }
        std::filesystem::file_time_type timeType; // 用来得到timeType类型
        decltype(timeType.time_since_epoch().count()) time;
        file.read(reinterpret_cast<char *>(&time), sizeof(time));
        std::filesystem::file_time_type fileTime{std::filesystem::file_time_type::duration(time)};
        cache[pathStr] = fileTime;
    }
    return cache;
}

void FrameWork::ResourceManager::CompileShader(const std::string &filepath) const {
    std::string command = "GLSLANG " + filepath + " -V -o " + filepath + ".spv";
    std::cout << "Compiling shader:   " << filepath << std::endl;
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Error compiling shader: " << filepath << std::endl;
    } else {
        std::cout << "Shader compiled successfully!" << std::endl;
        std::cout << std::endl;
    }
}

FrameWork::ResourceManager::ResourceManager() {
    std::ifstream iassetCacheTable(assetCacheTablePath);
    if (!iassetCacheTable.is_open()) {
        //意味着没创建
        std::ofstream ofile(assetCacheTablePath);
        ofile.close();
    }else {
        if (!iassetCacheTable.eof()) {
            assetCacheTable = nlohmann::json::parse(iassetCacheTable);
        }
    }
}

FrameWork::ResourceManager::~ResourceManager() {
    std::ofstream oassetCacheTable(assetCacheTablePath);
    nlohmann::json assetCache = assetCacheTable;
    oassetCacheTable << std::setw(4) << assetCache;
}


std::vector<FrameWork::MeshData> FrameWork::ResourceManager::LoadMesh(const std::string &fileName, ModelType modelType,
                                                                      TextureTypeFlags textureFlags, float scale) {
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
    } else if (modelType == ModelType::GLB) {
        path = generalModelPath + fileName + "/" + fileName + ".glb";
        directory = generalModelPath + fileName + "/";
    }
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, scale);
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
              aiProcess_GlobalScale);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Failed to load model from file " + fileName);
    }
    std::vector<MeshData> meshes; //返回值
    processNode(scene->mRootNode, scene, meshes, modelType, directory, textureFlags);
    return meshes;
}

std::unique_ptr<FrameWork::ModelData> FrameWork::ResourceManager::LoadModelData(const std::string &filePath,
    TextureTypeFlags textureFlags) {
    std::string extra = filePath.substr(filePath.find_last_of('.') + 1);
    if (extra == "") {
        LOG_ERROR("FilePath: {} has mistake", filePath);
        return nullptr;
    }
    ModelType modelType {ModelType::OBJ};
    if (extra == "obj") {
        modelType = ModelType::OBJ;
    }
    else if (extra == "fbx") {
        modelType = ModelType::FBX;
    }
    else if (extra == "gltf") {
        modelType = ModelType::GLTF;
    }else {
        LOG_ERROR("Can't find suitable modelType for {}", extra);
        return nullptr;
    }
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        filePath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
              aiProcess_GlobalScale);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("Failed to load model from file {}", filePath);
        return nullptr;
    }

    auto directory = filePath.substr(0, filePath.find_last_of('/') + 1); //得到目录
    auto modelData = std::make_unique<FrameWork::ModelData>();
    processNode(scene->mRootNode, scene, modelData->meshDatas, modelType, directory, textureFlags);
    return modelData;
}

std::vector<FrameWork::TextureFullData> FrameWork::ResourceManager::LoadTextureFullDatas(
    aiMaterial *mat, const aiScene *scene, aiTextureType type,
    std::string directory) {
    std::vector<TextureFullData> textures;
    int n = 0;
    n = mat->GetTextureCount(type);
    for (unsigned int i = 0; i < n; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        if (auto it = textureMap.find(directory + str.C_Str()); it != textureMap.end()) {
            textures.push_back(it->second);
        } else {
            auto texture = scene->GetEmbeddedTexture(str.C_Str());
            if (texture != nullptr) {
                TextureFullData texData;
                texData.data = (unsigned char *) texture->pcData;
                texData.width = texture->mWidth;
                texData.height = texture->mHeight;
                if (type == aiTextureType_DIFFUSE) {
                    texData.type = DiffuseColor;
                } else if (type == aiTextureType_NORMALS) {
                    texData.type = Normal;
                } else if (type == aiTextureType_UNKNOWN) {
                    texData.type = MetallicRoughness;
                } else if (type == aiTextureType_METALNESS) {
                    texData.type = MetallicRoughness;
                } else if (type == aiTextureType_DIFFUSE_ROUGHNESS) {
                    texData.type = MetallicRoughness;
                } else if (type == aiTextureType_EMISSIVE) {
                    texData.type = Emissive;
                } else if (type == aiTextureType_AMBIENT_OCCLUSION) {
                    texData.type = Occlusion;
                } else if (type == aiTextureType_BASE_COLOR) {
                    texData.type = BaseColor;
                } else {
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
            } else {
                TextureFullData texData;
                if (type == aiTextureType_DIFFUSE) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::DiffuseColor);
                } else if (type == aiTextureType_NORMALS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Normal);
                } else if (type == aiTextureType_UNKNOWN) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                } else if (type == aiTextureType_METALNESS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                } else if (type == aiTextureType_DIFFUSE_ROUGHNESS) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::MetallicRoughness);
                } else if (type == aiTextureType_EMISSIVE) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Emissive);
                } else if (type == aiTextureType_AMBIENT_OCCLUSION) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::Occlusion);
                } else if (type == aiTextureType_BASE_COLOR) {
                    texData = LoadTextureFullData(directory + str.C_Str(), TextureTypeFlagBits::BaseColor);
                } else {
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
        } else if (type == aiTextureType_NORMALS) {
            textures.push_back(CreateDefaultTexture(Normal));
        } else if (type == aiTextureType_UNKNOWN) {
            textures.push_back(CreateDefaultTexture(MetallicRoughness));
        } else if (type == aiTextureType_EMISSIVE) {
            textures.push_back(CreateDefaultTexture(Emissive));
        } else if (type == aiTextureType_AMBIENT_OCCLUSION) {
            textures.push_back(CreateDefaultTexture(Occlusion));
        } else if (type == aiTextureType_BASE_COLOR) {
            textures.push_back(CreateDefaultTexture(BaseColor));
        } else {
            std::cerr << "this type process has not complete !" << std::endl;
        }
    }
    return textures;
}

ExpectWithStr<std::unique_ptr<FrameWork::PrefabStruct>> FrameWork::ResourceManager::LoadPrefabStruct(
    const std::string &filePath) const {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return std::unexpected("Can't open file : " + filePath);
    }
    nlohmann::json jsonData = nlohmann::json::parse(file);
    auto prefabStruct = std::make_unique<PrefabStruct>(jsonData.get<PrefabStruct>());
    return prefabStruct;
}


FrameWork::TextureFullData FrameWork::ResourceManager::LoadTextureFullData(const std::string &filePath,
                                                                    TextureTypeFlagBits type) {
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

FrameWork::TextureFullData FrameWork::ResourceManager::LoadDDSTexture(const std::string &filePath, TextureTypeFlagBits type) {
    TextureFullData texData{};
#ifdef _WIN32
    using namespace DirectX;
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
        case DXGI_FORMAT_R32G32B32A32_FLOAT: // FLOAT32, 4通道
        case DXGI_FORMAT_R16G16B16A16_FLOAT: // FLOAT16, 4通道
            break;
        default:
            std::cerr << "Unsupported DDS format. Only FLOAT32 and FLOAT16 4-channel formats are supported." <<
                    std::endl;
            exit(-1);
    }

    // 获取图像数据
    const Image *img = image.GetImage(0, 0, 0);
    if (!img) {
        std::cerr << "Failed to get image data from DDS file: " << filePath << std::endl;
        exit(-1);
    }

    // 分配内存并复制数据
    size_t dataSize = img->rowPitch * img->height; //rowPitch 是每行字节数
    unsigned char *data = new unsigned char[dataSize];
    memcpy(data, img->pixels, dataSize);

    // 填充原有的TextureFullData结构
    texData.width = static_cast<int>(metadata.width);
    texData.height = static_cast<int>(metadata.height);
    texData.numChannels = 4; // DDS文件我们只支持4通道
    texData.data = data;
#endif


    return texData;
}

FrameWork::TextureFullData FrameWork::ResourceManager::LoadSTBTexture(const std::string &filePath, TextureTypeFlagBits type) {
    int width = 100, height = 100, numChannels;
    uint32_t desireChannels = 4;
    unsigned char *data = nullptr;

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

void FrameWork::ResourceManager::LoadDDSTextureAsset(const std::string &filePath, TextureAsset &textureAsset) {
#ifdef _WIN32
    using namespace DirectX;
    textureAsset.sourcePath = filePath;

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
        case DXGI_FORMAT_R32G32B32A32_FLOAT: // FLOAT32, 4通道
        case DXGI_FORMAT_R16G16B16A16_FLOAT: // FLOAT16, 4通道
            break;
        default:
            std::cerr << "Unsupported DDS format. Only FLOAT32 and FLOAT16 4-channel formats are supported." <<
                    std::endl;
            exit(-1);
    }

    // 获取图像数据
    const Image *img = image.GetImage(0, 0, 0);
    if (!img) {
        std::cerr << "Failed to get image data from DDS file: " << filePath << std::endl;
        exit(-1);
    }

    // 分配内存并复制数据
    size_t dataSize = img->rowPitch * img->height; //rowPitch 是每行字节数
    unsigned char *data = new unsigned char[dataSize];
    memcpy(data, img->pixels, dataSize);

    // 填充原有的TextureFullData结构
    textureAsset.width = static_cast<int>(metadata.width);
    textureAsset.height = static_cast<int>(metadata.height);
    textureAsset.numChannel = 4; // DDS文件我们只支持4通道
    textureAsset.data = data;
#endif
}

void FrameWork::ResourceManager::LoadSTBTextureAsset(const std::string &filePath, TextureAsset &textureAsset) {
    int width = 100, height = 100, numChannels;
    uint32_t desireChannels = 4;
    unsigned char *data = nullptr;

    data = stbi_load(filePath.c_str(), &width, &height, &numChannels, desireChannels);

    TextureFullData texData;
    textureAsset.width = width;
    textureAsset.height = height;
    textureAsset.numChannel = desireChannels;
    textureAsset.data = data;
    textureAsset.sourcePath = filePath;

    if (!data) {
        std::cerr << "Failed to load texture from file, may be the directory was wrong " << filePath << std::endl;
        exit(-1);
    }
}


nlohmann::json LoadJSONFromPath(const std::string & path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("Resource Manager does not exist : "  ,path);
        throw std::runtime_error("Resource Manager does not exist : " + path);
    }
    if (std::filesystem::path(path).extension() != ".json") {
        LOG_ERROR("Resource Manager does not JSON file {}"  ,path);
        throw std::runtime_error("Resource Manager does not JSON file : " + path);
    }
    std::ifstream file(path);
    nlohmann::json j;
    if (!file.is_open()) {
        LOG_ERROR("Open file failed: {}", path);
        throw std::runtime_error("Open file failed: " + path);
    }
    try {
        file >> j;
        return j;
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("JSON Error in : {}", e.what());
        throw std::runtime_error("JSON error in \""
            + path + "\": " + std::string(e.what()));
    }
    return j;
}

//计算hash
std::string ComputeFileSHA256(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file for hashing: " + path);

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    std::vector<char> buffer(8192);
    while (file.good()) {
        file.read(buffer.data(), buffer.size());
        SHA256_Update(&ctx, buffer.data(), file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    std::ostringstream result;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        result << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return result.str();
}


std::unique_ptr<uint32_t[]> LoadShaderSpv(const std::string& path, uint32_t& shaderSize) {
    auto shaderCode = std::make_unique<uint32_t[]>(shaderSize);
    std::ifstream file(path, std::ios::ate | std::ios::binary | std::ios::in);
    if (!file.is_open()) {
        LOG_ERROR("Open shader file failed: {}", path);
        throw std::runtime_error("Failed to open file for reading: " + path);
    }else {
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);
        shaderCode.reset();
        shaderCode = std::make_unique<uint32_t[]>(size);
        file.read(reinterpret_cast<char*>(shaderCode.get()), size);
        file.close();
        assert(size > 0);
        shaderSize = size;
    }
    return shaderCode;
}

void FrameWork::ResourceManager::CompileCaIShader(const std::string &path, std::shared_ptr<ShaderPass> shaderPass) {
    shaderPass->sourcePath = path;
    std::ifstream caiShaderFile(path);
    if (!caiShaderFile.is_open()) {
        LOG_ERROR("Failed to open cai Shader file from: {}", path);
        throw std::runtime_error("Failed to open cai Shader file from: " + path);
    }
    std::stringstream ss;
    ss << caiShaderFile.rdbuf();
    std::string code = ss.str();
    std::string vert, frag;
    ShaderParse::ParseShaderCode(code, vert, frag);
    bool hasVertex = ! vert.empty();
    bool hasFrag = ! frag.empty();
    if (!hasVertex && !hasFrag) {
        LOG_ERROR("Vertex and fragment shader not found in file: {}", path);
        throw std::runtime_error("Vertex and fragment shader not found in file: " + path);
    }
    if (! hasVertex) {
        LOG_WARNING("Vertex shader not found in file: {}", path);
    }
    if (! hasFrag) {
        LOG_WARNING("Fragment shader not found in file: {}", path);
    }
    std::string vulkanVertCode{} , vulkanFragCode{};
    std::filesystem::path vulkanShaderPath = std::filesystem::path(path).parent_path();
    vulkanShaderPath = vulkanShaderPath / std::filesystem::path(path).stem();
    shaderPass->shaderInfo = ShaderParse::GetShaderInfo(code);
    if (hasVertex) {
        vulkanVertCode = ShaderParse::TranslateToVulkan(vert, shaderPass->shaderInfo.vertProperties);
        std::ofstream vulkanVertShaderFile(vulkanShaderPath.string() + ".vert");
        if (! vulkanVertShaderFile.is_open()) {
            LOG_ERROR("Failed to open vertex shader file: {}", vulkanShaderPath.string());
            throw std::runtime_error("Failed to open fragment shader file:" +  vulkanShaderPath.string());
        }
        vulkanVertShaderFile << vulkanVertCode;
        vulkanVertShaderFile.close();
        CompileShader(vulkanShaderPath.string() + ".vert");
        shaderPass->vertShaderCode = std::move(LoadShaderSpv(vulkanShaderPath.string() + ".vert.spv", shaderPass->vertShaderSize));
    }
    if (hasFrag) {
        vulkanFragCode = ShaderParse::TranslateToVulkan(frag, shaderPass->shaderInfo.fragProperties);
        std::ofstream vulkanFragShaderFile(vulkanShaderPath.string() + ".frag");
        if (! vulkanFragShaderFile.is_open()) {
            LOG_ERROR("Failed to open fragment shader file: {}", vulkanShaderPath.string());
            throw std::runtime_error("Failed to open fragment shader file:" +  vulkanShaderPath.string());
        }
        vulkanFragShaderFile << vulkanFragCode;
        vulkanFragShaderFile.close();
        CompileShader(vulkanShaderPath.string() + ".frag");
        shaderPass->fragShaderCode = std::move(LoadShaderSpv(vulkanShaderPath.string() + ".frag.spv", shaderPass->fragShaderSize));
    }
}


uint32_t FrameWork::ResourceManager::LoadTextureAssetFromJSON(const std::string &path) {
    auto j = LoadJSONFromPath(path);
    //先加载JSON格式
    Asset_Impl::TextureAsset_Impl textureAsset_Impl;
    try {
        textureAsset_Impl = j.get<Asset_Impl::TextureAsset_Impl>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error( "Error :" + std::string(e.what()));
    }


    {   //检测内存是否已加载，加载后直接返回
        std::shared_lock lock(texturePathToIndexMutex);
        if (texturePathToIndex.contains(textureAsset_Impl.sourcePath)) {
            return texturePathToIndex[textureAsset_Impl.sourcePath];
        }
    }
    //提取bin
    auto binPath = textureAsset_Impl.binPath;
    if (!std::filesystem::exists(binPath)) {
        LOG_ERROR("Resource Manager does not exist : {}", binPath);
        throw std::runtime_error("Texture Path :" + path +
            " does not exist BIN PATH : !" + binPath);
    }
    unsigned char* textureData = nullptr;
    uint32_t totalSize{};
    switch (textureAsset_Impl.textureImport.textureFormat) {
        case TextureFormat::R8:
        case TextureFormat::R8G8B8A8:
            totalSize = textureAsset_Impl.width * textureAsset_Impl.height * textureAsset_Impl.numChannel;
            break;
        default:
            totalSize = textureAsset_Impl.width * textureAsset_Impl.height * textureAsset_Impl.numChannel * 2;
    }
    try {
        textureData = LoadTextureBin(binPath, totalSize);
    } catch (std::exception& e) {
        LOG_ERROR("Error : {}", std::string(e.what()));
        throw std::runtime_error("Error : " + std::string(e.what()));
    }

    TextureAsset textureAsset = {};

    textureAsset.name = textureAsset_Impl.name,
    textureAsset.sourcePath = textureAsset_Impl.sourcePath,
    textureAsset.width = textureAsset_Impl.width,
    textureAsset.height = textureAsset_Impl.height,
    textureAsset.numChannel = textureAsset_Impl.numChannel,
    textureAsset.textureImport = textureAsset_Impl.textureImport,
    textureAsset.data = textureData;

    auto index = AddAsset(std::make_shared<TextureAsset>(textureAsset));
    {
        std::scoped_lock lock(texturePathToIndexMutex);
        texturePathToIndex[textureAsset.sourcePath] = index; //进行记录
    }
    return index;
}

uint32_t FrameWork::ResourceManager::LoadMaterialAssetFromJSON(const std::string &path) {
    {
        std::shared_lock lock(materialPathToIndexMutex);
        if (materialPathToIndex.contains(path)) {
            return materialPathToIndex[path];
        }
    }
    auto j = LoadJSONFromPath(path);
    MaterialAsset materialAsset;
    try {
        materialAsset = j.get<MaterialAsset>();
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("Error : {}", std::string(e.what()));
        throw std::runtime_error( "Error :" + std::string(e.what()));
    }
    //Material 无bin，所以可以忽略

    //Load
    uint32_t index = AddAsset(std::make_shared<MaterialAsset>(std::move(materialAsset)));
    {
        std::scoped_lock lock(materialPathToIndexMutex);
        materialPathToIndex[materialAsset.sourcePath] = index;
    }
    return index;
}

uint32_t FrameWork::ResourceManager::LoadShaderAssetFromJSON(const std::string &path) {
    //From JSON则代表assetTable中已经有资源了

    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Can't open Shader Asset Json file : {}", path);
        throw std::runtime_error("Can't open Shader Asset Json file :" + path);
    }
    auto shaderAsset = nlohmann::json::parse(file).get<ShaderAsset>();
    if (assetCacheTable.contains(shaderAsset.sourcePath)) {
        std::shared_lock shaderPathLock(shaderPathToIndexMutex);
        return shaderPathToIndex[shaderAsset.sourcePath];
    }

    std::vector<std::future<void>> shaderPasseFutures;
    auto& threadPool = ThreadPool::GetInstance();
    for (auto& p : shaderAsset.passes) {
        //p : shaderTag , shaderSourcePath
        //加载ShaderPass
        shaderPasseFutures.push_back(
            threadPool.Enqueue([pp = p, this]() mutable {
                LoadShaderPassFromSource(pp.second);
            }));
    }

    //ShaderPass 加载
    for (auto& future : shaderPasseFutures) {
        future.wait();
    }

    auto index =  AddAsset(std::make_shared<ShaderAsset>(std::move(shaderAsset)));
    {
        std::scoped_lock lock(shaderPathToIndexMutex);
        shaderPathToIndex[shaderAsset.sourcePath] = index;
    }

    return index;
}

//自己创建的
std::string FrameWork::ResourceManager::CreateMaterialAsset(const std::string& name) {
    //创建引擎Asset，不是外部导入也不是嵌入
    std::string sourcePath = std::format("EngineAsset_Material/{}",name);
    std::string jsonPath = std::format("{}Materials/{}.json", assetCachePath, name); //地址
    if (std::filesystem::exists(jsonPath)) {
        LOG_WARNING("The Material name: {} has existed", name);
    }
    MaterialAsset materialAsset = {};
    materialAsset.name = name;
    materialAsset.sourcePath = sourcePath;

    nlohmann::json json = materialAsset;
    std::ofstream jsonFile(jsonPath);
    jsonFile << std::setw(4) << json;
    jsonFile.close();
    //因为刚创建为空
    {
        auto index  = AddAsset(std::make_shared<MaterialAsset>(materialAsset));
        std::scoped_lock lock(materialPathToIndexMutex);
        materialPathToIndex[materialAsset.sourcePath] = index;
    }
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[sourcePath] = jsonPath;
    }
    return sourcePath;
}

//自己创建
std::string FrameWork::ResourceManager::CreateShaderAsset(const std::string& name) {
    //创建引擎Asset，不是外部导入也不是嵌入
    std::string sourcePath = std::format("EngineAsset_Shader/{}",name);
    std::string jsonPath = std::format("{}Shaders/{}.json", assetCachePath, name); //地址
    LOG_DEBUG("Create Shader Asset: {}", jsonPath);
    if (std::filesystem::exists(jsonPath)) {
        LOG_WARNING("The Shader name: {} has existed", name);
    }
    ShaderAsset shaderAsset = {};
    shaderAsset.name = name;
    shaderAsset.sourcePath = sourcePath;

    nlohmann::json json = shaderAsset;
    std::ofstream jsonFile(jsonPath);
    jsonFile << std::setw(4) << json;
    jsonFile.close();
    //因为刚创建为空
    auto index  = AddAsset(std::make_shared<ShaderAsset>(shaderAsset));
    {
        std::scoped_lock lock(shaderPathToIndexMutex);
        shaderPathToIndex[sourcePath] = index;
    }
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[sourcePath] = jsonPath;
    }
    return sourcePath;
}

void FrameWork::ResourceManager::AddShaderPassToShaderAsset(const std::string &shaderPath, const std::string& shaderTag,
    const std::string &shaderPassSourcePath) {
    //先得到shaderAsset
    if (!assetCacheTable.contains(shaderPath) || !shaderPathToIndex.contains(shaderPath)) {
        LOG_ERROR("Can't find shaderPath : {}", shaderPath);
        throw std::runtime_error("Can't find shaderPath : {" + shaderPath + "}");
    }
    uint32_t shaderIndex = 0;
    {
        std::shared_lock shaderPathLock(shaderPathToIndexMutex);
        shaderIndex = shaderPathToIndex[shaderPath];
    }
    auto shaderAsset = GetAsset<ShaderAsset>(shaderIndex);
    bool hasContain = true;
    {
        std::shared_lock lock(assetCacheTableMutex);
        hasContain = assetCacheTable.contains(shaderPath) && hasContain;
    }
    {
        std::shared_lock lock(shaderPassPathToIndexMutex);
        hasContain = shaderPassPathToIndex.contains(shaderPath) && hasContain;
    }

    if (!hasContain) {
        LoadShaderPassFromSource(shaderPassSourcePath);
    }
    uint32_t shaderPassIndex = 0;
    {
        std::shared_lock shaderPassPathLock(shaderPassPathToIndexMutex);
        shaderPassIndex = shaderPassPathToIndex[shaderPassSourcePath];
    }
    //绑定
    auto shaderPass = GetAsset<ShaderPass>(shaderPassIndex);
    shaderPass->shaderTag = shaderTag;
    shaderAsset->passes[shaderPass->shaderTag] = shaderPassSourcePath;
}

void FrameWork::ResourceManager::SaveMaterialAssetToJSON(const MaterialAsset &materialAsset) {
    nlohmann::json json = materialAsset;
    std::string sourcePath = materialAsset.sourcePath;
    std::string jsonPath;
    {
        std::shared_lock lock(assetCacheTableMutex);
        if (assetCacheTable.contains(sourcePath)) {
            jsonPath = assetCacheTable[sourcePath];
        }
        else {
            LOG_ERROR("Can't find json path from sourcePath : {}", sourcePath);
            throw std::runtime_error("Can't find json path from sourcePath");
        }
    }
    std::ofstream jsonFile(jsonPath);
    jsonFile << std::setw(4) << json;
    jsonFile.close();
}


//加载ShaderPass
uint32_t FrameWork::ResourceManager::LoadShaderPassFromSource(const std::string &shaderPath, bool overlap) {
    if(!std::filesystem::exists(shaderPath)){
        LOG_ERROR("Shader file not found: {}", shaderPath);
        throw std::runtime_error("Shader file not found: " + shaderPath);
    }
    bool hasContain = false;
    {
        std::shared_lock readLock(assetCacheTableMutex);
        hasContain = assetCacheTable.contains(shaderPath);
    }
    if (!overlap) {
        if (hasContain) {
            std::shared_lock shaderPassLock(shaderPassPathToIndexMutex);
            if (shaderPassPathToIndex.contains(shaderPath)) {
                return shaderPassPathToIndex[shaderPath];
            }
        }
    }

    auto shaderHash = ComputeFileSHA256(shaderPath);
    auto fileTime = std::filesystem::last_write_time(shaderPath);
    auto shaderPass = std::make_shared<ShaderPass>();

    if(hasContain){
        if(std::filesystem::exists(assetCacheTable[shaderPath])){
            std::ifstream shaderPassJson(assetCacheTable[shaderPath]);
            auto shaderPass_Impl = nlohmann::json::parse(shaderPassJson).get<Asset_Impl::ShaderPass_Impl>();
            if(shaderPass_Impl.contentHash == shaderHash && shaderPass_Impl.fileTime == fileTime){
                return LoadShaderPassFromJSON(assetCacheTable[shaderPath]);
            }
        }
    }
    //不存在则创建
    CompileCaIShader(shaderPath, shaderPass);

    shaderPass->name = std::filesystem::path(shaderPath).stem().string();
    // shaderPass->shaderTag = shaderTag; //ShaderPass当Shader创建，填入ShaderPass时填入
    shaderPass->contentHash = shaderHash;
    shaderPass->fileTime = fileTime;
    shaderPass->sourcePath = shaderPath;

    //存储
    Asset_Impl::ShaderPass_Impl shaderPass_Impl = {};
    shaderPass_Impl.name = shaderPass->name;
    shaderPass_Impl.shaderTag = shaderPass->shaderTag;
    shaderPass_Impl.contentHash = shaderPass->contentHash;
    shaderPass_Impl.fileTime = shaderPass->fileTime;
    shaderPass_Impl.sourcePath = shaderPass->sourcePath;
    shaderPass_Impl.shaderInfo = shaderPass->shaderInfo;

    shaderPass_Impl.vertShaderSize = shaderPass->vertShaderSize;
    shaderPass_Impl.fragShaderSize = shaderPass->fragShaderSize;

    //存储Bin
    std::string jsonPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".json";
    std::string vertBinPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".vert.bin";
    std::string fragBinPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".frag.bin";
    shaderPass_Impl.vertBinPath = vertBinPath;
    shaderPass_Impl.fragBinPath = fragBinPath;
    SaveShaderCodeBin(vertBinPath, shaderPass->vertShaderCode.get(), shaderPass->vertShaderSize);
    SaveShaderCodeBin(fragBinPath, shaderPass->fragShaderCode.get(), shaderPass->fragShaderSize);

    //存储JSON
    nlohmann::json shaderJson = shaderPass_Impl;
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[shaderPath] = jsonPath;
    }
    std::ofstream jsonFile(jsonPath);
    jsonFile << std::setw(4) << shaderJson;
    jsonFile.close();
    auto index = AddAsset(shaderPass);
    {
        //将加载的ShaderPass存到内存
        std::scoped_lock lock(shaderPassPathToIndexMutex);
        shaderPassPathToIndex[shaderPath] = index;
    }
    return index;
}

uint32_t FrameWork::ResourceManager::LoadShaderPassFromJSON(const std::string &path) {
    auto j = LoadJSONFromPath(path);
    Asset_Impl::ShaderPass_Impl shaderPass_Impl = j.get<Asset_Impl::ShaderPass_Impl>();
    {
        std::shared_lock lock(shaderPassPathToIndexMutex);
        if (shaderPassPathToIndex.contains(shaderPass_Impl.sourcePath)) {
            return shaderPassPathToIndex[shaderPass_Impl.sourcePath];
        }
    }
    ShaderPass shaderPass = {};
    shaderPass.name = shaderPass_Impl.name;
    shaderPass.shaderTag = shaderPass_Impl.shaderTag;
    shaderPass.sourcePath = shaderPass_Impl.sourcePath;
    shaderPass.contentHash = shaderPass_Impl.contentHash;
    shaderPass.fileTime = shaderPass_Impl.fileTime;
    shaderPass.shaderInfo = shaderPass_Impl.shaderInfo;
    shaderPass.vertShaderSize = shaderPass_Impl.vertShaderSize;
    shaderPass.fragShaderSize = shaderPass_Impl.fragShaderSize;
    try {
        shaderPass.vertShaderCode =  LoadShaderCodeBin(shaderPass_Impl.vertBinPath, shaderPass_Impl.vertShaderSize);
        shaderPass.fragShaderCode = LoadShaderCodeBin(shaderPass_Impl.fragBinPath, shaderPass_Impl.fragShaderSize);
    } catch (const std::exception& e) {
        LOG_ERROR("Error : {}", std::string(e.what()));
        throw std::runtime_error("Error : " + std::string(e.what()));
    }
    //载入内存
    auto shaderPassPtr = std::make_shared<ShaderPass>(std::move(shaderPass));
    auto index = AddAsset(shaderPassPtr);
    {
        std::scoped_lock lock(shaderPassPathToIndexMutex);
        shaderPassPathToIndex[shaderPassPtr->sourcePath] = index; //从磁盘加载到内存
    }

    return index;
}


uint32_t FrameWork::ResourceManager::LoadModelAssetFromJSON(const std::string &path) {
    auto j = LoadJSONFromPath(path);
    ModelAsset modelAsset = j.get<ModelAsset>();

    //检测是否已经加载到内存
    {
        std::shared_lock lock(modelPathToIndexMutex);
        if (modelPathToIndex.contains(modelAsset.sourcePath)) {
            return modelPathToIndex[modelAsset.sourcePath];
        }
    }

    //加载节点,因为已经加载为json，不需要像处理源文件一样, 直接遍历节点即可
    // std::vector<std::future<void>> futures;
    for (auto& node : modelAsset.nodes) {
        for (auto& matPath : node.materials) {
            {
                std::string jsonPath;
                {
                    std::shared_lock lock(assetCacheTableMutex);
                    if (assetCacheTable.contains(matPath)) {
                        jsonPath = assetCacheTable[matPath];
                    }else {
                        LOG_ERROR("Can't find json path from {}", matPath);
                        throw std::runtime_error("Can't find json path from " + matPath);
                    }
                }
                LoadMaterialAssetFromJSON(jsonPath);
            }
        }
        for (auto& meshPath : node.meshes) {
            std::string jsonPath;
            {
                std::shared_lock lock(assetCacheTableMutex);
                if (assetCacheTable.contains(meshPath)) {
                    jsonPath = assetCacheTable[meshPath];
                }else {
                    LOG_ERROR("Can't find json path from {}", meshPath);
                    throw std::runtime_error("Can't find json path from " + meshPath);
                }
            }
            LoadMeshAssetFromJSON(jsonPath);
        }
    }
    auto index = AddAsset(std::make_shared<ModelAsset>(modelAsset));
    {
        std::scoped_lock lock(modelPathToIndexMutex);
        modelPathToIndex[modelAsset.sourcePath] = index;
    }
    return index;
}

uint32_t FrameWork::ResourceManager::LoadMeshAssetFromJSON(const std::string &path) {
    auto j = LoadJSONFromPath(path);
    auto meshAsset_Impl = j.get<Asset_Impl::MeshAsset_Impl>();
    {
        //检测是否已经加载 , 和Shader同理
        std::shared_lock lock(meshPathToIndexMutex);
        if (meshPathToIndex.contains(meshAsset_Impl.sourcePath)) {
            return meshPathToIndex[meshAsset_Impl.sourcePath];
        }
    }

    //取出bin文件
    MeshAsset meshAsset = {};
    meshAsset.name = meshAsset_Impl.name;
    meshAsset.contentHash = meshAsset_Impl.contentHash;
    meshAsset.sourcePath = meshAsset_Impl.sourcePath;
    meshAsset.fileTime = meshAsset_Impl.fileTime;
    LoadMeshBin(meshAsset, meshAsset_Impl.binPath);
    auto index = AddAsset(std::make_shared<MeshAsset>(std::move(meshAsset)));
    {
        std::scoped_lock lock(meshPathToIndexMutex);
        meshPathToIndex[meshAsset_Impl.sourcePath] = index; //Mesh Source 的地址是虚拟化的，类似与嵌入的纹理
    }
    return index;
}

std::string FrameWork::ResourceManager::LoadEmbeddingTexture(const aiTexture* texData, const std::string& modelPath, TextureImport* textureImport){
    std::string sourcePath = modelPath + "-Embedding/Texture" + texData->mFilename.C_Str();
    TextureAsset textureAsset = {};
    textureAsset.name = texData->mFilename.C_Str();
    textureAsset.sourcePath = sourcePath;
    textureAsset.contentHash = "Embedding";

    //压缩纹理
    if (texData->mHeight == 0 || texData->mWidth == 0) {
        textureAsset.data =
            stbi_load_from_memory((unsigned char*)texData->pcData, texData->mWidth,
                (int*)&textureAsset.width, (int*)&textureAsset.height, (int*)&textureAsset.numChannel, 4);
    }else {
        textureAsset.data = (unsigned char*)texData->pcData;
        textureAsset.width = texData->mWidth;
        textureAsset.height = texData->mHeight;
        textureAsset.numChannel = 4; //可能有问题
    }
    textureAsset.textureImport = *textureImport;

    std::string modelName = std::filesystem::path(modelPath).stem().string();
    std::string jsonPath  = assetCachePath + "Textures/" + modelName + "_Texture_" + textureAsset.name + ".json";
    std::string binPath  = assetCachePath + "Textures/" + modelName + "_Texture_" + textureAsset.name + ".bin";
    Asset_Impl::TextureAsset_Impl textureAsset_Impl = {};
    textureAsset_Impl.name = textureAsset.name;
    textureAsset_Impl.sourcePath = textureAsset.sourcePath;
    textureAsset_Impl.contentHash = textureAsset.contentHash;
    textureAsset_Impl.fileTime = textureAsset.fileTime;
    textureAsset_Impl.width = textureAsset.width;
    textureAsset_Impl.height = textureAsset.height;
    textureAsset_Impl.numChannel = textureAsset.numChannel;
    textureAsset_Impl.textureImport = textureAsset.textureImport;
    textureAsset_Impl.binPath = binPath;
    nlohmann::json json = textureAsset_Impl;
    std::ofstream out(jsonPath);
    out << std::setw(4) << json;
    out.close();
    uint32_t totalSize = textureAsset.width * textureAsset.height * textureAsset.numChannel;
    if (textureAsset.textureImport.textureFormat == TextureFormat::R16G16B16)
        totalSize *= 2;
    SaveTextureBin(binPath, textureAsset.data, totalSize);

    uint32_t index = AddAsset(std::make_shared<TextureAsset>(std::move(textureAsset)));
    {
        std::scoped_lock lock(texturePathToIndexMutex);
        texturePathToIndex[textureAsset.sourcePath] = index;
    }
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[textureAsset.sourcePath] = jsonPath;
    }

    return textureAsset.sourcePath;
}

std::string FrameWork::ResourceManager::LoadMaterialAssetFromModel(const aiScene* scene,aiMesh* mesh, const std::string& modelPath){
    //纹理
    auto* mat = scene->mMaterials[mesh->mMaterialIndex];
    std::string materialPath = CreateMaterialAsset(std::filesystem::path(modelPath).stem().string() +
        "_" + mesh->mName.C_Str() + "_Material");
    uint32_t index{};
    {
        std::shared_lock lock(materialPathToIndexMutex);
        index = materialPathToIndex[modelPath];
    }
    auto materialAsset = GetAsset<MaterialAsset>(index);
    auto modelName = std::filesystem::path(modelPath).stem().string();
    //嵌入不需要使用fileTime和contentHash
    // LoadShaderAssetFromSource(defaultShaderPath, false); //加载
    //创建这个模型的默认shader,1.创建一个shaderAsset 2.使用默认的shaderPass

    //加载shader
    std::string shaderName = std::filesystem::path(modelPath).stem().string() +
        "_" + mesh->mName.C_Str() + "_Shader";
    std::string shaderPath = CreateShaderAsset(shaderName);
    materialAsset->shaderPath = shaderPath;
    materialAsset->sourcePath = materialPath;
    try {
        AddShaderPassToShaderAsset(shaderPath, "default",defaultShaderPassPath);
    } catch (const std::exception& e) {
        LOG_ERROR("{}", e.what());
        throw e;
    }
    //便利所有情况
    for(auto& p : aiTextureTypeToString){
        auto nums = mat->GetTextureCount(p.first);
        for(int i = 0; i < nums; i++){
            aiString path;
            mat->GetTexture(p.first, i, &path);
            std::string texturePath = (std::filesystem::path(modelPath).parent_path() / std::filesystem::path(path.C_Str())).string();

            TextureImport textureImport{};
            switch (p.first) {
                case aiTextureType_METALNESS:
                case aiTextureType_AMBIENT_OCCLUSION:
                case aiTextureType_DIFFUSE_ROUGHNESS:
                case aiTextureType_HEIGHT:
                case aiTextureType_NORMALS:
                case aiTextureType_OPACITY:
                    textureImport.colorSpace = ColorSpace::LINEAR;
                    break;
                default:
                    textureImport.colorSpace = ColorSpace::SRGB;
                    break;
            }

            auto texData = scene->GetEmbeddedTexture(path.C_Str());
            if(texData != nullptr){
                //嵌入式纹理
                materialAsset->textures[p.second] = LoadEmbeddingTexture(texData, modelPath, &textureImport);
            }else{
                LoadTextureAssetFromSource(texturePath, false, &textureImport);
                materialAsset->textures[p.second] = texturePath;
            }
        }
    }

    //存储到磁盘，直写,后续再思考是否修改为写回
    SaveMaterialAssetToJSON(*materialAsset);

    return materialAsset->sourcePath;
}

std::string FrameWork::ResourceManager::LoadMeshAssetFromModel(aiMesh *mesh,
    const std::string & modelPath) {
    MeshAsset meshAsset = {};

    //name
    meshAsset.name = std::filesystem::path(modelPath).stem().string() +  "_" + mesh->mName.data + "_Mesh";

    //构建虚拟的Mesh Source
    //format : modelPath-Embedding/Meshes/MeshName
    std::string meshPath = modelPath + "-Embedding" + "/Meshes/" + meshAsset.name;
    meshAsset.sourcePath = meshPath;

    if (assetCacheTable.contains(meshAsset.sourcePath)) {
        LoadMeshAssetFromJSON(meshAsset.sourcePath); //返回值似乎无用-使用string作为索引
        return meshAsset.sourcePath;
    }

    //Vertex
    for (int i = 0; i < mesh->mNumVertices; i++) {
        VertexData vertex{};
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;

        vertex.tangent.x = mesh->mTangents[i].x;
        vertex.tangent.y = mesh->mTangents[i].y;
        vertex.tangent.z = mesh->mTangents[i].z;

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }else {
            LOG_WARNING("Can't Find Mesh texcoord ! in %s", mesh->mName.data);
        }
        meshAsset.vertices.push_back(std::move(vertex));
    }
    //Indices
    for (int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            meshAsset.indices.push_back(face.mIndices[j]);
        }
    }

    //因为Mesh完全依附于Model,所以此处先不使用contentHash 和 fileTime

    //format : modelName_Mesh_MeshName.json
    std::string modelName = std::filesystem::path(modelPath).stem().string();
    std::string jsonPath = assetCachePath + "Meshes/" + meshAsset.name + ".json";
    std::string binPath = assetCachePath + "Meshes/" + meshAsset.name + ".bin";
    SaveMeshBin(meshAsset, binPath);
    Asset_Impl::MeshAsset_Impl meshAssetImpl = {};
    meshAssetImpl.name = meshAsset.name,
    meshAssetImpl.contentHash = meshAsset.contentHash;
    meshAssetImpl.sourcePath = meshAsset.sourcePath;
    meshAssetImpl.fileTime = meshAsset.fileTime;
    meshAssetImpl.binPath =  binPath;
    nlohmann::json j = meshAssetImpl;
    std::ofstream of(jsonPath);
    of << std::setw(4) << j; //写入
    of.close();


    //写入内存
    std::string sourcePath = meshAsset.sourcePath;
    {
        auto index = AddAsset(std::make_shared<MeshAsset>(std::move(meshAsset)));
        std::scoped_lock lock(meshPathToIndexMutex);
        meshPathToIndex[meshAsset.sourcePath] = index;
    }
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[sourcePath] = jsonPath;
    }

    return sourcePath;
}

uint32_t FrameWork::ResourceManager::LoadModelAssetNode(std::shared_ptr<ModelAsset> modelAsset,const aiScene *scene, const aiNode *node,
                                                    const std::string & modelPath, uint32_t parent) {
    ModelNode modelNode{};
    modelNode.parentIndex = parent;
    std::vector<std::future<std::string>> meshFutures(node->mNumMeshes);
    std::vector<std::future<std::string>> materialFutures(node->mNumMeshes);
    auto& threadPool = ThreadPool::GetInstance();
    for (int i = 0; i < node->mNumMeshes; i++) {
        //加载Mesh
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshFutures[i] = (
            threadPool.Enqueue(
                [this](aiMesh* mesh, const std::string& modelPath) {
                    return LoadMeshAssetFromModel(mesh, modelPath);
                }, mesh, modelPath));

        materialFutures[i] = (
            threadPool.Enqueue(
                [this](const aiScene* scene, aiMesh* mesh,const std::string& modelPath) {
                    return LoadMaterialAssetFromModel(scene, mesh, modelPath);
                }, scene, mesh, modelPath));

        // modelNode.meshes.push_back(LoadMeshAssetFromModel(mesh, modelPath));
        // modelNode.materials.push_back(LoadMaterialAssetFromModel(scene, mesh, modelPath)); //加载Material
        modelNode.name += std::string(mesh->mName.C_Str()); //叠加起来
    }
    uint32_t index = modelAsset->nodes.size();
    modelNode.index = index;
    modelAsset->nodes.push_back(modelNode);

    for (int i = 0; i < node->mNumChildren; i++) {
        auto childIndex = LoadModelAssetNode(modelAsset, scene, node->mChildren[i], modelPath, index);
        modelAsset->nodes[index].childrenIndices.push_back(childIndex);
    }
    for (int i = 0; i < node->mNumMeshes; i++) {
        modelAsset->nodes[index].meshes.push_back(meshFutures[i].get());
        modelAsset->nodes[index].materials.push_back(materialFutures[i].get());
    }

    return index;
}


uint32_t FrameWork::ResourceManager::LoadTextureAssetFromSource(const std::string &path, bool overlap, TextureImport* textureImport) {
    TextureAsset textureAsset;
    textureAsset.name = std::filesystem::path(path).stem().string();
    textureAsset.fileTime = std::filesystem::last_write_time(path);
    textureAsset.contentHash = ComputeFileSHA256(path);
    if (!overlap) {
        std::shared_lock lock(textureAssetPoolMutex);
        if (texturePathToIndex.contains(path)) {
            return texturePathToIndex[path];
        }
    }
    bool hasContain = false;
    {
        std::shared_lock lock(assetCacheTableMutex);
        hasContain = assetCacheTable.contains(path);
    }
    if (hasContain) {
        if (std::filesystem::exists(assetCacheTable[path])) {
            std::ifstream textureAssetJson(assetCacheTable[path]);
            auto textureAsset_Impl = nlohmann::json::parse(textureAssetJson).get<Asset_Impl::TextureAsset_Impl>();
            if (textureAsset_Impl.fileTime == textureAsset.fileTime && textureAsset_Impl.contentHash == textureAsset.contentHash) {
                try {
                    return LoadTextureAssetFromJSON(assetCacheTable[path]);
                } catch (std::exception& e) {
                    LOG_ERROR("Error : {}", std::string(e.what()));
                    throw std::runtime_error("Error : " + std::string(e.what()));
                }
            }
        }
    }


    if (textureImport != nullptr) {
        textureAsset.textureImport = *textureImport; // 支持自定义textureImport
    }

    uint32_t totalSize{};
    if (std::filesystem::path(path).extension() == ".dds") {
        textureAsset.textureImport.colorSpace = ColorSpace::LINEAR;
        textureAsset.textureImport.textureFormat = TextureFormat::R16G16B16;
    }
    if (textureAsset.textureImport.textureFormat == TextureFormat::R16G16B16) {
        LoadDDSTextureAsset(path, textureAsset);
        totalSize = textureAsset.width * textureAsset.height * textureAsset.numChannel * 2;
    }else {
        LoadSTBTextureAsset(path, textureAsset);
        totalSize = textureAsset.width * textureAsset.height * textureAsset.numChannel;
    }

    std::string jsonPath = (std::filesystem::path(assetCachePath) / std::filesystem::path("Textures") /
        std::filesystem::path(std::filesystem::path(path).stem().string() + std::string(".json"))).string();
    std::string binPath = (std::filesystem::path(assetCachePath) / std::filesystem::path("Textures") /
        std::filesystem::path(std::filesystem::path(path).stem().string() + std::string(".bin"))).string();

    SaveTextureBin(binPath, textureAsset.data, totalSize);

    Asset_Impl::TextureAsset_Impl textureAsset_Impl = {};
    textureAsset_Impl.name = textureAsset.name;
    textureAsset_Impl.sourcePath = (std::filesystem::path(textureAsset.sourcePath)).string();
    textureAsset_Impl.contentHash = textureAsset.contentHash;
    textureAsset_Impl.fileTime = textureAsset.fileTime;
    textureAsset_Impl.width = textureAsset.width;
    textureAsset_Impl.height = textureAsset.height;
    textureAsset_Impl.numChannel = textureAsset.numChannel;
    textureAsset_Impl.textureImport = textureAsset.textureImport;
    textureAsset_Impl.binPath = binPath;

    std::ofstream jsonFile(jsonPath);
    nlohmann::json json = textureAsset_Impl;
    jsonFile << std::setw(4) << json;
    {
        std::shared_lock lock(assetCacheTableMutex);
        assetCacheTable[path] = jsonPath;
    }
    auto index = AddAsset(std::make_shared<TextureAsset>(textureAsset));
    {
        std::scoped_lock lock(texturePathToIndexMutex);
        texturePathToIndex[path] = index;
    }

    return index;
}

// uint32_t FrameWork::ResourceManager::LoadShaderAssetFromSource(const std::string &path, bool overlap) {
//     auto j = LoadJSONFromPath(path);
//     ShaderSource shaderSource;
//     try {
//         j.get_to<ShaderSource>(shaderSource);
//     }catch (const std::exception &e) {
//         LOG_ERROR("Failed to load shader source from source: {} , Becasue: {}", path, std::string(e.what()));
//         throw std::runtime_error("Failed to load shader source from source: " + path + "\nError: " + e.what());
//     }
//
//     if (!overlap) {
//         std::shared_lock readLock(shaderPathToIndexMutex);
//         if (shaderPathToIndex.contains(path)) {
//             return shaderPathToIndex[path];
//         }
//     }
//     //算hash
//
//     ShaderAsset shaderAsset;
//     shaderAsset.name = shaderSource.name;
//
//     std::vector<std::future<uint32_t>> passFutures;
//     auto& threadPool = ThreadPool::GetInstance();
//     for(auto &[shaderTag ,shaderPath] : shaderSource.passes){
//         passFutures.push_back( threadPool.Enqueue(
//             [this](const std::string& shaderTag, const std::string& shaderPath)-> uint32_t {
//                 if(!std::filesystem::exists(shaderPath)){
//                     LOG_ERROR("Shader file not found: {}", shaderPath);
//                     throw std::runtime_error("Shader file not found: " + shaderPath);
//                 }
//                 auto shaderHash = ComputeFileSHA256(shaderPath);
//                 auto fileTime = std::filesystem::last_write_time(shaderPath);
//                 auto shaderPass = std::make_shared<ShaderPass>();
//                 bool hasContain = false;
//                 {
//                     std::shared_lock readLock(assetCacheTableMutex);
//                     hasContain = assetCacheTable.contains(shaderPath);
//                 }
//                 if(hasContain){
//                     if(std::filesystem::exists(assetCacheTable[shaderPath])){
//                         std::ifstream shaderPassJson(assetCacheTable[shaderPath]);
//                         auto shaderPass_Impl = nlohmann::json::parse(shaderPassJson).get<Asset_Impl::ShaderPass_Impl>();
//                         if(shaderPass_Impl.contentHash == shaderHash && shaderPass_Impl.fileTime == fileTime){
//                             return LoadShaderPassFromJSON(assetCacheTable[shaderPath]);
//                         }
//                     }
//                 }
//                 //不存在则创建
//                 CompileCaIShader(shaderPath, shaderPass);
//
//                 shaderPass->name = std::filesystem::path(shaderPath).stem().string();
//                 shaderPass->shaderTag = shaderTag;
//                 shaderPass->contentHash = shaderHash;
//                 shaderPass->fileTime = fileTime;
//                 shaderPass->sourcePath = shaderPath;
//
//                 //存储
//                 Asset_Impl::ShaderPass_Impl shaderPass_Impl = {};
//                 shaderPass_Impl.name = shaderPass->name;
//                 shaderPass_Impl.shaderTag = shaderPass->shaderTag;
//                 shaderPass_Impl.contentHash = shaderPass->contentHash;
//                 shaderPass_Impl.fileTime = shaderPass->fileTime;
//                 shaderPass_Impl.sourcePath = shaderPass->sourcePath;
//                 shaderPass_Impl.shaderInfo = shaderPass->shaderInfo;
//
//                 shaderPass_Impl.vertShaderSize = shaderPass->vertShaderSize;
//                 shaderPass_Impl.fragShaderSize = shaderPass->fragShaderSize;
//
//                 //存储Bin
//                 std::string jsonPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".json";
//                 std::string vertBinPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".vert.bin";
//                 std::string fragBinPath = assetCachePath + "ShaderPasses/" + shaderPass_Impl.name + ".frag.bin";
//                 shaderPass_Impl.vertBinPath = vertBinPath;
//                 shaderPass_Impl.fragBinPath = fragBinPath;
//                 SaveShaderCodeBin(vertBinPath, shaderPass->vertShaderCode.get(), shaderPass->vertShaderSize);
//                 SaveShaderCodeBin(fragBinPath, shaderPass->fragShaderCode.get(), shaderPass->fragShaderSize);
//
//                 //存储JSON
//                 nlohmann::json shaderJson = shaderPass_Impl;
//                 {
//                     std::scoped_lock lock(assetCacheTableMutex);
//                     assetCacheTable[shaderPath] = jsonPath;
//                 }
//                 std::ofstream jsonFile(jsonPath);
//                 jsonFile << std::setw(4) << shaderJson;
//                 jsonFile.close();
//                 auto index = AddAsset(shaderPass);
//                 {
//                     //将加载的ShaderPass存到内存
//                     std::scoped_lock lock(shaderPassPathToIndexMutex);
//                     shaderPassPathToIndex[shaderPath] = index;
//                 }
//                 return index;
//             }, shaderTag, shaderPath
//             ));
//     }
//
//     for (auto& f : passFutures) {
//         auto shaderPass = shaderPassPool[f.get()];
//
//         shaderAsset.passes[shaderPass->shaderTag] = shaderPass->sourcePath; //sourcePath作为唯一标识,类似与GUID
//     }
//     auto index = AddAsset(std::make_shared<ShaderAsset>(shaderAsset));
//     {
//         std::scoped_lock lock(shaderPathToIndexMutex);
//         shaderPathToIndex[path] = index;
//     }
//
//     //Shader Asset 缓存
//     std::string jsonPath =  assetCachePath + "Shaders/" + shaderAsset.name + ".json";
//     nlohmann::json shaderJson = shaderAsset;
//     std::ofstream jsonFile(jsonPath);
//     jsonFile << std::setw(4) << shaderJson;
//     jsonFile.close();
//     {
//         std::scoped_lock lock(assetCacheTableMutex);
//         assetCacheTable[path] = jsonPath;
//     }
//
//     return index;
// }

//加载模型节点
//对应处理多种模型


uint32_t FrameWork::ResourceManager::LoadModelAssetFromSource(const std::string &path, bool overlap, ModelImport* modelImport) {
    //Model 比较复杂，可能需要加载嵌入式纹理，还要为Mesh和嵌入式纹理生成虚拟地址
    auto modelAsset = std::make_shared<ModelAsset>();
    modelAsset->name = std::filesystem::path(path).stem().string();
    modelAsset->fileTime = last_write_time(std::filesystem::path(path));
    modelAsset->contentHash = ComputeFileSHA256(path);
    modelAsset->sourcePath = path;

    if (modelImport != nullptr) {
        modelAsset->import = *modelImport;
    }else {
        modelAsset->import = ModelImport();
    }
    if (!overlap) {
        std::shared_lock lock(modelPathToIndexMutex);
        if (modelPathToIndex.contains(path)) {
            return modelPathToIndex[path];
        }
    }
    bool hasContain = false;

    //加载已有缓存
    std::string jsonPath{};
    {
        std::shared_lock lock(assetCacheTableMutex);
        hasContain = assetCacheTable.contains(path);
        jsonPath = assetCacheTable[path];
    }
    if (hasContain) {
        if (std::filesystem::exists(jsonPath)) {
            std::ifstream assetFile(jsonPath);
            if (!assetFile.is_open()) {
                LOG_ERROR("Failed to open asset file {}", jsonPath);
                throw std::runtime_error("Failed to open asset file");
            }
            nlohmann::json json = nlohmann::json::parse(assetFile);
            auto loadModelAsset = json.get<ModelAsset>();
            if (loadModelAsset.fileTime == modelAsset->fileTime && loadModelAsset.contentHash == modelAsset->contentHash) {
               return LoadModelAssetFromJSON(jsonPath);
            }
        }
    }

    //没有缓存直接加载
    Assimp::Importer importer;
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, modelAsset->import.scale);
    unsigned int loadFlags = 0;
    if (modelAsset->import.genNormal) {
        loadFlags |= aiProcess_GenNormals;
    }
    if (modelAsset->import.genTangent) {
        loadFlags |= aiProcess_CalcTangentSpace;
    }
    if (modelAsset->import.flipUV) {
        loadFlags |= aiProcess_FlipUVs;
    }
    const aiScene *scene = importer.ReadFile(
        path, loadFlags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("Error : {}", std::string(importer.GetErrorString()));
        throw std::runtime_error("Failed to load model from file " + path);
    }

    LoadModelAssetNode(modelAsset, scene, scene->mRootNode, path);

    jsonPath = assetCachePath + "Models/" + std::filesystem::path(path).stem().string() + ".json";
    nlohmann::json json = *modelAsset;
    std::ofstream jsonFile(jsonPath);
    jsonFile << std::setw(4) << json;
    jsonFile.close();

    //载入内存
    auto index = AddAsset(modelAsset);
    {
        std::scoped_lock lock(modelPathToIndexMutex);
        modelPathToIndex[path] = index;
    }
    {
        std::scoped_lock lock(assetCacheTableMutex);
        assetCacheTable[path] = jsonPath;
    }
    return index;
}


FrameWork::ShaderModulePackages FrameWork::ResourceManager::GetShaderCaIShaderModule(VkDevice device, const std::string &filePath,
                                                                                     ShaderInfo &shaderInfo) const {

    auto IfCompile = [](const std::filesystem::path &filepath1, const std::filesystem::file_time_type &time)-> bool {
        if (filepath1.string() == ".") {
            return false;
        }
        auto time1 = std::filesystem::last_write_time(filepath1);
        if (time1 == time) {
            return false;
        } else {
            return true;
        }
    };

    FrameWork::ShaderModulePackages shaderModules{};
    caiShaderTimeCache = LoadShaderCache(caiShaderTimeCachePath);
    std::ifstream testFile(filePath);
    bool ifCompile = true;
    if (! caiShaderTimeCache.empty()) {
        ifCompile = IfCompile(filePath, caiShaderTimeCache[filePath]);
    }
    if (!testFile.is_open()) {
        LOG_ERROR("Failed to open test file from: {}", filePath);
    }
    std::stringstream ss;
    ss << testFile.rdbuf();
    std::string code = ss.str();
    shaderInfo = ShaderParse::GetShaderInfo(code);
    std::string vert, frag;
    ShaderParse::ParseShaderCode(code, vert, frag);
    bool hasVertex = ! vert.empty();
    bool hasFrag = ! frag.empty();
    if (!hasVertex && !hasFrag) {
        LOG_ERROR("Vertex and fragment shader not found in file: {}", filePath);
        return {};
    }
    if (! hasVertex) {
        LOG_WARNING("Vertex shader not found in file: {}", filePath);
    }
    if (! hasFrag) {
        LOG_WARNING("Fragment shader not found in file: {}", filePath);
    }
    std::string vulkanVertCode{} , vulkanFragCode{};
    std::filesystem::path vulkanShaderPath = std::filesystem::path(filePath).parent_path();
    vulkanShaderPath = vulkanShaderPath / std::filesystem::path(filePath).stem();
    if (hasVertex) {
        if (ifCompile) {
            vulkanVertCode = ShaderParse::TranslateToVulkan(vert, shaderInfo.vertProperties);
            std::ofstream vulkanVertShaderFile(vulkanShaderPath.string() + ".vert");
            if (! vulkanVertShaderFile.is_open()) {
                LOG_ERROR("Failed to open vertex shader file: {}", vulkanShaderPath.string());
                return {};
            }
            vulkanVertShaderFile << vulkanVertCode;
            vulkanVertShaderFile.close();
            CompileShader(vulkanShaderPath.string() + ".vert");
            caiShaderTimeCache[filePath] = std::filesystem::last_write_time(filePath);
        }
        auto vertShaderModule = VulkanTool::loadShader(vulkanShaderPath.string() + ".vert.spv" , device);
        shaderModules.emplace_back(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);
    }
    if (hasFrag) {
        if (ifCompile) {
            vulkanFragCode = ShaderParse::TranslateToVulkan(frag, shaderInfo.fragProperties);
            std::ofstream vulkanFragShaderFile(vulkanShaderPath.string() + ".frag");
            if (! vulkanFragShaderFile.is_open()) {
                LOG_ERROR("Failed to open fragment shader file: {}", vulkanShaderPath.string());
                return {};
            }
            vulkanFragShaderFile << vulkanFragCode;
            vulkanFragShaderFile.close();
            CompileShader(vulkanShaderPath.string() + ".frag");
            caiShaderTimeCache[filePath] = std::filesystem::last_write_time(filePath);
        }
        auto fragShaderModule = VulkanTool::loadShader(vulkanShaderPath.string() + ".frag.spv" , device);
        shaderModules.emplace_back(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);
    }
    if (ifCompile) {
        SaveCache(caiShaderTimeCachePath, caiShaderTimeCache);
        //测试会因为地址原因报错
    }

    return shaderModules;
}

FrameWork::ShaderInfo FrameWork::ResourceManager::GetShaderInfo(const std::string &filePath) const {
    std::ifstream testFile(filePath);
    if (!testFile.is_open()) {
        LOG_ERROR("Failed to open CaIShaderFile file from: {}", filePath);
    }
    std::stringstream ss;
    ss << testFile.rdbuf();
    std::string code = ss.str();
    auto shaderInfo = ShaderParse::GetShaderInfo(code);
    std::string vert, frag;
    ShaderParse::ParseShaderCode(code, vert, frag);
    return shaderInfo;
}

FrameWork::ShaderModulePackages FrameWork::ResourceManager::GetCompShaderModule(VkDevice device, const std::string &filePath,
                                                                         CompShaderInfo &compShaderInfo) const {
    auto IfCompile = [](const std::filesystem::path &filepath1, const std::filesystem::file_time_type &time)-> bool {
        if (filepath1.string() == ".") {
            return false;
        }
        std::filesystem::file_time_type time1;
        try {
            time1 = std::filesystem::last_write_time(filepath1);
        }catch (std::exception &e) {
            LOG_ERROR("Exception in GetCompShaderModule: {}", e.what());
        }
        if (time1 == time) {
            return false;
        } else {
            return true;
        }
    };
    FrameWork::ShaderModulePackages shaderModules{};
    std::ifstream testFile(filePath);
    bool ifCompile = true;
    {
        std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
        caiShaderTimeCache = LoadShaderCache(caiShaderTimeCachePath);
        if (! caiShaderTimeCache.empty()) {
            ifCompile = IfCompile(filePath, caiShaderTimeCache[filePath]);
        }
        if (!testFile.is_open()) {
            LOG_ERROR("Failed to open test file from: {}", filePath);
        }
    }
    std::stringstream ss;
    ss << testFile.rdbuf();
    std::string code = ss.str();
    compShaderInfo = ShaderParse::GetCompShaderInfo(code);

    std::string vulkanCode{};
    std::filesystem::path vulkanShaderPath = std::filesystem::path(filePath).parent_path();
    vulkanShaderPath = vulkanShaderPath / std::filesystem::path(filePath).stem();
    if (ifCompile) {
        vulkanCode = ShaderParse::TranslateCompToVulkan(code, compShaderInfo);
        std::ofstream vulkanVertShaderFile(vulkanShaderPath.string() + ".comp");
        if (! vulkanVertShaderFile.is_open()) {
            LOG_ERROR("Failed to open vertex shader file: {}", vulkanShaderPath.string());
            return {};
        }
        vulkanVertShaderFile << vulkanCode;
        vulkanVertShaderFile.close();
        CompileShader(vulkanShaderPath.string() + ".comp");
        std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
        caiShaderTimeCache[filePath] = std::filesystem::last_write_time(filePath);
    }
    auto  compShaderModule = VulkanTool::loadShader(vulkanShaderPath.string() + ".comp.spv" , device);
    shaderModules.emplace_back(VK_SHADER_STAGE_COMPUTE_BIT, compShaderModule);

    if (ifCompile) {
        std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
        SaveCache(caiShaderTimeCachePath, caiShaderTimeCache);
    }

    return shaderModules;
}

void FrameWork::ResourceManager::ReleaseTextureFullData(const TextureFullData &textureFullData) {
    //主要是释放图像的指针指向的数据
    for (auto &it: textureMap) {
        if (it.second.path == textureFullData.path) {
            stbi_image_free(it.second.data);
            textureMap.erase(it.first);
            return;
        }
    }
}


std::future<FrameWork::ShaderModulePackages> FrameWork::ResourceManager::AsyncGetShaderCaIShaderModule(VkDevice device,
    const std::string &filePath) const {
    auto Func = [this](VkDevice device, const std::string& filePath)->ShaderModulePackages {
        ShaderModulePackages shaderModulePackages;
        ShaderInfo shaderInfo{};
        auto IfCompile = [](const std::filesystem::path &filepath1, const std::filesystem::file_time_type &time)-> bool {
            if (filepath1.string() == ".") {
                return false;
            }
            auto time1 = std::filesystem::last_write_time(filepath1);
            if (time1 == time) {
                return false;
            } else {
                return true;
            }
        };

        auto& shaderModules = shaderModulePackages;
        std::ifstream testFile(filePath);
        bool ifCompile = true;
        {
            std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
            caiShaderTimeCache = LoadShaderCache(caiShaderTimeCachePath);
            if (! caiShaderTimeCache.empty()) {
                ifCompile = IfCompile(filePath, caiShaderTimeCache[filePath]);
            }
        }
        if (!testFile.is_open()) {
            LOG_ERROR("Failed to open test file from: {}", filePath);
        }
        std::stringstream ss;
        ss << testFile.rdbuf();
        std::string code = ss.str();
        shaderInfo = ShaderParse::GetShaderInfo(code);
        std::string vert, frag;
        ShaderParse::ParseShaderCode(code, vert, frag);
        bool hasVertex = ! vert.empty();
        bool hasFrag = ! frag.empty();
        if (!hasVertex && !hasFrag) {
            LOG_ERROR("Vertex and fragment shader not found in file: {}", filePath);
            return {};
        }
        if (! hasVertex) {
            LOG_WARNING("Vertex shader not found in file: {}", filePath);
        }
        if (! hasFrag) {
            LOG_WARNING("Fragment shader not found in file: {}", filePath);
        }
        std::string vulkanVertCode{} , vulkanFragCode{};
        std::filesystem::path vulkanShaderPath = std::filesystem::path(filePath).parent_path();
        vulkanShaderPath = vulkanShaderPath / std::filesystem::path(filePath).stem();
        if (hasVertex) {
            if (ifCompile) {
                vulkanVertCode = ShaderParse::TranslateToVulkan(vert, shaderInfo.vertProperties);
                std::ofstream vulkanVertShaderFile(vulkanShaderPath.string() + ".vert");
                if (! vulkanVertShaderFile.is_open()) {
                    LOG_ERROR("Failed to open vertex shader file: {}", vulkanShaderPath.string());
                    return {};
                }
                vulkanVertShaderFile << vulkanVertCode;
                vulkanVertShaderFile.close();
                CompileShader(vulkanShaderPath.string() + ".vert");
                caiShaderTimeCache[filePath] = std::filesystem::last_write_time(filePath);
            }
            auto vertShaderModule = VulkanTool::loadShader(vulkanShaderPath.string() + ".vert.spv" , device);
            shaderModules.emplace_back(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);
        }
        if (hasFrag) {
            if (ifCompile) {
                vulkanFragCode = ShaderParse::TranslateToVulkan(frag, shaderInfo.fragProperties);
                std::ofstream vulkanFragShaderFile(vulkanShaderPath.string() + ".frag");
                if (! vulkanFragShaderFile.is_open()) {
                    LOG_ERROR("Failed to open fragment shader file: {}", vulkanShaderPath.string());
                    return {};
                }
                vulkanFragShaderFile << vulkanFragCode;
                vulkanFragShaderFile.close();
                CompileShader(vulkanShaderPath.string() + ".frag");
                std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
                caiShaderTimeCache[filePath] = std::filesystem::last_write_time(filePath);
            }
            auto fragShaderModule = VulkanTool::loadShader(vulkanShaderPath.string() + ".frag.spv" , device);
            shaderModules.emplace_back(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule);
        }
        if (ifCompile) {
            std::lock_guard<std::mutex> lock(this->caiShaderTimeCacheMutex);
            SaveCache(caiShaderTimeCachePath, caiShaderTimeCache);
            //测试会因为地址原因报错
        }
        return shaderModulePackages;
    };
    return ThreadPool::GetInstance().Enqueue(Func, device, filePath);
}

std::future<FrameWork::ShaderInfo> FrameWork::ResourceManager::AsyncGetShaderInfo(VkDevice device,
    const std::string &filePath) const {
    auto Func = [](VkDevice device, const std::string &filePath)->ShaderInfo {
        std::ifstream testFile(filePath);
        if (!testFile.is_open()) {
            LOG_ERROR("Failed to open test file from: {}", filePath);
        }
        std::stringstream ss;
        ss << testFile.rdbuf();
        std::string code = ss.str();
        auto shaderInfo = ShaderParse::GetShaderInfo(code);
        std::string vert, frag;
        ShaderParse::ParseShaderCode(code, vert, frag);
        return shaderInfo;
    };
    return ThreadPool::GetInstance().Enqueue(Func, device, filePath);
}

std::future<ExpectWithStr<std::unique_ptr<FrameWork::PrefabStruct>>> FrameWork::ResourceManager::AsyncLoadPrefabStruct(
    const std::string &filePath) const {
    return ThreadPool::GetInstance().Enqueue(
        [this](const std::string & filePath)->ExpectWithStr<std::unique_ptr<PrefabStruct>> {
            return this->LoadPrefabStruct(filePath);
        }, filePath);
}

FrameWork::ResourceManager &FrameWork::ResourceManager::GetInstance() {
    static ResourceManager instance;
    return instance;
}
