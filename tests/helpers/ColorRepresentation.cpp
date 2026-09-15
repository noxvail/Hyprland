#include <helpers/cm/ColorRepresentation.hpp>
#include <protocols/types/SurfaceState.hpp>
#include <gtest/gtest.h>
#include <drm_fourcc.h>

using namespace NColorManagement;

TEST(ColorRepresentation, acceptsAdvertisedMatricesAndRanges) {
    for (const auto coefficients :
         {WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT601, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020}) {
        EXPECT_TRUE(isColorRepresentationSupported(coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL));
        EXPECT_TRUE(isColorRepresentationSupported(coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED));
        EXPECT_FALSE(isColorRepresentationSupported(coefficients, static_cast<wpColorRepresentationSurfaceV1Range>(0)));
    }

    EXPECT_FALSE(isColorRepresentationSupported(WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_IDENTITY, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL));
    EXPECT_FALSE(isColorRepresentationSupported(WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020_CL, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED));
    EXPECT_FALSE(isColorRepresentationSupported(WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_ICTCP, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED));
}

TEST(ColorRepresentation, acceptsAllProtocolChromaLocations) {
    for (uint32_t location = 1; location <= 6; ++location)
        EXPECT_TRUE(isChromaLocationSupported(static_cast<wpColorRepresentationSurfaceV1ChromaLocation>(location)));

    EXPECT_FALSE(isChromaLocationSupported(static_cast<wpColorRepresentationSurfaceV1ChromaLocation>(0)));
    EXPECT_FALSE(isChromaLocationSupported(static_cast<wpColorRepresentationSurfaceV1ChromaLocation>(7)));
}

TEST(ColorRepresentation, explicitYuvMetadataRequiresSupportedYuvBuffer) {
    SColorRepresentation representation;
    EXPECT_TRUE(isColorRepresentationCompatible(DRM_FORMAT_ARGB8888, representation));

    representation.coefficientsSet = true;
    EXPECT_TRUE(isColorRepresentationCompatible(DRM_FORMAT_NV12, representation));
    EXPECT_TRUE(isColorRepresentationCompatible(DRM_FORMAT_P010, representation));
    EXPECT_FALSE(isColorRepresentationCompatible(DRM_FORMAT_ARGB8888, representation));
    EXPECT_FALSE(isColorRepresentationCompatible(DRM_FORMAT_YUYV, representation));

    representation.coefficientsSet   = false;
    representation.chromaLocationSet = true;
    EXPECT_FALSE(isColorRepresentationCompatible(DRM_FORMAT_ARGB8888, representation));
}

TEST(ColorRepresentation, stateIsCopiedOnlyByARepresentationCommit) {
    SSurfaceState current;
    SSurfaceState pending;
    pending.colorRepresentation.coefficients    = WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020;
    pending.colorRepresentation.range           = WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL;
    pending.colorRepresentation.coefficientsSet = true;

    current.updateFrom(pending);
    EXPECT_FALSE(current.colorRepresentation.coefficientsSet);

    pending.updated.bits.colorRepresentation = true;
    current.updateFrom(pending);
    EXPECT_EQ(current.colorRepresentation, pending.colorRepresentation);

    pending.reset();
    EXPECT_FALSE(pending.updated.bits.colorRepresentation);
    EXPECT_EQ(current.colorRepresentation, pending.colorRepresentation);

    pending.colorRepresentation              = {};
    pending.updated.bits.colorRepresentation = true;
    EXPECT_TRUE(current.colorRepresentation.coefficientsSet);
    current.updateFrom(pending);
    EXPECT_EQ(current.colorRepresentation, SColorRepresentation{});
}

TEST(ColorRepresentation, queuedCommitsKeepTheirOwnMetadata) {
    SSurfaceState current;
    SSurfaceState pending;
    pending.colorRepresentation.coefficients    = WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020;
    pending.colorRepresentation.coefficientsSet = true;
    pending.updated.bits.colorRepresentation    = true;
    auto queued                                 = pending;

    pending.reset();
    pending.colorRepresentation.coefficients = WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709;
    pending.updated.bits.colorRepresentation = true;

    current.updateFrom(queued);
    EXPECT_EQ(current.colorRepresentation.coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020);
    current.updateFrom(pending);
    EXPECT_EQ(current.colorRepresentation.coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709);
}
