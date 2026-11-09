#pragma once

#include "color-representation-v1.hpp"

namespace NColorManagement {
    struct SColorRepresentation {
        wpColorRepresentationSurfaceV1Coefficients   coefficients      = WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT601;
        wpColorRepresentationSurfaceV1Range          range             = WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED;
        wpColorRepresentationSurfaceV1ChromaLocation chromaLocation    = WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_1;
        bool                                         coefficientsSet   = false;
        bool                                         chromaLocationSet = false;

        bool                                         operator==(const SColorRepresentation&) const = default;
    };

    bool isColorRepresentationSupported(wpColorRepresentationSurfaceV1Coefficients coefficients, wpColorRepresentationSurfaceV1Range range);
    bool isChromaLocationSupported(wpColorRepresentationSurfaceV1ChromaLocation location);
    bool isColorRepresentationCompatible(uint32_t drmFormat, const SColorRepresentation& representation);
}
