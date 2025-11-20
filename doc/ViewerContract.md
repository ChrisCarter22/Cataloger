# Viewer Contract (Preview Pipeline → Viewer)

This document specifies the minimal interface between the preview pipeline and the UI Viewer. The goal is to let the preview service (Metal on macOS first) deliver color-managed textures without leaking platform-specific details into higher layers. Windows/Linux ports can adopt the same API with different backends (Vulkan, D3D) later.

## Texture Formats
- **macOS (v1):** BGRA8 (unsigned, premultiplied alpha). All Metal uploads publish BGRA8 textures.
- **Future Windows/Linux:** RGBA8 (Vulkan/D3D) to match cross-platform expectations.
- Additional formats (10-bit, HDR) can be negotiated later via `FrameContext::format`.

## Sync & Frame Lifecycle
- `beginFrame()` signals the start of a presentation cycle. The viewer receives a `FrameContext` describing the requested size/scale and a `TextureHandle` (native Metal texture in v1).
- `submitFrame()` records the copy/blit from the provided texture into the viewer’s drawable. GPU errors are reported immediately and surfaced up the stack.
- `present()` flushes the command buffer and presents vsync-aligned frames. If vsync is disabled, the call still drains the GPU pipeline but does not block on display sync.

## Presentation & HiDPI
- Coordinate space is device-independent pixels. `FrameContext::scale` encodes the HiDPI scale factor (1.0 for standard, 2.0 for Retina). The viewer updates its layer/drawable size accordingly.
- Vsync defaults to enabled (`FrameContext::vsync = true`). Future viewers may toggle it for benchmarking.

## Platform Targets
- **Mac v1:** Implemented via `MetalViewer`, which owns a `CAMetalLayer`, command queue, and drawable lifecycle. It accepts BGRA8 textures from the preview service/offscreen pipeline and blits them into the drawable before presentation.
- **Windows/Linux future:** `VulkanViewer` / `D3DViewer` stubs compile today and log “not implemented” while honoring the same contract. When the Windows port begins, the stub can be swapped with a real implementation without touching UI/business logic.

## Data Structures
- `viewer::TextureHandle` wraps the native handle (`id<MTLTexture>` on macOS) plus backend metadata.
- `viewer::FrameContext` carries dimensions, scale, vsync flag, an optional color-space tag (e.g., `"sRGB IEC61966-2.1"`), and the texture handle.
- `viewer::Viewer` defines `beginFrame()`, `submitFrame()`, and `present()`; concrete backends must honor this contract.

Keeping the interface narrow allows the preview pipeline, UI presenters, and future Windows ports to evolve independently while sharing the same surface contract.
