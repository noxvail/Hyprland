#pragma once

#include "WaylandProtocol.hpp"
#include "color-representation-v1.hpp"
#include "../helpers/signal/Signal.hpp"

class CWLSurfaceResource;

class CColorRepresentationSurface {
  public:
    CColorRepresentationSurface(SP<CWpColorRepresentationSurfaceV1> resource, SP<CWLSurfaceResource> surface);
    ~CColorRepresentationSurface();

    bool good();

  private:
    bool                                surfaceAlive();
    void                                damageSurface();
    void                                validatePending();

    SP<CWpColorRepresentationSurfaceV1> m_resource;
    WP<CWLSurfaceResource>              m_surface;
    CHyprSignalListener                 m_surfacePrecommit;
    CHyprSignalListener                 m_surfaceDestroy;
};

class CColorRepresentationProtocol : public IWaylandProtocol {
  public:
    CColorRepresentationProtocol(const wl_interface* iface, const int& ver, const std::string& name);

    void bindManager(wl_client* client, void* data, uint32_t ver, uint32_t id) override;

  private:
    void                                             getSurface(CWpColorRepresentationManagerV1* manager, uint32_t id, SP<CWLSurfaceResource> surface);
    void                                             destroyResource(CWpColorRepresentationManagerV1* resource);
    void                                             destroyResource(CColorRepresentationSurface* resource);

    std::vector<UP<CWpColorRepresentationManagerV1>> m_managers;
    std::vector<SP<CColorRepresentationSurface>>     m_surfaces;

    friend class CColorRepresentationSurface;
};

namespace PROTO {
    inline UP<CColorRepresentationProtocol> colorRepresentation;
}
