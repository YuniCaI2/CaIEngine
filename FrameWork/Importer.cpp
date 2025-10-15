//
// Created by 51092 on 2025/10/13.
//

#include "Importer.h"
#include "PublicEnum.h"
#include "ResourceManager.h"
#include <__expected/unexpected.h>
#include <openssl/sha.h>
#include <sstream>
#include<filesystem>
#include<fstream>
#include <stb_image.h>

namespace FrameWork {
    Importer& Importer::GetInstance(){
        static Importer instance;
        return instance;
    }

    void LoadSTBTexture(const std::string& path, TextureAsset& texData) {
            int width = 100, height = 100, numChannels;
            uint32_t desireChannels = 4;
            unsigned char *data = nullptr;

            data = stbi_load(path.c_str(), &width, &height, &numChannels, desireChannels);

            texData.width = width;
            texData.height = height;
            texData.numChannel = desireChannels;

            if (!data) {
                std::cerr << "Failed to load texture from file, may be the directory was wrong " << std::endl;
                exit(-1);
            }else {
                texData.data = std::vector<uint8_t>(data, data + width * height * texData.numChannel);
            }
        }

    void LoadDDSTexture(const std::string& filePath, TextureAsset& textureAsset) {
        #ifdef _WIN32
            textureAsset = TextureAsset();
            using namespace DirectX;

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
            textureAsset.data = std::vector<uint8_t>(data, data + width * height * texData.numChannel);
        #endif
    }

    void HashTextureImportSettings(SHA256_CTX& ctx, const TextureImport& importSettings) {
        SHA256_Update(&ctx, &importSettings.texDim, sizeof(importSettings.texDim));
        SHA256_Update(&ctx, &importSettings.textureFormat, sizeof(importSettings.textureFormat));
        SHA256_Update(&ctx, &importSettings.colorSpace, sizeof(importSettings.colorSpace));
        SHA256_Update(&ctx, &importSettings.generateMipmap, sizeof(importSettings.generateMipmap));

        SHA256_Update(&ctx, &importSettings.textureSampler, sizeof(importSettings.textureSampler));
    }

    // 计算完整的纹理内容哈希（源文件 + 导入设置）
    void ComputeTextureContentHash(const TextureMeta& textureMeta, std::string& outHash) {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        // 1. 源文件内容
        std::ifstream file(textureMeta.source, std::ios::binary);
        if (file) {
            constexpr size_t BUFFER_SIZE = 8192;
            char buffer[BUFFER_SIZE];
            while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
                SHA256_Update(&ctx, buffer, file.gcount());
            }
        } else {
            // 如果源文件不存在，使用源路径字符串
            SHA256_Update(&ctx, textureMeta.source.c_str(), textureMeta.source.size());
        }

        // 2. 导入器版本
        SHA256_Update(&ctx, &textureMeta.importerVersion, sizeof(textureMeta.importerVersion));

        // 3. 资源类型
        SHA256_Update(&ctx, &textureMeta.type, sizeof(textureMeta.type));

        // 4. TextureImport 设置
        HashTextureImportSettings(ctx, textureMeta.textureImport);

        // 计算最终哈希
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &ctx);

        // 转换为十六进制字符串
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        outHash = ss.str();
    }



    ExpectedWithInfo<GUID, Importer::ErrorCode> Importer::LoadTextureSource(const std::string& path){
        std::string metaPath = path + ".meta";
        std::filesystem::path filePath(path);
        std::filesystem::path assetsPath(AssetPath);
        std::filesystem::path texturePath("textures/");
        std::string binPath = (assetsPath / texturePath / filePath.filename()).string() + ".bin";
        if(!std::filesystem::exists(path)){
            return UnexpectedWithInfo<Importer::ErrorCode>({"File not found in" + path, Importer::ErrorCode::FileNotFound});
        }
        bool reload = false;

        TextureMeta textureMeta;
        if(std::filesystem::exists(metaPath)){
            //得到hash,进行校验
            std::ifstream file(metaPath);
            nlohmann::json metaJson = nlohmann::json::parse(file);
            textureMeta = metaJson.get<TextureMeta>();

            if(textureMeta.importerVersion < minVersion || textureMeta.importerVersion > maxVersion){
                return UnexpectedWithInfo<Importer::ErrorCode>
                ({"File OutDated" + path, Importer::ErrorCode::OutDated});
            }
            textureMeta.name = std::filesystem::path(metaPath).filename().string();
            textureMeta.source = path;
            textureMeta.type = AssetType::Texture;
            std::string hash;
            ComputeTextureContentHash(textureMeta, hash);
            if(hash != textureMeta.contextHash){
                reload = true;
                textureMeta.contextHash = hash;
            }
        }else{
            //创建TextureMeta
            textureMeta.id = globalGUID++;
            //存储信息到表
            {
                std::scoped_lock g(guidToMetaTableMutex);
                if (textureMeta.id >= guidToMetaTable.size()) {
                    guidToMetaTable.resize(textureMeta.id + 1);
                }
                guidToMetaTable[textureMeta.id] = metaPath;
            }
            {
                std::scoped_lock g(guidToAssetTableMutex);
                if (textureMeta.id >= guidToAssetTable.size()) {
                    guidToAssetTable.resize(textureMeta.id + 1);
                }
                guidToAssetTable[textureMeta.id] = binPath;
            }
            textureMeta.name = std::filesystem::path(metaPath).filename().string();
            textureMeta.importerVersion = maxVersion;
            textureMeta.source = path;
            textureMeta.type = AssetType::Texture;
            std::string hash;
            ComputeTextureContentHash(textureMeta, hash);
            if(hash != textureMeta.contextHash){
                reload = true;
                textureMeta.contextHash = hash;
            }
            reload = true;
        }

        //重新加载bin和meta
        TextureAsset textureAsset;
        if(reload ||! std::filesystem::exists(binPath)){
            std::filesystem::path path_sys(path);
            if (path_sys.extension().c_str() == ".dds") {
                LoadDDSTexture(path, textureAsset);
            }else {
                LoadSTBTexture(path, textureAsset);
            }
            if (textureAsset.data.empty()) {
                return UnexpectedWithInfo<Importer::ErrorCode>
                ({"File Can't load" + path, Importer::ErrorCode::UnsupportedType});
            }
        }else {
            return textureMeta.id;
        }

        std::ofstream oMeta(metaPath);
        if (!oMeta) {
            return UnexpectedWithInfo<ErrorCode>(
                {"Can't save texture meta to file" + path, Importer::ErrorCode::FileNotFound}
                );
        }
        nlohmann::json json = textureMeta;
        oMeta << json;
        oMeta.close();

        auto res = SaveTextureAsset(binPath, textureAsset);
        if (res.has_value()) {
            return textureMeta.id;
        }else {
            UnexpectedWithInfo<ErrorCode>(res.error());
        }
    }

    ExpectedWithInfo<TextureAsset, Importer::ErrorCode> Importer::LoadTextureAsset(const std::string & filename) {
        std::ifstream ifs(filename, std::ios::binary);
        TextureAsset asset;
        if (!ifs) return UnexpectedWithInfo<Importer::ErrorCode>({
            "Can't load texture asset file :" + filename, Importer::ErrorCode::FileNotFound
        });

        // 1. 读取 GUID
        ifs.read(reinterpret_cast<char*>(&asset.id), sizeof(GUID));

        // 2. 读取 name
        uint32_t nameLen;
        ifs.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        asset.name.resize(nameLen);
        ifs.read(&asset.name[0], nameLen);

        // 3. 读取基本类型
        ifs.read(reinterpret_cast<char*>(&asset.width), sizeof(asset.width));
        ifs.read(reinterpret_cast<char*>(&asset.height), sizeof(asset.height));
        ifs.read(reinterpret_cast<char*>(&asset.numChannel), sizeof(asset.numChannel));

        // 4. 读取 data
        uint32_t dataSize;
        ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        if (dataSize > 0) {
            asset.data.resize(dataSize);
            ifs.read(reinterpret_cast<char*>(asset.data.data()), dataSize);
        }

        ifs.close();
        return asset;
    }

    ExpectedWithInfo<bool, Importer::ErrorCode> Importer::SaveTextureAsset(const std::string &filename, TextureAsset &asset) {
        std::ofstream ofs(filename, std::ios::binary);

        ofs.write(reinterpret_cast<const char*>(&asset.id), sizeof(GUID));
        if (!ofs) {
            return UnexpectedWithInfo<Importer::ErrorCode>
            ({"Can't save tex asset in " + filename, ErrorCode::FileNotFound});
        }

        // 2. 写入 name（先写长度，再写内容）
        uint32_t nameLen = asset.name.size();
        ofs.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        ofs.write(asset.name.c_str(), nameLen);

        // 3. 写入基本类型
        ofs.write(reinterpret_cast<const char*>(&asset.width), sizeof(asset.width));
        ofs.write(reinterpret_cast<const char*>(&asset.height), sizeof(asset.height));
        ofs.write(reinterpret_cast<const char*>(&asset.numChannel), sizeof(asset.numChannel));

        // 4. 写入 data（先写大小，再写内容）
        uint32_t dataSize = asset.data.size();
        ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        if (dataSize > 0) {
            ofs.write(reinterpret_cast<const char*>(asset.data.data()), dataSize);
        }

        ofs.close();

        return true;
    }

    Importer::~Importer() {
        std::ofstream assertTable(guidToAssetTablePath, std::ios::binary);
        std::ofstream metaTable(guidToMetaTablePath, std::ios::binary);
        for (int i = 0; i < guidToAssetTable.size(); ++i) {
            uint32_t pathLen = guidToAssetTable[i].length();
            assertTable.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
            assertTable.write(guidToAssetTable[i].c_str(), pathLen);
            pathLen = guidToMetaTable[i].length();
            metaTable.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
            metaTable.write(guidToMetaTable[i].c_str(), pathLen);
        }
        assertTable.close();
        metaTable.close();


        std::ofstream file(guidPath);
        file.write(reinterpret_cast<char*>(&globalGUID), sizeof(GUID));
        file.close();
    }

    Importer::Importer() {
        //无表则创建
        if (! std::filesystem::exists(guidToAssetTablePath)) {
            std::ofstream assetTable(guidToAssetTablePath, std::ios::binary);
            assetTable.close();
        }else {
            std::ifstream assetTable(guidToAssetTablePath, std::ios::binary);
            uint32_t pathLen;
            while (assetTable.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) {
                if (assetTable.eof()) {
                    break;
                }
                guidToAssetTable.emplace_back(pathLen, '\0');
                assetTable.read(&guidToAssetTable.back()[0], pathLen);
            }
        }
        if (! std::filesystem::exists(guidToMetaTablePath)) {
            std::ofstream metaTable(guidToMetaTablePath, std::ios::binary);
            metaTable.close();
        }else {
            std::ifstream assetTable(guidToMetaTablePath, std::ios::binary);
            uint32_t pathLen;
            while (assetTable.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) {
                if (assetTable.eof()) {
                    break;
                }
                guidToMetaTable.emplace_back(pathLen, '\0');
                assetTable.read(&guidToMetaTable.back()[0], pathLen);
            }
        }
        LoadGUID();
    }

    void Importer::LoadGUID() {
        std::ifstream file(guidPath, std::ios::binary);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(&globalGUID), sizeof(GUID));
            file.close();
        }else {
            LOG_WARNING("Importer::LoadGUID(): Failed to open guid file");
            file.close();
            std::ofstream ofile(guidPath, std::ios::binary);
            ofile.write(reinterpret_cast<char*>(&globalGUID), sizeof(GUID));
            ofile.close();
        }
    }
}
