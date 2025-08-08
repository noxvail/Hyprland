#include "tests.hpp"
#include "../../shared.hpp"
#include "../../hyprctlCompat.hpp"
#include <print>
#include <thread>
#include <chrono>
#include <hyprutils/os/Process.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <csignal>
#include <cerrno>
#include "../shared.hpp"

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

        const auto TEXT = getFromSocket("/clients");
        if (std::string{TEXT}.contains("contentType:"))
            EXPECT_CONTAINS(TEXT, "contentType: ");

        Tests::killAllWindows();
    }

    return !ret;
}

REGISTER_TEST_FN(test);
