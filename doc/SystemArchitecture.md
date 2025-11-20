# System Architecture

## Layered Overview
- **Presentation Layer (Qt6 Widgets):** Navigator, Contact Sheet, Viewer, Metadata panes, Navigator panel, and FTP dialogs share a Qt state store connected to backend services. Views subscribe to catalog/preview updates for immediate UI refreshes.
- **Application Core & Message Bus:** Central dispatcher/event loop routes ingest events, watcher notifications, metadata commits, and UI commands via typed signals/slots for deterministic ordering.
- **Catalog Service:** Owns the SQLite catalog (`catalog.db`), exposing async APIs for queries/mutations and emitting change deltas to the UI, preview cache, ingest service, and delivery stack.
- **Preview & Cache Service:** Handles directory scans, multithreaded embedded JPEG extraction, RAM/preload caches, and GPU presentation via Metal (macOS) or Vulkan (Windows/Linux).
- **Ingest Service:** Manages source detection, filtering, copy/mirroring, renaming, metadata application, incremental ingest state, and post-ingest actions; modeled as resumable job state machines.
- **Metadata Engine:** Provides global/local/per-image templates, IPTC/XMP editing, validation, and write-back to sidecars/headers while updating catalog metadata revisions.
- **Delivery Services:** FTP/SFTP uploader and future exporters use a shared transfer controller with queue tracking and Tasks UI integration.
- **Background Task Orchestrator:** Schedules ingest copies, watcher reconciliations, metadata writes, preview extraction, FTP transfers, catalog checkpoints, and integrity checks on prioritized thread pools.
- **Platform Abstraction Layer:** Wraps OS-specific APIs (FSEvents/ReadDirectoryChangesW/inotify, Metal, native Vulkan, Keychain/DPAPI, dialogs, threading) to keep higher layers portable.
- **Configuration & Secrets:** Settings service stores preferences, ingest presets, templates, and credentials (via OS secure stores). Handles feature flags and schema migrations.

## Catalog Persistence Model
- **Storage:** Single SQLite file per workspace, WAL mode with `synchronous=NORMAL`. Automatic checkpoints run hourly or when WAL exceeds 128 MB; startup executes `PRAGMA integrity_check`.
- **Roots & Paths:** `root_folders` map volume UUIDs + absolute paths to IDs, tracking last scan times and watcher tokens. `files` store relative paths so moving a root only updates one record.
- **Key Tables:**
  - `files` – metadata for each asset (paths, timestamps, ratings, stack IDs, hashes, preview state).
  - `metadata_blobs` – IPTC/XMP JSON plus update metadata.
  - `stacks` – group membership (pair/burst/manual) with anchored ordering.
  - `previews` – cache coordination data (cache keys, last render timestamps, tier states).
  - `sync_queue` – watcher events awaiting reconciliation.
- **Indices/Views:** Unique `(root_id, relative_path)` index; composite indices for capture time/ingest seq/ID and filename; materialized `current_sort_view` combining primary/secondary/tertiary sort keys.
- **Sync Flow:** Ingest transactions insert rows and enqueue watcher seeds; native watchers append to `sync_queue`, and a worker reconciles events, verifying mtimes/hashes before updating `files` and metadata. Fallback rescans compare directory listings against stored hashes when watchers fail.
- **Metadata Propagation:** Edits bump `metadata_rev`, update `metadata_blobs`, and trigger sidecar/header writes. Triggers ensure `files.metadata_rev` reflects on-disk freshness.
- **Recovery:** All write paths wrap in transactions with retry-on-busy logic; corruption triggers automatic backups and `.recover` attempts. Catalog backups (`.pmcatalog`) bundle the DB and preview manifest.

## Repository & Module Layout
- `src/app/` – Qt bootstrap, dependency injection container, view presenters (`navigator/`, `contact_sheet/`, `viewer/`, `metadata_panel/`).
- `src/services/`
  - `catalog/`, `preview/`, `ingest/`, `metadata/`, `delivery/`, `tasks/`.
- `src/platform/` – watchers, GPU bridges, credential adapters, dialogs, threading wrappers.
- `src/config/` – settings, presets, feature flags, schema migrations.
- `src/tests/`
  - `unit/` (GoogleTest + QtTest), `integration/` (headless ingest/metadata/FTP), `perf/` (preview/navigation/ingest benchmarks).
- `cmake/` & `scripts/` – toolchain files, presets, packaging helpers (`macdeployqt`, `windeployqt`).
- `resources/` – icons, translations, default templates, sample ingest presets.
- **Modularity Rules:** Interface headers in `include/` mirror implementation units; modules communicate via interfaces or the message bus to keep dependencies acyclic; CI builds/test modules independently (e.g., `libpreview`, `libingest`, `libcatalog`).
