#include <protocols/types/ColorManagement.hpp>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace NColorManagement;

TEST(ColorManagement, extLinearFallbackUsesSdrWhite) {
    SImageDescription desc{
        .transferFunction = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .primariesNameSet = true,
        .primariesNamed   = CM_PRIMARIES_SRGB,
        .primaries        = NColorPrimaries::BT709,
    };

    EXPECT_FLOAT_EQ(desc.getTFMaxLuminance(), SDR_MAX_LUMINANCE);
    EXPECT_FLOAT_EQ(desc.getTFRefLuminance(), SDR_REF_LUMINANCE);
}

TEST(ColorManagement, scrgbFallbackUsesSdrWhite) {
    EXPECT_TRUE(SCRGB_IMAGE_DESCRIPTION->value().windowsScRGB);
    EXPECT_FLOAT_EQ(SCRGB_IMAGE_DESCRIPTION->value().luminances.max, SDR_MAX_LUMINANCE);
    EXPECT_FLOAT_EQ(SCRGB_IMAGE_DESCRIPTION->value().luminances.reference, SDR_REF_LUMINANCE);
}

TEST(ColorManagement, referenceWhiteScaleUsesSourceAndTargetReference) {
    const SImageDescription source{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(HDR_REF_LUMINANCE)},
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 40},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 40.f / HDR_REF_LUMINANCE);
}

TEST(ColorManagement, referenceWhiteMismatchRequiresLuminanceMapping) {
    const SImageDescription source{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(HDR_REF_LUMINANCE)},
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 40},
    };
    const SImageDescription sameReferenceTarget{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(HDR_REF_LUMINANCE)},
    };

    EXPECT_TRUE(source.needsLuminanceMapping(target));
    EXPECT_FALSE(source.needsLuminanceMapping(sameReferenceTarget));
}

TEST(ColorManagement, externalShaderIncludesColorManagement) {
    const auto    shaderPath = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src/render/shaders/glsl/ext.frag";
    std::ifstream shaderFile(shaderPath);

    ASSERT_TRUE(shaderFile.is_open()) << shaderPath;

    const std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

    EXPECT_NE(shaderSource.find("surface_CM.glsl"), std::string::npos);
    EXPECT_NE(shaderSource.find("doColorManagement"), std::string::npos);
    EXPECT_NE(shaderSource.find("skipCM"), std::string::npos);
}
