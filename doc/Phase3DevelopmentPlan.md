# Phase 3 Development Plan

This document breaks the Phase 3 (“Preview Pipeline”) effort into focused tasks that can be executed sequentially. Each section outlines scope, dependencies, deliverables, and open questions so work can be scheduled without ambiguity.

## 1. Metal Renderer Implementation

**Goal:** Replace the placeholder Metal bridge with a production-ready upload/command pipeline the future Qt Viewer can consume.

### Scope
- Create `src/platform/gpu/metal/` with:
  - Device/command queue management (lifetime + recreation on device loss).
  - Texture pool that ingests the color-managed preview buffer (RGB) and uploads it via blit/compute passes.
  - Fences/synchronization primitives so the UI knows when a texture is safe to use.
  - Error propagation (device loss, command buffer failures) surfaced through `GpuBridge`.
- Define the handoff contract to the Viewer (e.g., texture ID + semaphore handle).

### Dependencies
- Requires agreement with UI/Renderer team on texture format, colorspace, and synchronization model.
- Needs sample UI harness (or mock tests) to validate texture presentation.

### Deliverables
- New Metal module with unit/integration coverage.
- Updated `platform/gpu/GpuBridgeFactory` to instantiate the Metal backend.
- Documentation describing the rendering contract and error handling.
- Vulkan backend remains a stub logging “not implemented” (future Windows/Linux task).

### Open Questions
- What texture format/stride does the Viewer expect (RGBA8, BGRA8, etc.)?
- Should we manage double-buffered textures per preview, or rely on a shared pool?
- How do we communicate GPU fences/semaphores to Qt without MetalKit?

## 2. Embedded ICC Parsing & Color Metadata

**Goal:** Automatically extract ICC profiles from JPEG/RAW payloads (not just sidecars), feed them to LittleCMS, and expose the active profile name to the UI.

### Scope
- Evaluate libraries:
  - JPEG: libjpeg-turbo for decoding APP2 ICC chunks.
  - RAW metadata: Exiv2 (or similar) to read embedded profiles from RAW containers.
- Extend PreviewService extraction to:
  - Parse APP2/EXIF data when scanning files.
  - Prefer embedded ICC bytes; fall back to sidecars, then sRGB.
- Update `PreviewImage` to store source profile names and display them in metadata panels.

### Dependencies
- New third-party libs may require vetting (license/size).
- Need sample JPEG/RAW assets with embedded ICC profiles for tests.
- Must integrate with existing LittleCMS path without regressing performance.

### Deliverables
- Dependency proposal + build configuration updates.
- PreviewService changes that read ICC data automatically.
- Unit/integration tests using sample files (fixtures).
- Documentation describing the ICC search order (embedded → sidecar → sRGB) and UI exposure.

### Open Questions
- Where do we store sample ICC-enabled assets (test fixture repo vs. mocked data)?
- Do we cache parsed ICC metadata in the catalog to avoid repeated parsing?
- How do we handle exotic profiles (16-bit, cLab, etc.)?

## 3. UI Event Wiring & Perf Regression Hooks

**Goal:** Pipe PreviewService cache events into the Navigator/Contact Sheet/Viewer once those presenters exist, and enforce performance budgets in CI.

### Scope
- Implement Qt presenters that subscribe to PreviewService events:
  - Cache hits → remove loading spinners.
  - Cache misses → show “loading” state.
  - GPU errors → surface visible warnings/badges.
- Ensure neighbor preloading remains ±2 items and is reflected in UI state.
- Expand the perf harness to log GPU upload timings and navigation latency, then integrate thresholds into CI (fail builds when budgets regress).

### Dependencies
- Requires Navigator/Contact Sheet/Viewer code (currently not implemented).
- Needs a reference dataset and scripted runner to gather perf metrics.
- CI infrastructure must support GPU timing harnesses (macOS runner with Metal access).

### Deliverables
- Presenter implementations (Navigator, Contact Sheet, Viewer) reacting to cache events.
- Perf scripts that log warm/request timings and compare against baselines.
- CI job definitions and documentation showing how to run the perf suite locally.
- README/Agents updates confirming Metal is the active backend and describing the UI behavior.

### Open Questions
- How do we store/display perf baselines (JSON artifact, dashboard, etc.)?
- What’s the UX for GPU errors (banner, toast, log entry)?
- Which dataset do we standardize on for perf budgets?

## Summary

Phase 3 spans rendering, metadata, UI, and performance. Each task above can be owned independently once the corresponding dependencies (renderer contract, metadata parser, UI presenters, perf infrastructure) are ready. Completing these will finalize the preview pipeline on macOS with Metal, while Vulkan remains a future Windows/Linux milestone. Continuous documentation updates (README, Agents, wiki) should accompany each task to keep the team aligned.
