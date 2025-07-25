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
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


void FrameWork::Resource::processNode(aiNode *node, const aiScene *scene, std::vector<MeshData>& meshes, std::string directory) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, directory));
    }
    //接下来重复子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes, directory);
    }
}

FrameWork::MeshData FrameWork::Resource::processMesh(aiMesh *mesh, const aiScene *scene, std::string directory) {
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
            throw std::runtime_error("Warning : Don't have texCoord");
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
    auto diffuseMap = LoadTextureFullDatas(material, aiTextureType_DIFFUSE, directory);
    meshData.texData.insert(meshData.texData.end(), diffuseMap.begin(), diffuseMap.end());

    return meshData;
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
        shaderModule = VulkanTool::loadShader(shaderPath, device);
    }catch(const std::ios_base::failure &e) {
        std::cerr << e.what() << std::endl;
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

//这里默认这里的Obj不支持PBR贴图，等待后续扩展
std::vector<FrameWork::MeshData> FrameWork::Resource::LoadOBJMesh(const std::string &fileName) {
    Assimp::Importer importer;
    //使用惰性求值,处理fileName可能的两种输入情况
    std::vector<std::string_view> fsplits;
    for (const auto& range : std::views::split(fileName, '.')) {
        fsplits.emplace_back(range);
    }
    std::string path;
    std::string directory;
    for (const auto& range : fsplits) {
        if (range == ".obj") {
            path = generalModelPath + fileName.substr(0,fileName.size() - 4) + "/" + fileName;
            directory = generalModelPath + fileName.substr(0,fileName.size() - 4) + "/";
        }
    }
    if (path.empty()) {
        path = generalModelPath + fileName + "/" + fileName + ".obj";
        directory = generalModelPath + fileName + "/";
    }
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals |aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        throw std::runtime_error("Failed to load model from file " + fileName);
    }
    std::vector<MeshData> meshes;//返回值
    processNode(scene->mRootNode, scene, meshes, directory);
    return meshes;
}

std::vector<FrameWork::TextureFullData> FrameWork::Resource::LoadTextureFullDatas(aiMaterial *mat, aiTextureType type,
    std::string directory) {
    std::vector<TextureFullData> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        TextureFullData texData;
        if (type == aiTextureType_DIFFUSE) {
            texData = LoadTextureFullData(directory + str.C_Str(), TextureType::DiffuseColor);
        }
        else if (type == aiTextureType_SPECULAR) {
            texData = LoadTextureFullData(directory + str.C_Str(), TextureType::SpecColor);
        }else {
            std::cerr << "this type process has not complete !" << std::endl;
        }
        textures.push_back(texData);
    }
    return textures;
}

FrameWork::TextureFullData FrameWork::Resource::LoadTextureFullData(const std::string &filePath, TextureType type) {
    if (textureMap.find(filePath) != textureMap.end()) {
        return textureMap[filePath];
    }
    int width, height, numChannels;
    uint32_t desireChannels = 0;
    switch (type) {
        case TextureType::DiffuseColor:
            desireChannels = 4;
            break;
        case TextureType::SpecColor:
            desireChannels = 4;
            break;
        case TextureType::Normal:
            desireChannels = 4;
            break;
        default:
            desireChannels = 0;
    }
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &numChannels, desireChannels);
    TextureFullData texData;
    texData.width = width;
    texData.height = height;
    texData.numChannels = desireChannels;
    texData.data = data;
    texData.path = filePath;
    texData.type = type;
    if (!data) {
        std::cerr << "Failed to load texture from file " << filePath << std::endl;
        unsigned char* pixels = new unsigned char[width * height * numChannels];
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 1;//全部置为1
        }
        texData.data = pixels;
    }
    textureMap[filePath] = texData;

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
