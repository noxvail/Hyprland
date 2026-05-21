#include <desktop/rule/windowRule/WindowRule.hpp>

#include <gtest/gtest.h>

using namespace Desktop::Rule;

TEST(WindowRule, hdrReferenceLuminanceParsesAsInteger) {
    CWindowRule rule;
    const auto  effect = windowEffects()->get("hdr_reference_luminance");
    if (!effect)
        FAIL() << "missing hdr_reference_luminance effect";

    const auto result = rule.addEffect(effect.value(), "80");

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(rule.effects().size(), 1);
    EXPECT_EQ(std::get<int64_t>(rule.effects().front().value), 80);
}

TEST(WindowRule, hdrReferenceLuminanceRejectsNegativeValues) {
    CWindowRule rule;
    const auto  effect = windowEffects()->get("hdr_reference_luminance");
    if (!effect)
        FAIL() << "missing hdr_reference_luminance effect";

    const auto result = rule.addEffect(effect.value(), "-1");

    EXPECT_FALSE(result.has_value());
}
