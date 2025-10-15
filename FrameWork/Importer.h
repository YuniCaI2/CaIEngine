//
// Created by 51092 on 2025/10/13.
//

#ifndef CAIENGINE_IMPORTER_H
#define CAIENGINE_IMPORTER_H
#include "PublicEnum.h"
#include "Schema.h"
#include <shared_mutex>

namespace FrameWork {
    class Importer {
    public:
        //Import 错误代码
        enum class ErrorCode {
            FileNotFound,
            InvalidFormat,
            UnsupportedType,
            OutDated,
            UnknownError
        };
        //Instance
        static Importer& GetInstance();

        //Asset
        ExpectedWithInfo<GUID, ErrorCode> LoadTextureSource(const std::string& path);
        ExpectedWithInfo<TextureAsset, ErrorCode> LoadTextureAsset(const std::string& path);
        ExpectedWithInfo<bool, ErrorCode> SaveTextureAsset(const std::string& path, TextureAsset& textureAsset);

        ~Importer();
    private:
        Importer();
        void LoadGUID();
        uint32_t maxVersion = 0;
        uint32_t minVersion = 0;

        std::string AssetPath{"../../Asset/"};
        std::string guidPath{"../../Asset/globalGUID.bin"};
        std::string guidToAssetTablePath{"../../Asset/tableAssetGUID.bin"};
        std::string guidToMetaTablePath{"../../Asset/tableMetaGUID.bin"};

        std::shared_mutex guidToAssetTableMutex;
        std::shared_mutex guidToMetaTableMutex;
        std::vector<std::string> guidToAssetTable;
        std::vector<std::string> guidToMetaTable;
        GUID globalGUID{};
    };
}


#endif //CAIENGINE_IMPORTER_H
