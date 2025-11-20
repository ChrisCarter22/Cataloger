# Core Pipelines

## Preview Loading & Zoom Pipeline
1. **Fast Directory Scan**
   - Reads filenames/sizes/timestamps only to build an in-memory list (10k files in milliseconds) without touching RAW payloads.
2. **Multithreaded Preview Extraction**
   - Worker threads open RAW headers, locate embedded JPEG offsets, and copy preview bytes directly—no demosaicing required.
3. **In-Memory Caching**
   - Tier A RAM cache stores recently opened previews decompressed; Tier B preload cache speculatively loads neighbors (current ±2) to keep arrow navigation smooth.
4. **Color-Managed Display**
   - Embedded profiles flow through LittleCMS → monitor profile, then render via Metal (macOS, through MoltenVK) or native Vulkan (Windows/Linux).
5. **Lag-Free Zooming**
   - Zoom uses the embedded JPEG decoded once into RAM; GPU transforms handle scaling for instant 1:1 checks.

## Ingest Pipeline
1. **Source Selection**
   - Supports mounted disks (auto-detected card roots), arbitrary folders (recursive scan), or current contact-sheet selection (skip scanning).
2. **Incremental Ingest (Optional)**
   - Hidden ingest cache (timestamps/hashes) skips previously copied files, ideal for multi-card shoots or re-ingesting after breaks.
3. **Auto Ingest (Optional)**
   - Disk-mount listener starts ingest automatically with saved filters, renaming, metadata templates, and destinations.
4. **Filtering**
   - Locked/unlocked copy policies and RAW/JPEG/video filters; uses EXIF protection flags and MIME/extension checks, pairing RAW+JPEG via filename matching.
5. **Destinations & Structure**
   - Primary (mandatory) and optional secondary mirror destinations; templated folder logic (ignore, mirror, dated, job-specific) using tokens like `{year}`, `{job}`, `{sequence}`.
6. **Renaming**
   - Template-driven filenames leveraging metadata tokens; sequence numbers tracked per job or globally.
7. **Metadata Templates**
   - Applies IPTC/XMP templates (global or session-specific) during copy—RAW updates go to sidecars, JPEG/TIFF embed headers.
8. **Background Execution**
   - Copying, renaming, metadata writes, and mirroring run asynchronously, enabling live culling; destination folders can open mid-ingest for immediate previews.
9. **Post-Ingest Actions (Optional)**
   - Erase or unmount source disks, open contact sheets in the background.

## Metadata Templates
- **Global Templates:** Persistent `.IPT` presets for copyright/identity plus placeholders (`{year4}`, `{event}`, `{filenamebase}`, `{sequence}`).
- **Local Templates:** Session-only templates applied during ingest for assignment-specific data.
- **Per-Image Editing:** Metadata window shows previews, existing IPTC/XMP fields, and allows keyword drag/drop, captions, and template application per image; writes to XMP sidecars or JPEG/TIFF headers.
- **Field Map:** See `Metadata (IPTC) Template Field Map` section of Agents.md for variables and descriptions.

## FTP Uploader
1. **Selection & Launch**
   - Choose files in the contact sheet, then use `File → FTP Photos as…` or `File → Upload…`.
2. **Connection Profiles**
   - Select or create FTP/SFTP profiles (host, port, protocol, credentials, remote path, passive mode, key auth).
3. **Transfer Settings**
   - Decide original vs resized output, file type inclusion, metadata stripping/applying.
4. **Metadata Template**
   - Apply global or upload-specific templates before transfer.
5. **Execution & Monitoring**
   - Upload tasks show progress/errors; contact sheet badges indicate status (yellow uploading, green success, red failure). Failed transfers can be retried after fixing credentials or permissions.
6. **Capabilities**
   - Supports FTP (IPv6) and SFTP, saved connections, upload queues, and real-time status; live ingest→FTP automation remains manual unless external tooling is provided.
