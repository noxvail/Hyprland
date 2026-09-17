#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <sys/mman.h>
#include <sys/poll.h>
#include <unistd.h>
#include <vector>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>
#include <wayland-client.h>
#include <wayland.hpp>
#include <xdg-shell.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;
using namespace Hyprutils::Utils;

struct SSurface {
    CSharedPointer<CCWlSurface>    surface;
    CSharedPointer<CCWlSubsurface> subsurface;
    CSharedPointer<CCWlBuffer>     buffer;
};

struct SWlState {
    wl_display*                       display = nullptr;
    CSharedPointer<CCWlRegistry>      registry;
    CSharedPointer<CCWlCompositor>    compositor;
    CSharedPointer<CCWlSubcompositor> subcompositor;
    CSharedPointer<CCWlShm>           shm;
    CSharedPointer<CCXdgWmBase>       shell;
    CSharedPointer<CCXdgSurface>      xdgSurface;
    CSharedPointer<CCXdgToplevel>     toplevel;
    std::vector<SSurface>             surfaces;
    bool                              configured = false;
    int                               width      = 640;
    int                               height     = 480;
};

static bool pollDisplay(wl_display* display, int timeoutMs) {
    while (wl_display_prepare_read(display) != 0) {
        if (wl_display_dispatch_pending(display) < 0)
            return false;
    }

    if (wl_display_flush(display) < 0 && errno != EAGAIN) {
        wl_display_cancel_read(display);
        return false;
    }

    pollfd fd     = {.fd = wl_display_get_fd(display), .events = POLLIN, .revents = 0};
    int    result = 0;
    do {
        result = poll(&fd, 1, timeoutMs);
    } while (result < 0 && errno == EINTR);

    if (result <= 0 || (fd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
        wl_display_cancel_read(display);
        return result == 0;
    }

    return wl_display_read_events(display) >= 0 && wl_display_dispatch_pending(display) >= 0;
}

static bool waitForFlag(SWlState& state, const bool& flag) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!flag) {
        if (std::chrono::steady_clock::now() >= deadline || !pollDisplay(state.display, 100))
            return false;
    }
    return true;
}

static bool waitForCallback(SWlState& state, CSharedPointer<CCWlCallback> callback) {
    bool done = false;
    callback->setDone([&done](CCWlCallback*, uint32_t) { done = true; });
    return waitForFlag(state, done);
}

static bool settleFrames(SWlState& state) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!pollDisplay(state.display, 50))
            return false;
    }
    return true;
}

static bool roundtrip(SWlState& state) {
    return waitForCallback(state, makeShared<CCWlCallback>(rc<wl_proxy*>(wl_display_sync(state.display))));
}

static bool presentRoot(SWlState& state) {
    auto callback = makeShared<CCWlCallback>(state.surfaces[0].surface->sendFrame());
    state.surfaces[0].surface->sendDamage(0, 0, 1, 1);
    state.surfaces[0].surface->sendCommit();
    return waitForCallback(state, callback);
}

static bool bindRegistry(SWlState& state) {
    state.registry = makeShared<CCWlRegistry>(rc<wl_proxy*>(wl_display_get_registry(state.display)));
    state.registry->setGlobal([&state](CCWlRegistry*, uint32_t id, const char* interface, uint32_t version) {
        auto*                  registry = rc<wl_registry*>(state.registry->resource());
        const std::string_view name     = interface;
        if (name == "wl_compositor")
            state.compositor = makeShared<CCWlCompositor>(rc<wl_proxy*>(wl_registry_bind(registry, id, &wl_compositor_interface, std::min(version, 4U))));
        else if (name == "wl_subcompositor")
            state.subcompositor = makeShared<CCWlSubcompositor>(rc<wl_proxy*>(wl_registry_bind(registry, id, &wl_subcompositor_interface, 1)));
        else if (name == "wl_shm")
            state.shm = makeShared<CCWlShm>(rc<wl_proxy*>(wl_registry_bind(registry, id, &wl_shm_interface, 1)));
        else if (name == "xdg_wm_base")
            state.shell = makeShared<CCXdgWmBase>(rc<wl_proxy*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, 1)));
    });
    return roundtrip(state) && state.compositor && state.subcompositor && state.shm && state.shell;
}

static bool attachBuffer(SWlState& state, SSurface& surface, int width, int height, uint32_t color) {
    const int       size = width * height * sizeof(uint32_t);
    CFileDescriptor fd{memfd_create("hyprtester-subsurface-teardown", MFD_CLOEXEC)};
    if (!fd.isValid() || ftruncate(fd.get(), size) < 0)
        return false;

    void* data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.get(), 0);
    if (data == MAP_FAILED)
        return false;
    const CScopeGuard unmap([data, size] { munmap(data, size); });
    std::fill_n(sc<uint32_t*>(data), width * height, color);

    auto pool      = makeShared<CCWlShmPool>(state.shm->sendCreatePool(fd.get(), size));
    surface.buffer = makeShared<CCWlBuffer>(pool->sendCreateBuffer(0, width, height, width * sizeof(uint32_t), WL_SHM_FORMAT_XRGB8888));
    pool->sendDestroy();
    surface.surface->sendAttach(surface.buffer.get(), 0, 0);
    surface.surface->sendDamage(0, 0, width, height);
    return true;
}

static bool mapTree(SWlState& state, int depth) {
    auto& root       = state.surfaces[0];
    root.surface     = makeShared<CCWlSurface>(state.compositor->sendCreateSurface());
    state.xdgSurface = makeShared<CCXdgSurface>(state.shell->sendGetXdgSurface(root.surface->resource()));
    state.toplevel   = makeShared<CCXdgToplevel>(state.xdgSurface->sendGetToplevel());
    state.toplevel->setConfigure([&state](CCXdgToplevel*, int32_t width, int32_t height, wl_array*) {
        if (width > 0)
            state.width = width;
        if (height > 0)
            state.height = height;
    });
    state.shell->setPing([&state](CCXdgWmBase*, uint32_t serial) { state.shell->sendPong(serial); });
    state.xdgSurface->setConfigure([&state](CCXdgSurface*, uint32_t serial) {
        state.xdgSurface->sendAckConfigure(serial);
        state.configured = true;
    });
    state.toplevel->sendSetAppId("subsurface-teardown");
    state.toplevel->sendSetTitle("subsurface teardown test");
    root.surface->sendCommit();

    if (!waitForFlag(state, state.configured) || !attachBuffer(state, root, state.width, state.height, 0xff202430) || !presentRoot(state))
        return false;

    for (int i = 1; i <= depth; ++i) {
        auto& child      = state.surfaces[i];
        child.surface    = makeShared<CCWlSurface>(state.compositor->sendCreateSurface());
        child.subsurface = makeShared<CCWlSubsurface>(state.subcompositor->sendGetSubsurface(child.surface.get(), state.surfaces[i - 1].surface.get()));
        child.subsurface->sendSetPosition(11 + i * 3, 13 + i * 5);
        child.subsurface->sendSetDesync();
        if (!attachBuffer(state, child, i == depth ? 48 : 128, i == depth ? 40 : 96, 0xff204030 + i * 0x00090705))
            return false;
    }

    for (int i = depth; i >= 1; --i)
        state.surfaces[i].surface->sendCommit();
    return presentRoot(state) && roundtrip(state);
}

static bool destroyParentFirst(SWlState& state, int parent) {
    for (size_t i = parent; i < state.surfaces.size(); ++i) {
        std::cout << "destroy level=" << i << std::endl;
        state.surfaces[i].surface->sendDestroy();
        state.surfaces[i].surface.reset();
        if (!roundtrip(state))
            return false;
    }
    return true;
}

static bool destroyTree(SWlState& state) {
    for (size_t i = state.surfaces.size() - 1; i > 0; --i) {
        state.surfaces[i].subsurface->sendDestroy();
        if (state.surfaces[i].surface)
            state.surfaces[i].surface->sendDestroy();
    }
    state.toplevel->sendDestroy();
    state.xdgSurface->sendDestroy();
    state.surfaces[0].surface->sendDestroy();
    return roundtrip(state);
}

static bool parseNumber(std::string_view value, int& result) {
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size();
}

int main(int argc, char** argv) {
    int depth  = 0;
    int parent = 0;
    if (argc != 4 || !parseNumber(argv[2], depth) || !parseNumber(argv[3], parent) || depth < 2 || depth > 4 || parent < 1 || parent >= depth)
        return 2;
    const std::string_view mode = argv[1];
    if (mode != "parent-first" && mode != "disconnect")
        return 2;

    auto* display = wl_display_connect(nullptr);
    if (!display)
        return 1;
    const CScopeGuard disconnect([display] { wl_display_disconnect(display); });
    SWlState          state{.display = display};
    state.surfaces.resize(depth + 1);
    if (!bindRegistry(state) || !mapTree(state, depth) || !settleFrames(state)) {
        std::cerr << "failed to map subsurface hierarchy" << std::endl;
        return 1;
    }

    std::cout << "ready" << std::endl;
    if (mode == "disconnect")
        std::quick_exit(0);
    if (!destroyParentFirst(state, parent) || !destroyTree(state)) {
        std::cerr << "failed to tear down subsurface hierarchy" << std::endl;
        return 1;
    }
    return 0;
}
