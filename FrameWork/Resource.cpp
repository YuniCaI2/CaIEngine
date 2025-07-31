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
            pixels[i * numChannels] = 0;
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
std::vector<FrameWork::MeshData> FrameWork::Resource::LoadMesh(const std::string &fileName, ModelType modelType, TextureTypeFlags textureFlags) {
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
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals |aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
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
    int width = 100, height = 100, numChannels;
    uint32_t desireChannels = 4;
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
        if (type == TextureTypeFlagBits::DiffuseColor) {
            for (uint32_t i = 0; i < width * height * numChannels; i++) {
                pixels[i] = 1;//全部置为1
            }
        }
        if (type == TextureTypeFlagBits::MetallicRoughness) {
            for (uint32_t i = 0; i < width * height; i++) {
                pixels[i * numChannels + 0] = 0;
                pixels[i * numChannels + 1] = 0.5;
                pixels[i * numChannels + 2] = 0;
                pixels[i * numChannels + 3] = 1;
            }
        }
        if (type == TextureTypeFlagBits::Emissive) {
            for (uint32_t i = 0; i < width * height * numChannels; i++) {
                pixels[i * numChannels + 0] = 0;
            }
        }
        if (type == TextureTypeFlagBits::Occlusion) {
            for (uint32_t i = 0; i < width * height * numChannels; i++) {
                pixels[i * numChannels + 0] = 1;
            }
        }
        for (uint32_t i = 0; i < width * height * numChannels; i++) {
            pixels[i] = 1;//全部置为1
        }
        texData.data = pixels;
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
