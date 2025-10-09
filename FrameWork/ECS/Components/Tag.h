//
// Created by 51092 on 2025/10/8.
//

#ifndef CAIENGINE_TAG_H
#define CAIENGINE_TAG_H
#include <string>
#include "PublicEnum.h"

namespace ECS {
    struct Tag {
        std::string name{};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Tag, name)
}

#endif //CAIENGINE_TAG_H