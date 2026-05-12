#include <helpers/cm/ColorManagement.hpp>

#include <gtest/gtest.h>

using namespace NColorManagement;

TEST(ColorManagement, extLinearFallbackUsesSdrReferenceWhite) {
    SImageDescription desc{
        .transferFunction = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .primariesNameSet = true,
        .primariesNamed   = CM_PRIMARIES_SRGB,
        .primaries        = NColorPrimaries::BT709,
    };

    EXPECT_FLOAT_EQ(desc.getTFMaxLuminance(), SDR_MAX_LUMINANCE);
    EXPECT_FLOAT_EQ(desc.getTFRefLuminance(), SDR_REF_LUMINANCE);
}

TEST(ColorManagement, scRGBFallbackUsesSdrReferenceWhite) {
    EXPECT_TRUE(SCRGB_IMAGE_DESCRIPTION->value().windowsScRGB);
    EXPECT_FLOAT_EQ(SCRGB_IMAGE_DESCRIPTION->value().luminances.max, SDR_MAX_LUMINANCE);
    EXPECT_FLOAT_EQ(SCRGB_IMAGE_DESCRIPTION->value().luminances.reference, SDR_REF_LUMINANCE);
}

TEST(ColorManagement, fallbackExtLinearScalesToTargetReferenceWhite) {
    const SImageDescription source{
        .transferFunction   = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .luminances         = {.min = SDR_MIN_LUMINANCE, .max = SDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(SDR_REF_LUMINANCE)},
        .fallbackLuminances = true,
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 120},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 120.F / SDR_REF_LUMINANCE);
    EXPECT_TRUE(source.needsLuminanceMapping(target));
}

TEST(ColorManagement, explicitExtLinearDoesNotScaleToTargetReferenceWhite) {
    const SImageDescription source{
        .transferFunction = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .luminances       = {.min = SDR_MIN_LUMINANCE, .max = SDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(SDR_REF_LUMINANCE)},
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 120},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 1.F);
    EXPECT_FALSE(source.needsLuminanceMapping(target));
}

TEST(ColorManagement, fallbackExtLinearWithoutReferenceWhiteDoesNotScale) {
    const SImageDescription source{
        .transferFunction   = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .luminances         = {.reference = 0},
        .fallbackLuminances = true,
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 120},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 1.F);
    EXPECT_FALSE(source.needsLuminanceMapping(target));
}

TEST(ColorManagement, hdrFallbackDoesNotScaleToTargetReferenceWhite) {
    const SImageDescription source{
        .transferFunction   = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances         = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(HDR_REF_LUMINANCE)},
        .fallbackLuminances = true,
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 120},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 1.F);
    EXPECT_FALSE(source.needsLuminanceMapping(target));
}
