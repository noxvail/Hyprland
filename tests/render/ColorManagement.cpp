#include <protocols/types/ColorManagement.hpp>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace NColorManagement;

static std::filesystem::path sourceFilePath() {
    const auto filePath = std::filesystem::path(__FILE__);

    if (filePath.is_absolute())
        return filePath;

    return std::filesystem::weakly_canonical(std::filesystem::current_path().parent_path() / filePath);
}

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

TEST(ColorManagement, fallbackPqDoesNotApplyReferenceWhiteRemap) {
    const SImageDescription source{
        .transferFunction   = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances         = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(HDR_REF_LUMINANCE)},
        .fallbackLuminances = true,
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 40},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 1.0f);
    EXPECT_FALSE(source.needsLuminanceMapping(target));
}

TEST(ColorManagement, fallbackExtLinearStillUsesReferenceWhiteRemap) {
    const SImageDescription source{
        .transferFunction   = CM_TRANSFER_FUNCTION_EXT_LINEAR,
        .luminances         = {.min = SDR_MIN_LUMINANCE, .max = SDR_MAX_LUMINANCE, .reference = static_cast<uint32_t>(SDR_REF_LUMINANCE)},
        .fallbackLuminances = true,
    };
    const SImageDescription target{
        .transferFunction = CM_TRANSFER_FUNCTION_ST2084_PQ,
        .luminances       = {.min = HDR_MIN_LUMINANCE, .max = HDR_MAX_LUMINANCE, .reference = 40},
    };

    EXPECT_FLOAT_EQ(source.getReferenceWhiteScale(target), 40.f / SDR_REF_LUMINANCE);
    EXPECT_TRUE(source.needsLuminanceMapping(target));
}

TEST(ColorManagement, externalShaderIncludesColorManagement) {
    const auto    shaderPath = sourceFilePath().parent_path().parent_path().parent_path() / "src/render/shaders/glsl/ext.frag";
    std::ifstream shaderFile(shaderPath);

    ASSERT_TRUE(shaderFile.is_open()) << shaderPath;

    const std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

    EXPECT_NE(shaderSource.find("surface_CM.glsl"), std::string::npos);
    EXPECT_NE(shaderSource.find("doColorManagement"), std::string::npos);
    EXPECT_NE(shaderSource.find("skipCM"), std::string::npos);
}

TEST(ColorManagement, tonemapShaderUsesMappedLuminance) {
    const auto    shaderPath = sourceFilePath().parent_path().parent_path().parent_path() / "src/render/shaders/glsl/tonemap.glsl";
    std::ifstream shaderFile(shaderPath);

    ASSERT_TRUE(shaderFile.is_open()) << shaderPath;

    const std::string shaderSource((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

    EXPECT_NE(shaderSource.find("ICtCp[0]"), std::string::npos);
    EXPECT_NE(shaderSource.find("newLum / HDR_MAX_LUMINANCE"), std::string::npos);
    EXPECT_NE(shaderSource.find("color.rgb * refScale"), std::string::npos);
}
