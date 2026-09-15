#include "YUVConversion.hpp"
#include <drm_fourcc.h>

NColorManagement::SYUVConversion NColorManagement::getYUVConversion(uint32_t format, const SColorRepresentation& representation, const Vector2D& lumaSize,
                                                                    const Vector2D& chromaSize) {
    float kr = 0.299F;
    float kb = 0.114F;
    if (representation.coefficients == WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709) {
        kr = 0.2126F;
        kb = 0.0722F;
    } else if (representation.coefficients == WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020) {
        kr = 0.2627F;
        kb = 0.0593F;
    }

    const float kg          = 1.F - kr - kb;
    const bool  p010        = format == DRM_FORMAT_P010;
    const bool  limited     = representation.range == WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED;
    const float sampleScale = p010 ? 65535.F / 64.F : 255.F;
    const float depthScale  = p010 ? 4.F : 1.F;
    const float fullScale   = p010 ? 1023.F : 255.F;
    const float yScale      = sampleScale / (limited ? 219.F * depthScale : fullScale);
    const float cScale      = sampleScale / (limited ? 224.F * depthScale : fullScale);

    Vector2D    location = {0.5, 0.5};
    switch (representation.chromaLocation) {
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_0: location = {0.0, 0.5}; break;
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_1: break;
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_2: location = {0, 0}; break;
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_3: location = {0.5, 0.0}; break;
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_4: location = {0, 1}; break;
        case WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_5: location = {0.5, 1.0}; break;
    }

    return {
        .matrix =
            {
                yScale,
                yScale,
                yScale,
                0.F,
                -2.F * kb * (1.F - kb) / kg * cScale,
                2.F * (1.F - kb) * cScale,
                2.F * (1.F - kr) * cScale,
                -2.F * kr * (1.F - kr) / kg * cScale,
                0.F,
            },
        .offset       = {limited ? -16.F * depthScale / sampleScale : 0.F, -128.F * depthScale / sampleScale, -128.F * depthScale / sampleScale},
        .chromaScale  = lumaSize / (chromaSize * 2.0),
        .chromaOffset = (Vector2D{0.5, 0.5} - location) / (chromaSize * 2.0),
    };
}
