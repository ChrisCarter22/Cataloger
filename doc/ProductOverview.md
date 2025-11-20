# Cataloger Product Overview

## Mission & Philosophy
- Build a high-performance, JPEG-driven, metadata-aware image browser optimized for instant culling.
- Never render RAW pixels directly; always show the embedded camera-generated preview to prioritize speed over per-pixel fidelity.
- Deliver pro-grade workflows for sports, news, and wire-service photographers who must ingest, cull, rate, and tag thousands of images without delay.
- Maintain full color accuracy by honoring embedded profiles and running everything through LittleCMS before display.

## Core Characteristics
- **Instant Embedded Preview Loading:** Reads RAW containers (CR3, NEF, ARW, etc.), extracts the full-resolution embedded JPEG, and displays it immediately—no demosaicing, GPU color transforms, or heavy CPU work.
- **Zero RAW Processing:** The viewer never debayers pixels or applies tone curves/color profiles; it relies entirely on embedded previews or (optionally) Adobe DNG Converter when users request a slower RAW→JPEG path.
- **Color-Managed Pipeline:** Each preview leverages its embedded profile, which is converted via LittleCMS to the monitor profile before rendering through Metal on macOS. GPU backends for Windows/Linux will be introduced in a future port.
- **Metadata-Centric Experience:** XMP drives IPTC captions, ratings, tags, and RAW+JPEG sync; sidecar reads/writes keep metadata in lockstep with the high-speed viewer.
- **Fast Navigation & Caching:** Aggressive preloading (adjacent images, predictive read-ahead, RAM caches) keeps arrow-key navigation buttery smooth, even in huge folders.
- **Multi-Core Thumbnail Extraction:** Directory browsing spawns multi-threaded JPEG extraction so thumbnails appear without blocking the UI.
- **No Develop Module:** Cataloger deliberately excludes RAW editing sliders; the application focuses on ingesting, culling, tagging, metadata management, and organization.
- **Stacking Logic:** RAW+JPEG pairing, burst/sequence detection, and user-defined stacks collapse into logical groups with shared metadata updates.
- **Predictable Sorting:** User-selected primary keys (capture time, filename, rating, etc.) combine with deterministic secondary/tertiary tie-breakers to keep massive sessions ordered.

## Navigation Surfaces
### Contact Sheet
- Virtualized thumbnail grid rendering only visible rows while preloading the next two rows.
- Each cell shows the preview plus rating stars, color labels, tags/checkmarks, sequence numbers, and optional RAW+JPEG labels.
- Selection model mirrors newsroom tools: click, Shift+click (range), Cmd/Ctrl+click (toggle), marquee selection, and Select All.
- Keyboard navigation uses arrow keys (cells), Page Up/Down (viewport), Home/End (sheet bounds); space/enter opens the Viewer while maintaining grid selection.
- Ratings (numeric keys) and color labels (e.g., 8=Pink, 9=Red) update instantly; Auto-Advance mode shifts focus after each metadata action.

### Full Viewer
- Displays one image with color-managed rendering, preloading neighbors for near-zero latency.
- Supports Shift+Arrow multi-select continuation, Up Arrow to return to the contact sheet, and synchronized metadata panels (IPTC, XMP, EXIF) that update without blocking navigation.
- Zooming operates solely on the embedded JPEG decoded once into RAM; GPU transforms handle zooming for lag-free 1:1 checks.

### Navigator Panel
- Folder tree (volumes, standard locations, Photos Library, user folders) with favorites and a search field.
- Selecting a folder refreshes the contact sheet instantly while preserving per-folder scroll and last-selected state.
- Tasks area displays ingest/copy/metadata jobs; panel avoids decoding previews to stay responsive for deep folder structures.

## Navigation State & Context
- Navigation stack tracks folder, filters, sort order, selected items, Anchor for Shift+select, and last-viewed file to enable undo-like context restoration.
- Filtering preserves original order while maintaining valid selections within the filtered subset.
- Folder switches remember scroll position and reopen the last viewed file for continuity.

## User Workflows
- **Ingest → Contact Sheet:** As soon as ingest begins, the destination folder can open and show thumbnails in real time, enabling simultaneous culling.
- **Metadata Editing:** Layered approach—global templates set identity, local ingest templates add assignment context, per-image edits capture player names/actions with live previews.
- **Delivery:** FTP/SFTP uploader leverages saved connections, optional resizing/metadata templates, upload queue monitoring, and colored status badges in the contact sheet.
