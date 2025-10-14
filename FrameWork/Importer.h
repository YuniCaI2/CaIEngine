//
// Created by 51092 on 2025/10/13.
//

#ifndef CAIENGINE_IMPORTER_H
#define CAIENGINE_IMPORTER_H
#include <cstdint>

namespace FrameWork {
    //Schema
    enum class Schema {
        Asset,
        Component,
        Scene,
        Editor
    };

    class Importer {
    public:
        Importer();
    private:

    };
}


#endif //CAIENGINE_IMPORTER_H