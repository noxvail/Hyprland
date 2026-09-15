#include "ColorRepresentation.hpp"
#include "core/Compositor.hpp"
#include "../helpers/cm/ColorRepresentation.hpp"

using namespace NColorManagement;

CColorRepresentationSurface::CColorRepresentationSurface(SP<CWpColorRepresentationSurfaceV1> resource, SP<CWLSurfaceResource> surface) : m_resource(resource), m_surface(surface) {
    if (!good())
        return;

    m_resource->setDestroy([this](CWpColorRepresentationSurfaceV1* resource) { PROTO::colorRepresentation->destroyResource(this); });
    m_resource->setOnDestroy([this](CWpColorRepresentationSurfaceV1* resource) { PROTO::colorRepresentation->destroyResource(this); });

    m_resource->setSetAlphaMode([this](CWpColorRepresentationSurfaceV1* resource, wpColorRepresentationSurfaceV1AlphaMode mode) {
        if (!surfaceAlive())
            return;

        if (mode != WP_COLOR_REPRESENTATION_SURFACE_V1_ALPHA_MODE_PREMULTIPLIED_ELECTRICAL)
            resource->error(WP_COLOR_REPRESENTATION_SURFACE_V1_ERROR_ALPHA_MODE, "Unsupported alpha mode");
    });

    m_resource->setSetCoefficientsAndRange(
        [this](CWpColorRepresentationSurfaceV1* resource, wpColorRepresentationSurfaceV1Coefficients coefficients, wpColorRepresentationSurfaceV1Range range) {
            if (!surfaceAlive())
                return;

            if (!isColorRepresentationSupported(coefficients, range)) {
                resource->error(WP_COLOR_REPRESENTATION_SURFACE_V1_ERROR_COEFFICIENTS, "Unsupported matrix coefficients or range");
                return;
            }

            auto& pending           = m_surface->m_pending.colorRepresentation;
            pending.coefficients    = coefficients;
            pending.range           = range;
            pending.coefficientsSet = true;
            damageSurface();
        });

    m_resource->setSetChromaLocation([this](CWpColorRepresentationSurfaceV1* resource, wpColorRepresentationSurfaceV1ChromaLocation location) {
        if (!surfaceAlive())
            return;

        if (!isChromaLocationSupported(location)) {
            resource->error(WP_COLOR_REPRESENTATION_SURFACE_V1_ERROR_CHROMA_LOCATION, "Invalid chroma location");
            return;
        }

        auto& pending             = m_surface->m_pending.colorRepresentation;
        pending.chromaLocation    = location;
        pending.chromaLocationSet = true;
        damageSurface();
    });

    m_surfacePrecommit = m_surface->m_events.precommit.listen([this] { validatePending(); });
    m_surfaceDestroy   = m_surface->m_events.destroy.listen([this] {
        m_surface.reset();
        m_surfacePrecommit.reset();
    });
}

CColorRepresentationSurface::~CColorRepresentationSurface() {
    if (!m_surface)
        return;

    m_surface->m_colorRepresentation.reset();
    m_surface->m_pending.colorRepresentation = {};
    damageSurface();
}

bool CColorRepresentationSurface::good() {
    return m_resource && m_resource->resource();
}

bool CColorRepresentationSurface::surfaceAlive() {
    if (m_surface)
        return true;

    m_resource->error(WP_COLOR_REPRESENTATION_SURFACE_V1_ERROR_INERT, "Surface has been destroyed");
    return false;
}

void CColorRepresentationSurface::damageSurface() {
    m_surface->m_pending.updated.bits.colorRepresentation = true;
}

void CColorRepresentationSurface::validatePending() {
    if (!m_surface || m_surface->m_pending.bufferSize == Vector2D{})
        return;

    auto& pending = m_surface->m_pending;
    if (isColorRepresentationCompatible(pending.texture ? pending.texture->m_drmFormat : 0, pending.colorRepresentation))
        return;

    m_resource->error(WP_COLOR_REPRESENTATION_SURFACE_V1_ERROR_PIXEL_FORMAT, "Pixel format is incompatible with the color representation");
    pending.rejected = true;
}

CColorRepresentationProtocol::CColorRepresentationProtocol(const wl_interface* iface, const int& ver, const std::string& name) : IWaylandProtocol(iface, ver, name) {
    ;
}

void CColorRepresentationProtocol::bindManager(wl_client* client, void* data, uint32_t ver, uint32_t id) {
    const auto manager = m_managers.emplace_back(makeUnique<CWpColorRepresentationManagerV1>(client, ver, id)).get();
    if (!manager->resource()) {
        wl_client_post_no_memory(client);
        m_managers.pop_back();
        return;
    }

    manager->setDestroy([this](CWpColorRepresentationManagerV1* resource) { destroyResource(resource); });
    manager->setOnDestroy([this](CWpColorRepresentationManagerV1* resource) { destroyResource(resource); });
    manager->setGetSurface(
        [this](CWpColorRepresentationManagerV1* resource, uint32_t id, wl_resource* surface) { getSurface(resource, id, CWLSurfaceResource::fromResource(surface)); });

    manager->sendSupportedAlphaMode(WP_COLOR_REPRESENTATION_SURFACE_V1_ALPHA_MODE_PREMULTIPLIED_ELECTRICAL);
    for (const auto coefficients :
         {WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT601, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT709, WP_COLOR_REPRESENTATION_SURFACE_V1_COEFFICIENTS_BT2020}) {
        manager->sendSupportedCoefficientsAndRanges(coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_FULL);
        manager->sendSupportedCoefficientsAndRanges(coefficients, WP_COLOR_REPRESENTATION_SURFACE_V1_RANGE_LIMITED);
    }
    manager->sendDone();
}

void CColorRepresentationProtocol::getSurface(CWpColorRepresentationManagerV1* manager, uint32_t id, SP<CWLSurfaceResource> surface) {
    if (surface->m_colorRepresentation) {
        manager->error(WP_COLOR_REPRESENTATION_MANAGER_V1_ERROR_SURFACE_EXISTS, "Color representation already exists for this surface");
        return;
    }

    const auto resource =
        m_surfaces.emplace_back(makeShared<CColorRepresentationSurface>(makeShared<CWpColorRepresentationSurfaceV1>(manager->client(), manager->version(), id), surface));
    if (!resource->good()) {
        manager->noMemory();
        m_surfaces.pop_back();
        return;
    }

    surface->m_colorRepresentation = resource;
}

void CColorRepresentationProtocol::destroyResource(CWpColorRepresentationManagerV1* resource) {
    std::erase_if(m_managers, [resource](const auto& manager) { return manager.get() == resource; });
}

void CColorRepresentationProtocol::destroyResource(CColorRepresentationSurface* resource) {
    std::erase_if(m_surfaces, [resource](const auto& surface) { return surface.get() == resource; });
}
