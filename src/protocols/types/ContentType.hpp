#pragma once

#include "content-type-v1.hpp"
#include <cstdint>
#include <string_view>

namespace NContentType {

    enum eContentType : uint8_t {
        CONTENT_TYPE_NONE  = 0,
        CONTENT_TYPE_PHOTO = 1,
        CONTENT_TYPE_VIDEO = 2,
        CONTENT_TYPE_GAME  = 3,
    };

    eContentType fromString(const std::string name);
    eContentType fromWP(wpContentTypeV1Type contentType);
    uint16_t     toDRM(eContentType contentType);
    std::string_view toString(eContentType contentType);
}