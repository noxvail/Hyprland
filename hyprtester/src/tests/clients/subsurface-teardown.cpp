#include "../../Log.hpp"
#include "../../hyprctlCompat.hpp"
#include "../shared.hpp"
#include "build.hpp"
#include "tests.hpp"

#include <array>
#include <hyprutils/os/Process.hpp>

TEST_CASE(subsurfaceTeardown) {
    if (!Tests::killAllWindows())
        FAIL_TEST("Couldn't clear windows before subsurface teardown test");

    struct SScenario {
        std::string mode;
        int         depth  = 2;
        int         parent = 1;
    };

    const std::array<SScenario, 5> scenarios = {{
        {"parent-first", 2, 1},
        {"disconnect", 2, 1},
        {"parent-first", 4, 1},
        {"parent-first", 4, 3},
        {"disconnect", 4, 1},
    }};

    for (const auto& scenario : scenarios) {
        NLog::log("Testing subsurface teardown: {} depth={} parent={}", scenario.mode, scenario.depth, scenario.parent);
        Hyprutils::OS::CProcess client(binaryDir + "/subsurface-teardown", {scenario.mode, std::to_string(scenario.depth), std::to_string(scenario.parent)});
        client.addEnv("WAYLAND_DISPLAY", WLDISPLAY);
        if (!client.runSync() || client.exitCode() != 0 || !client.stdOut().contains("ready"))
            FAIL_TEST("Subsurface teardown client failed: stdout={} stderr={}", client.stdOut(), client.stdErr());

        Tests::sync();
        EXPECT(getFromSocket("/version").contains("Hyprland"), true);
        EXPECT(Tests::windowCount(), 0);
    }
}
