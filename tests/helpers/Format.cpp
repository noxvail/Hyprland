#include <helpers/Format.hpp>

#include <gtest/gtest.h>

#include <drm_fourcc.h>
#include <wayland-server-protocol.h>
#include <hyprgraphics/egl/Egl.hpp>
#include <array>
#include <limits>

using namespace Hyprgraphics::Egl;
using namespace NFormatUtils;

TEST(Helpers, formatDrmToShm) {
    EXPECT_EQ(drmToShm(DRM_FORMAT_XRGB8888), WL_SHM_FORMAT_XRGB8888);
    EXPECT_EQ(drmToShm(DRM_FORMAT_ARGB8888), WL_SHM_FORMAT_ARGB8888);
}

TEST(Helpers, formatShmToDrm) {
    EXPECT_EQ(shmToDRM(WL_SHM_FORMAT_XRGB8888), DRM_FORMAT_XRGB8888);
    EXPECT_EQ(shmToDRM(WL_SHM_FORMAT_ARGB8888), DRM_FORMAT_ARGB8888);
}

TEST(Helpers, formatDrmShmRoundTrip) {
    EXPECT_EQ(shmToDRM(drmToShm(DRM_FORMAT_XRGB8888)), DRM_FORMAT_XRGB8888);
    EXPECT_EQ(shmToDRM(drmToShm(DRM_FORMAT_ARGB8888)), DRM_FORMAT_ARGB8888);
}

TEST(Helpers, formatIsFormatYUV) {
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_YUYV));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_NV12));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_NV21));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_P010));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_P012));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_YVU420));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_XYUV8888));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_Y210));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_Q410));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_NV24));
    EXPECT_TRUE(isFormatYUV(DRM_FORMAT_P010 | DRM_FORMAT_BIG_ENDIAN));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_XRGB8888));
    EXPECT_FALSE(isFormatYUV(DRM_FORMAT_ARGB8888));
}

TEST(Helpers, formatYuvModifiersRequireBothRawPlanesAndKnownLayouts) {
    constexpr uint64_t            amdTiled      = 0x0200000028a01f04;
    constexpr uint64_t            amdDcc        = amdTiled | AMD_FMT_MOD_SET(DCC, 1);
    constexpr uint64_t            unknownVendor = fourcc_mod_code(INTEL, 1);
    const std::array<uint64_t, 5> luma{DRM_FORMAT_MOD_LINEAR, amdTiled, amdDcc, unknownVendor, DRM_FORMAT_MOD_INVALID};
    const std::array<uint64_t, 4> chroma{amdTiled, amdDcc, unknownVendor, DRM_FORMAT_MOD_INVALID};

    EXPECT_EQ(intersectYUVModifiers(luma, chroma), std::vector<uint64_t>{amdTiled});
    EXPECT_TRUE(isSupportedYUVModifier(DRM_FORMAT_MOD_LINEAR));
    EXPECT_FALSE(isSupportedYUVModifier(DRM_FORMAT_MOD_INVALID));
    EXPECT_FALSE(isSupportedYUVModifier(amdDcc));
}

TEST(Helpers, formatGetPixelFormatFromDRM) {
    const auto* xrgb = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(xrgb, nullptr);
    EXPECT_EQ(xrgb->drmFormat, DRM_FORMAT_XRGB8888);
    EXPECT_FALSE(xrgb->withAlpha);

    const auto* argb = getPixelFormatFromDRM(DRM_FORMAT_ARGB8888);
    ASSERT_NE(argb, nullptr);
    EXPECT_EQ(argb->drmFormat, DRM_FORMAT_ARGB8888);
    EXPECT_TRUE(argb->withAlpha);

    EXPECT_EQ(getPixelFormatFromDRM(0), nullptr);
}

TEST(Helpers, formatIsFormatOpaque) {
    EXPECT_TRUE(isDrmFormatOpaque(DRM_FORMAT_XRGB8888));
    EXPECT_FALSE(isDrmFormatOpaque(DRM_FORMAT_ARGB8888));
}

TEST(Helpers, formatPixelsPerBlock) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);
    EXPECT_GT(pixelsPerBlock(fmt), 0);
}

TEST(Helpers, formatMinStride) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);
    // XRGB8888 = 4 bytes per pixel, 1920 wide = 7680 bytes stride
    EXPECT_EQ(minStride(fmt, 1920), 1920 * 4);
    EXPECT_EQ(minStride(fmt, 0), 0);
}

TEST(Helpers, formatShmBufferLayoutValid) {
    const auto* fmt = getPixelFormatFromDRM(DRM_FORMAT_XRGB8888);
    ASSERT_NE(fmt, nullptr);

    const auto stride = static_cast<int32_t>(minStride(fmt, 64));
    const auto size   = static_cast<size_t>(stride) * 64;

    EXPECT_TRUE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 0, size));
    EXPECT_TRUE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 16, size + 16));

    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride - 1, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, 1, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, stride, -1, size));
    EXPECT_FALSE(isShmBufferLayoutValid(0, {64, 64}, stride, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {0, 64}, stride, 0, size));
    EXPECT_FALSE(isShmBufferLayoutValid(DRM_FORMAT_XRGB8888, {64, 64}, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max(), 0));
}

TEST(Helpers, formatDrmFormatName) {
    EXPECT_FALSE(drmFormatName(DRM_FORMAT_XRGB8888).empty());
    EXPECT_FALSE(drmFormatName(DRM_FORMAT_ARGB8888).empty());
    EXPECT_EQ(drmFormatName(0), "INVALID");
}

TEST(Helpers, formatAlphaFormat) {
    EXPECT_EQ(alphaFormat(DRM_FORMAT_XRGB8888), DRM_FORMAT_ARGB8888);
    EXPECT_EQ(alphaFormat(DRM_FORMAT_XBGR8888), DRM_FORMAT_ABGR8888);
    // Format without alpha stripped entry returns DRM_FORMAT_INVALID (0)
    EXPECT_EQ(alphaFormat(DRM_FORMAT_ARGB8888), 0u);
}
