#include <desktop/rule/windowRule/WindowRule.hpp>
#include <desktop/rule/windowRule/WindowRuleApplicator.hpp>

#include <gtest/gtest.h>

using namespace Desktop::Rule;

namespace Desktop::Rule {
    class CWindowRuleApplicatorTest : public ::testing::Test {
      protected:
        using Prop = std::pair<Types::COverridableVar<int>, std::underlying_type_t<eRuleProperty>>;

        void reset(Prop& prop, std::unordered_set<CWindowRuleEffectContainer::storageType>& effects) {
            CWindowRuleApplicator::resetHdrReferenceLuminanceProp(prop, RULE_PROP_CLASS, Types::PRIORITY_WINDOW_RULE, effects);
        }
    };
}

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

TEST(WindowRule, hdrReferenceLuminanceRejectsValuesAboveHdrRange) {
    CWindowRule rule;
    const auto  effect = windowEffects()->get("hdr_reference_luminance");
    if (!effect)
        FAIL() << "missing hdr_reference_luminance effect";

    const auto result = rule.addEffect(effect.value(), "10001");

    EXPECT_FALSE(result.has_value());
}

TEST_F(CWindowRuleApplicatorTest, hdrReferenceLuminanceResetsWhenRuleStopsMatching) {
    Prop prop{Desktop::Types::COverridableVar<int>{0}, RULE_PROP_CLASS};
    prop.first.set(80, Desktop::Types::PRIORITY_WINDOW_RULE);
    std::unordered_set<CWindowRuleEffectContainer::storageType> effects;

    reset(prop, effects);

    EXPECT_EQ(prop.first.valueOrDefault(), 0);
    EXPECT_EQ(prop.second, RULE_PROP_NONE);
    EXPECT_TRUE(effects.contains(WINDOW_RULE_EFFECT_HDR_REFERENCE_LUMINANCE));
}
