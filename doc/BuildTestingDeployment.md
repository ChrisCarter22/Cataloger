# Build, Testing, Security, and Deployment

## Toolchains & Builds
- **Supported Compilers:** Clang 15+ (macOS/Xcode 15), MSVC 17.x (Visual Studio 2022), GCC/Clang 13+ (Linux).
- **CMake Presets:** Use the provided presets (`cmake --preset <platform>-debug|release`, `ninja` or `cmake --build --preset …`) to ensure consistent flags and dependency resolution (Qt6, Vulkan SDK, ICU, SQLite).
- **Formatting & Static Analysis:** `clang-format`, `clang-tidy` (modernize, cppcoreguidelines), and `cppcheck` must pass before review.

## Testing Expectations
- **Unit Tests:** GoogleTest for core logic, QtTest for UI components.
- **Integration Tests:** Headless suites validating ingest, metadata pipelines, filesystem/database writes, and XMP output.
- **Performance Tests:** Benchmark harness measuring preview load latency (<150 ms p95), navigation responsiveness (<16 ms per frame), and ingest throughput (≥250 MB/s SSD-bound).
- **Validation Checklist:** Every change builds Release + Debug, runs the entire automated test matrix, exercises smoke tests (ingest, preview, metadata, FTP), and attaches benchmark deltas to PRs.

## Strict Build & Test Rules
1. Use canonical CMake presets; ad-hoc builds are unsupported.
2. CI must pass on macOS, Windows, and Linux for Release + Debug, static analysis, tests, and packaging. Waivers require owner approval and linked issues.
3. Local developers run `ctest --preset all` and the perf harness before PR submission, capturing command outputs in the PR template.
4. Every feature/fix adds appropriate unit, QtTest, and integration/regression tests.
5. Performance-sensitive changes must include before/after benchmarks; regressions block merge until resolved or mitigated.
6. Release smoke tests run manually on macOS and Windows prior to tagging; sign-off is recorded in release notes.
7. Build artifacts undergo codesigning/notarization (macOS) or signtool verification (Windows) before publication; unsigned artifacts remain in private staging.
8. Dependency upgrades require a dedicated “toolchain update” proposal plus full validation of installers.

## Security & Privacy Policies
- **Credentials:** Store FTP/SFTP secrets using OS facilities (Keychain on macOS, Credential Manager/DPAPI on Windows). Never log plaintext secrets.
- **Metadata Sanitization:** Uploader can strip sensitive IPTC/XMP fields; warn users when personal data (contact info, GPS) is sent externally.
- **Filesystem Boundaries:** Watchers and ingest operate only on user-approved roots; services run with least-privilege permissions.
- **Network Transport:** Prefer SFTP/FTPS. Plain FTP is allowed only when explicitly required and must warn about insecurity.
- **Logging & Telemetry:** Redact credentials, PII, and sensitive file paths. Telemetry is strictly opt-in and anonymized.
- **Dependency Discipline:** Only install dependencies listed in the approved tech stack or explicitly required by tasks. All other packages need written owner approval.

## Deployment Targets & Packaging
- **Platforms:** macOS 13 Ventura & 14 Sonoma (Intel + Apple Silicon) and Windows 10/11 (22H2+) are tier-1. Linux builds are developer-only until packaging is finalized.
- **Packaging:** macOS releases ship as signed/notarized DMGs produced via `macdeployqt`, bundling MoltenVK, ICU, SQLite, and other required libs. Windows releases use MSIX/installers created with `windeployqt`, including Vulkan runtime redistributables and VC++ runtimes.
- **Dependencies in Installers:** Qt6 Widgets modules, ICU data, SQLite, libjpeg-turbo, libtiff, libheif, libde265, LittleCMS, Adobe XMP Toolkit, and Vulkan/MoltenVK components must ship with installers.
- **Updates & Rollback:** Publish versioned manifests, SHA-256 signatures, and changelog entries for each build. Allow rollback by reinstalling prior DMG/MSIX without manual cleanup.
- **CI Artifacts:** Release pipelines output notarized/signed builds plus symbol files (dSYM/PDB) for crash diagnostics.
