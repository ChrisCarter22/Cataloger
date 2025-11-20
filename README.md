# Cataloger

Cataloger is a high-performance, JPEG-driven, metadata-aware image browser engineered for instant culling. It displays embedded camera previews rather than rendering RAW pixels, prioritizing speed while maintaining full color accuracy through LittleCMS and Metal/Vulkan pipelines. Key capabilities include:

- Instant embedded preview loading with multithreaded extraction and aggressive caching.
- Metadata-centric workflows (IPTC/XMP, RAW+JPEG pairing, live rating/tagging, auto-advance).
- Dual-mode UI (contact sheet + viewer) sharing a common preview cache for seamless navigation.
- Robust ingest engine with templated destinations, renaming, metadata application, and optional auto-ingest.
- Delivery pathways via FTP/SFTP uploader with saved connections and metadata templates.

Project documentation lives under `/doc`, covering product overview, system architecture, pipelines, and build/testing/deployment policies. Agent-facing rules remain in `Agents.md` (excluded from version control).

## Building & Testing
Follow the strict build/test rules defined in `/doc/BuildTestingDeployment.md`:
1. Configure the project using the provided CMake presets (`cmake --preset macos-debug`, `cmake --preset macos-release`, and analogous `linux-*`/`windows-*` presets).
2. Build with `ninja` (or `cmake --build --preset …`).
3. Run the automated suites with `ctest --preset all` (macOS Debug binaries) plus the performance harness.
4. Ensure clang-format/clang-tidy/cppcheck are clean before submitting changes.

### Quick Start
```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset all
```
Use the `*-release` presets for optimized builds and the `linux-*` / `windows-*` presets on their respective hosts. All presets live in `CMakePresets.json` and reference toolchains under `cmake/toolchains/`.

## Repository Structure
- `src/app/` – Application shell and entry point (currently a stub wired to the placeholder services).
- `src/services/` – Module-per-library scaffolding for Catalog, Preview, Ingest, Metadata, Delivery, and Tasks services.
- `src/platform/` – Host detection and OS abstraction stubs.
- `src/config/` – Settings/bootstrap configuration placeholders.
- `.github/workflows/super-linter.yml` – GitHub Actions workflow running [Super Linter](https://github.com/super-linter/super-linter) on pushes/PRs for repo-wide lint coverage.
- `src/tests/unit/` – GoogleTest suites (e.g., catalog and preview services).
- `src/tests/perf/` – Lightweight performance harnesses (preview pipeline latency, etc.) invoked via `ctest`.
- `third_party/adobe_xmp/` – Adobe XMP Toolkit SDK (vendored from github.com/adobe/XMP-Toolkit-SDK) used for IPTC/XMP serialization.
- `doc/` – Product and architecture documentation.
- `Agents.md` – Internal agent rules (ignored from Git).

Refer to `Agents.md` for the exhaustive implementation roadmap, coding standards, security constraints, and workflows.
