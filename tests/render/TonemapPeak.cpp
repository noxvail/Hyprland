#include <render/Renderer.hpp>

#include <gtest/gtest.h>

using namespace NColorManagement;

static constexpr auto PQ     = CM_TRANSFER_FUNCTION_ST2084_PQ;
static constexpr auto HLG    = CM_TRANSFER_FUNCTION_HLG;
static constexpr auto LINEAR = CM_TRANSFER_FUNCTION_EXT_LINEAR;
static constexpr auto SRGB   = CM_TRANSFER_FUNCTION_SRGB;
static constexpr auto G22    = CM_TRANSFER_FUNCTION_GAMMA22;

TEST(Render, tonemapPeakKeepsAbsolutePQPeakOnHDRWorkBuffer) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, LINEAR, 1000, 630, 446, 203), 1000);
}

TEST(Render, tonemapPeakKeepsAbsolutePQPeakOnPQTarget) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, PQ, 1000, 630, 446, 203), 1000);
}

TEST(Render, tonemapPeakKeepsAbsoluteHLGPeak) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(HLG, LINEAR, 1000, 630, 446, 203), 1000);
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(HLG, HLG, 1000, 630, 446, 203), 1000);
}

TEST(Render, tonemapPeakKeepsAbsoluteLinearSourcePeak) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(LINEAR, LINEAR, 1000, 630, 446, 203), 1000);
}

TEST(Render, tonemapPeakTreatsExplicitHDRTargetAsHDRWithoutHeadroom) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, PQ, 1000, 203, 1000, 203), 1000);
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(HLG, HLG, 1000, 203, 1000, 203), 1000);
}

TEST(Render, tonemapPeakUnchangedWhenReferencesMatch) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, LINEAR, 1000, 630, 203, 203), 1000);
}

TEST(Render, tonemapPeakScalesOnSDRWorkBuffer) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, LINEAR, 1000, 80, 203, 80), 1000.F * 80 / 203);
}

TEST(Render, tonemapPeakScalesOnSDRTargets) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(LINEAR, SRGB, 630, 80, 203, 80), 630.F * 80 / 203);
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(PQ, G22, 10000, 80, 203, 80), 10000.F * 80 / 203);
}

TEST(Render, tonemapPeakScalesForSDRSources) {
    EXPECT_FLOAT_EQ(Render::tonemapShaderMaxLuminance(G22, LINEAR, 400, 630, 400, 203), 400.F * 203 / 400);
}

TEST(Render, tonemapPeakMatchesHDRReferenceRulePixelScale) {
    for (const auto tf : {PQ, HLG, LINEAR}) {
        for (const uint32_t reference : {203U, 228U, 446U}) {
            const SImageDescription desc{
                .transferFunction = tf,
                .luminances       = {.min = 0, .max = 1000, .reference = reference},
            };
            const auto scale = desc.hdrReferenceWhiteScale(40);
            const auto peak  = Render::tonemapShaderMaxLuminance(tf, LINEAR, desc.luminances.max, 630, reference, 203) * scale;

            EXPECT_FLOAT_EQ(peak, 1000.F * 40 / reference);
            EXPECT_FLOAT_EQ(reference * scale, 40);
        }
    }
}

TEST(Render, tonemapPeakKeepsAbsolutePeakWithDisabledHDRReferenceRule) {
    const SImageDescription desc{
        .transferFunction = PQ,
        .luminances       = {.min = 0, .max = 1000, .reference = 446},
    };
    const auto peak = Render::tonemapShaderMaxLuminance(PQ, LINEAR, 1000, 630, 446, 203) * desc.hdrReferenceWhiteScale(0);

    EXPECT_FLOAT_EQ(peak, 1000);
    EXPECT_GE(peak, 630 * 1.01F);
}

TEST(Render, tonemapPeakStillNeedsTonemapWhenHDRReferenceRuleRaisesBrightness) {
    const SImageDescription desc{
        .transferFunction = PQ,
        .luminances       = {.min = 0, .max = 1000, .reference = 446},
    };
    const auto peak = Render::tonemapShaderMaxLuminance(PQ, LINEAR, 1000, 630, 446, 203) * desc.hdrReferenceWhiteScale(400);

    EXPECT_FLOAT_EQ(peak, 1000.F * 400 / 446);
    EXPECT_GE(peak, 630 * 1.01F);
}

TEST(Render, tonemapPeakKeepsSDRTargetScalingWithHDRReferenceRule) {
    const SImageDescription desc{
        .transferFunction = PQ,
        .luminances       = {.min = 0, .max = 1000, .reference = 446},
    };
    const auto peak = Render::tonemapShaderMaxLuminance(PQ, SRGB, 1000, 80, 446, 80) * desc.hdrReferenceWhiteScale(40);

    EXPECT_FLOAT_EQ(peak, (1000.F * 80 / 446) * (40.F / 446));
}
