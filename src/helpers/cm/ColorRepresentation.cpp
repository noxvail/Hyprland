#include "ColorRepresentation.hpp"
#include <drm_fourcc.h>

bool NColorManagement::isColorRepresentationSupported(wpColorRepresentationSurfaceV1Coefficients coefficients, wpColorRepresentationSurfaceV1Range range) {
    if (range != WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL && range != WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED)
        return false;

    return coefficients == WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT601 || coefficients == WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709 ||
        coefficients == WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020;
}

bool NColorManagement::isChromaLocationSupported(wpColorRepresentationSurfaceV1ChromaLocation location) {
    return location >= WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_0 && location <= WP_COLOR_REPRESENTATION_SURFACE_V1_CHROMA_LOCATION_TYPE_5;
}

bool NColorManagement::isColorRepresentationCompatible(uint32_t drmFormat, const SColorRepresentation& representation) {
    if (!representation.coefficientsSet && !representation.chromaLocationSet)
        return true;

    return drmFormat == DRM_FORMAT_NV12 || drmFormat == DRM_FORMAT_P010;
}
