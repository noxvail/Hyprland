# Subsurface Recheck Damage Fix

The `fix/subsurface-recheck-damage` branch narrows `CSubsurface::recheckDamageForSubsurfaces()` so it only damages a child subsurface when that child's global geometry changed since the previous recheck.

## Why

Timing traces on `fix/v0.54.1-hdr-reference-from-sdr-max-clean` showed that one focused game subsurface was being re-damaged far more often than it actually committed. Those repeated same-geometry recheck damages fed `AQ_SCHEDULE_DAMAGE` and pushed DRM present cadence well above the game's real content cadence.

When the recheck damage path was fully disabled in a debug experiment, monitor Hz dropped back near the game's real FPS. A narrower follow-up experiment showed that allowing recheck damage only when the child's global box changed preserved that fix while keeping the original recheck traversal available for real geometry updates.

## What changed

- Each `CSubsurface` now caches the last global position and size used by recheck damage.
- `recheckDamageForSubsurfaces()` skips `damageSurface()` if the child's current global box matches the cached one.
- The cached recheck geometry is reset on map/unmap so newly visible surfaces still receive initial recheck damage.

This is intended as a production fix, not a debug toggle.
