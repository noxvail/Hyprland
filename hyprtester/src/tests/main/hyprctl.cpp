#include "tests.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include <hyprutils/os/Process.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include "../shared.hpp"
#include <regex>

static int ret = 0;

using namespace Hyprutils::OS;
using namespace Hyprutils::Memory;

#define UP CUniquePointer
#define SP CSharedPointer

static bool test() {
    NLog::log("{}Testing hyprctl", Colors::GREEN);

    {
        NLog::log("{}Testing hyprctl descriptions for any json errors", Colors::GREEN);
        CProcess jqProc("bash", {"-c", "hyprctl descriptions | jq"});
        jqProc.addEnv("HYPRLAND_INSTANCE_SIGNATURE", HIS);
        jqProc.runSync();
        EXPECT(jqProc.exitCode(), 0);
    }

    {
        NLog::log("{}Testing hyprctl clients contentType formatting", Colors::GREEN);
        const int BEFORE = Tests::windowCount();
        auto       kitty  = Tests::spawnKitty();
        EXPECT(Tests::windowCount(), BEFORE + 1);

        const auto JSON = getFromSocket("j/clients");
        EXPECT_CONTAINS(JSON, "\"contentType\"");

        {
            const std::string jsonStr = std::string{JSON};
            const std::regex  re(R"CT("contentType"\s*:\s*"([^"]+)")CT");
            auto              it  = std::sregex_iterator(jsonStr.begin(), jsonStr.end(), re);
            auto              end = std::sregex_iterator();
            EXPECT(it == end, false);
            for (; it != end; ++it) {
                const std::string value = (*it)[1].str();
                const bool        valid = value == "none" || value == "photo" || value == "video" || value == "game";
                EXPECT(valid, true);
            }
        }

        const auto TEXT = getFromSocket("/clients");
        if (std::string{TEXT}.contains("contentType:")) {
            EXPECT_CONTAINS(TEXT, "contentType: ");
        }

        Tests::killAllWindows();
        EXPECT(Tests::windowCount(), BEFORE);
    }

    return !ret;
}

REGISTER_TEST_FN(test);
