#include <helpers/cm/YUVConversion.hpp>
#include <drm_fourcc.h>
#include <gtest/gtest.h>

using namespace NColorManagement;

static std::array<float, 3> convert(const SYUVConversion& conversion, float y, float u, float v) {
    const std::array<float, 3> input  = {y + conversion.offset[0], u + conversion.offset[1], v + conversion.offset[2]};
    std::array<float, 3>       output = {};
    for (size_t row = 0; row < output.size(); ++row)
        for (size_t col = 0; col < input.size(); ++col)
            output[row] += conversion.matrix[col * 3 + row] * input[col];
    return output;
}

TEST(YUVConversion, LimitedP010BlackWhiteAndNeutral) {
    for (const auto coefficients :
         {WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT601, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020}) {
        SColorRepresentation representation;
        representation.coefficients = coefficients;
        const auto conversion       = getYUVConversion(DRM_FORMAT_P010, representation, {1920, 1080}, {960, 540});
        for (const auto& [code, expected] : {std::pair{64.F, 0.F}, std::pair{502.F, 0.5F}, std::pair{940.F, 1.F}}) {
            const auto rgb = convert(conversion, code * 64.F / 65535.F, 512.F * 64.F / 65535.F, 512.F * 64.F / 65535.F);
            for (const auto channel : rgb)
                EXPECT_NEAR(channel, expected, 0.000001F);
        }
    }
}

TEST(YUVConversion, FullP010AndNV12Endpoints) {
    SColorRepresentation representation;
    representation.range = WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL;
    for (const auto format : {DRM_FORMAT_NV12, DRM_FORMAT_P010}) {
        const bool  p010       = format == DRM_FORMAT_P010;
        const float maximum    = p010 ? 1023.F * 64.F / 65535.F : 1.F;
        const float neutral    = p010 ? 512.F * 64.F / 65535.F : 128.F / 255.F;
        const auto  conversion = getYUVConversion(format, representation, {1920, 1080}, {960, 540});
        for (const auto channel : convert(conversion, 0.F, neutral, neutral))
            EXPECT_NEAR(channel, 0.F, 0.000001F);
        for (const auto channel : convert(conversion, maximum, neutral, neutral))
            EXPECT_NEAR(channel, 1.F, 0.000001F);
    }
}

TEST(YUVConversion, BT2020P010PrimaryColors) {
    SColorRepresentation representation;
    representation.coefficients                          = WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020;
    const auto                                conversion = getYUVConversion(DRM_FORMAT_P010, representation, {1920, 1080}, {960, 540});
    const std::array<std::array<float, 3>, 3> samples    = {{{294.F, 387.F, 960.F}, {658.F, 189.F, 100.F}, {116.F, 960.F, 476.F}}};
    for (size_t primary = 0; primary < samples.size(); ++primary) {
        const auto& sample = samples[primary];
        const auto  rgb    = convert(conversion, sample[0] * 64.F / 65535.F, sample[1] * 64.F / 65535.F, sample[2] * 64.F / 65535.F);
        for (size_t channel = 0; channel < rgb.size(); ++channel)
            EXPECT_NEAR(rgb[channel], channel == primary ? 1.F : 0.F, 0.002F);
    }
}

TEST(YUVConversion, RetainsTenBitStepsAndHeadroom) {
    const auto  conversion = getYUVConversion(DRM_FORMAT_P010, {}, {1920, 1080}, {960, 540});
    const float neutral    = 512.F * 64.F / 65535.F;
    const auto  first      = convert(conversion, 511.F * 64.F / 65535.F, neutral, neutral);
    const auto  second     = convert(conversion, 512.F * 64.F / 65535.F, neutral, neutral);
    EXPECT_NEAR(second[0] - first[0], 1.F / 876.F, 0.000001F);
    EXPECT_GT(convert(conversion, 1023.F * 64.F / 65535.F, neutral, neutral)[0], 1.F);
    EXPECT_LT(convert(conversion, 0.F, neutral, neutral)[0], 0.F);
}

TEST(YUVConversion, AllChromaLocationsAndOddDimensions) {
    const std::array<Vector2D, 6> offsets = {{{0.125, 0.0}, {0.0, 0.0}, {0.125, 0.125}, {0.0, 0.125}, {0.125, -0.125}, {0.0, -0.125}}};
    for (size_t i = 0; i < offsets.size(); ++i) {
        SColorRepresentation representation;
        representation.chromaLocation = static_cast<wpColorRepresentationSurfaceV1ChromaLocation>(i + 1);
        const auto conversion         = getYUVConversion(DRM_FORMAT_P010, representation, {4, 4}, {2, 2});
        EXPECT_EQ(conversion.chromaScale, (Vector2D{1, 1}));
        EXPECT_EQ(conversion.chromaOffset, offsets[i]);
    }
    const auto conversion = getYUVConversion(DRM_FORMAT_NV12, {}, {5, 3}, {3, 2});
    EXPECT_DOUBLE_EQ(conversion.chromaScale.x, 5.0 / 6.0);
    EXPECT_DOUBLE_EQ(conversion.chromaScale.y, 3.0 / 4.0);
}
