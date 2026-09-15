#pragma once

#include <array>
#include <cstdint>
#include "ColorRepresentation.hpp"
#include "../math/Math.hpp"

namespace NColorManagement {
    struct SYUVConversion {
        std::array<float, 9> matrix = {};
        std::array<float, 3> offset = {};
        Vector2D             chromaScale;
        Vector2D             chromaOffset;
    };

    SYUVConversion getYUVConversion(uint32_t format, const SColorRepresentation& representation, const Vector2D& lumaSize, const Vector2D& chromaSize);
}
