#include <helpers/cm/ColorManagement.hpp>

#include <gtest/gtest.h>

using namespace NColorManagement;

TEST(ColorManagement, hdrReferenceScaleMapsAdvertisedReferenceWhiteToRuleValue) {
    const SImageDescription desc{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 203},
    };

    EXPECT_FLOAT_EQ(desc.hdrReferenceWhiteScale(40), 40.F / 203.F);
}

TEST(ColorManagement, hdrReferenceScaleLeavesSdrDescriptionsUnchanged) {
    const SImageDescription desc{
        .transferFunction = CM_TRANSFER_FUNCTION_SRGB,
        .luminances       = {.min = SDR_MIN_LUMINANCE, .max = SDR_MAX_LUMINANCE, .reference = 80},
    };

    EXPECT_FLOAT_EQ(desc.hdrReferenceWhiteScale(40), 1.F);
}
