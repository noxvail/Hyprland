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

TEST(ColorManagement, bt1886KeepsProtocolLuminanceDefaults) {
    const SImageDescription desc{.transferFunction = CM_TRANSFER_FUNCTION_BT1886};

    EXPECT_FLOAT_EQ(desc.getTFMinLuminance(), 0.01F);
    EXPECT_FLOAT_EQ(desc.getTFMaxLuminance(), 100.F);
    EXPECT_FLOAT_EQ(desc.getTFRefLuminance(), 100.F);
}

TEST(ColorManagement, bt1886UsesConfiguredSdrLuminance) {
    const SImageDescription desc{.transferFunction = CM_TRANSFER_FUNCTION_BT1886};

    EXPECT_FLOAT_EQ(desc.getTFMinLuminance(0.005F), 0.005F);
    EXPECT_FLOAT_EQ(desc.getTFMaxLuminance(40), 40.F);
    EXPECT_FLOAT_EQ(desc.getTFRefLuminance(40), 40.F);
}

TEST(ColorManagement, pqKeepsAbsoluteLuminanceWithSdrOverrides) {
    const SImageDescription desc{.transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ};

    EXPECT_FLOAT_EQ(desc.getTFMinLuminance(0.01F), HDR_MIN_LUMINANCE);
    EXPECT_FLOAT_EQ(desc.getTFMaxLuminance(40), HDR_MAX_LUMINANCE);
    EXPECT_FLOAT_EQ(desc.getTFRefLuminance(40), HDR_REF_LUMINANCE);
}

TEST(ColorManagement, bt1886UsesDisplayGammaAtZeroBlack) {
    EXPECT_NEAR(bt1886Eotf(0.5, 0, 1), 0.18946457081379976, 1e-12);
    EXPECT_NEAR(bt1886Eotf(0.1, 0, 1), 0.003981071705534973, 1e-12);
}

TEST(ColorManagement, bt1886CompensatesForBlackLuminance) {
    EXPECT_NEAR(bt1886Eotf(0.5, 0.005, 40), 8.015765473428517, 1e-10);
    EXPECT_NEAR(bt1886Eotf(0.5, 0.1, 100), 21.604911167389365, 1e-10);
    EXPECT_NEAR(bt1886Eotf(0, 0.005, 40), 0.005, 1e-12);
    EXPECT_NEAR(bt1886Eotf(1, 0.005, 40), 40, 1e-10);
}

TEST(ColorManagement, bt1886InverseMapsKnownLuminances) {
    EXPECT_NEAR(bt1886InverseEotf(8.015765473428517, 0.005, 40), 0.5, 1e-12);
    EXPECT_NEAR(bt1886InverseEotf(0.005, 0.005, 40), 0, 1e-12);
    EXPECT_NEAR(bt1886InverseEotf(40, 0.005, 40), 1, 1e-12);
}

TEST(ColorManagement, bt1886PreservesExtendedCodeValues) {
    EXPECT_NEAR(bt1886InverseEotf(bt1886Eotf(1.1, 0.005, 40), 0.005, 40), 1.1, 1e-12);
    EXPECT_NEAR(bt1886InverseEotf(bt1886Eotf(-0.01, 0.005, 40), 0.005, 40), -0.01, 1e-12);
    EXPECT_DOUBLE_EQ(bt1886Eotf(-1, 0.005, 40), 0);
}
