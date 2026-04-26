# Streaming Concepts

A reference guide for developers working with media streaming protocols and adaptive bitrate delivery.

---

## Streaming Protocols Overview

| Protocol | Type | Latency | Transport | Use Case |
|---|---|---|---|---|
| HLS | ABR | 5–30s (LL-HLS: <2s) | HTTP | VOD and live streaming (Apple, browsers) |
| MPEG-DASH | ABR | 4–30s | HTTP | VOD and live (cross-platform) |
| RTMP | Push | 1–5s | TCP | Ingest (OBS → server) |
| RTSP | Pull/Push | <1s | UDP/TCP | IP cameras, surveillance |
| SRT | Push/Pull | <1s | UDP | Low-latency live contribution |
| WebRTC | P2P/SFU | <0.5s | UDP/DTLS | Real-time video calls |
| WHIP/WHEP | Push/Pull | <1s | HTTP/WebRTC | WebRTC-based ingest/egress |

---

## HLS (HTTP Live Streaming)

### Concepts

- Developed by Apple. Supported natively on iOS, Safari, and by most players via JS libraries.
- Media is divided into **segments** (typically 2–10 seconds) stored as MPEG-TS or fragmented MP4 files.
- A **playlist file** (`.m3u8`) lists segments and their durations.
- A **master playlist** references multiple variant playlists at different bitrates/resolutions for ABR.

### Playlist Structure

**Master playlist** (`master.m3u8`):
```
#EXTM3U
#EXT-X-VERSION:3

#EXT-X-STREAM-INF:BANDWIDTH=800000,RESOLUTION=640x360
360p/playlist.m3u8

#EXT-X-STREAM-INF:BANDWIDTH=2000000,RESOLUTION=1280x720
720p/playlist.m3u8

#EXT-X-STREAM-INF:BANDWIDTH=5000000,RESOLUTION=1920x1080
1080p/playlist.m3u8
```

**Variant playlist** (`720p/playlist.m3u8`):
```
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:6
#EXT-X-MEDIA-SEQUENCE:0

#EXTINF:6.000000,
segment_000.ts
#EXTINF:6.000000,
segment_001.ts
#EXTINF:6.000000,
segment_002.ts
#EXT-X-ENDLIST
```

### Generate HLS with FFmpeg

```bash
# Basic HLS output
ffmpeg -i input.mp4 \
  -c:v libx264 -crf 23 -preset fast \
  -c:a aac -b:a 128k \
  -f hls \
  -hls_time 6 \
  -hls_playlist_type vod \
  -hls_segment_filename "segments/segment_%03d.ts" \
  playlist.m3u8

# Multi-bitrate HLS with fMP4 segments
ffmpeg -i input.mp4 \
  -filter_complex "[0:v]split=2[v1][v2];[v1]scale=1280:720[v1out];[v2]scale=640:360[v2out]" \
  -map "[v1out]" -c:v:0 libx264 -b:v:0 2500k \
  -map "[v2out]" -c:v:1 libx264 -b:v:1 700k \
  -map 0:a -c:a aac -b:a 128k \
  -f hls \
  -hls_time 6 \
  -hls_segment_type fmp4 \
  -hls_playlist_type vod \
  -master_pl_name master.m3u8 \
  -var_stream_map "v:0,a:0 v:1,a:0" \
  -hls_segment_filename "stream_%v/seg_%03d.mp4" \
  stream_%v/playlist.m3u8

# Low-latency HLS (LL-HLS) live stream
ffmpeg -re -i input.mp4 \
  -c:v libx264 -b:v 2500k -g 60 -keyint_min 60 \
  -c:a aac -b:a 128k \
  -f hls \
  -hls_time 2 \
  -hls_flags delete_segments+split_by_time \
  -hls_playlist_type event \
  -hls_segment_filename "live/seg_%03d.ts" \
  live/playlist.m3u8
```

---

## MPEG-DASH (Dynamic Adaptive Streaming over HTTP)

### Concepts

- ISO standard. Container-agnostic. Uses fragmented MP4 (ISO BMFF) or WebM.
- Described by an **MPD** (Media Presentation Description) XML file.
- Client selects appropriate adaptation set (bitrate/resolution) based on bandwidth.

### Generate DASH with FFmpeg

```bash
# Basic DASH output
ffmpeg -i input.mp4 \
  -c:v libx264 -b:v 2500k \
  -c:a aac -b:a 128k \
  -f dash \
  -seg_duration 6 \
  -use_timeline 1 \
  -use_template 1 \
  manifest.mpd

# Multi-bitrate DASH
ffmpeg -i input.mp4 \
  -filter_complex "[0:v]split=2[v1][v2];[v1]scale=1280:720[v1out];[v2]scale=854:480[v2out]" \
  -map "[v1out]" -c:v:0 libx264 -b:v:0 2500k \
  -map "[v2out]" -c:v:1 libx264 -b:v:1 1000k \
  -map 0:a -c:a aac -b:a 128k \
  -f dash \
  -seg_duration 4 \
  -use_template 1 \
  -use_timeline 1 \
  -adaptation_sets "id=0,streams=v id=1,streams=a" \
  manifest.mpd
```

---

## RTMP (Real-Time Messaging Protocol)

### Concepts

- Developed by Adobe. Used primarily for **live stream ingest** (OBS → Wowza / Nginx-RTMP / cloud services).
- Operates over TCP (reliable delivery). Latency: 1–5 seconds.
- Typically transcoded server-side before HLS/DASH delivery.

### Stream to RTMP with FFmpeg

```bash
# Stream to RTMP server
ffmpeg -re -i input.mp4 \
  -c:v libx264 -b:v 3000k -g 60 -keyint_min 60 \
  -c:a aac -b:a 160k \
  -f flv \
  rtmp://live.server.com/live/stream_key

# Stream to YouTube Live
ffmpeg -re -i input.mp4 \
  -c:v libx264 -preset veryfast -b:v 4000k -maxrate 4000k -bufsize 8000k \
  -g 60 -keyint_min 60 \
  -c:a aac -b:a 192k -ar 48000 \
  -f flv \
  rtmp://a.rtmp.youtube.com/live2/YOUR_STREAM_KEY

# Stream a live camera feed
ffmpeg -f v4l2 -i /dev/video0 -f alsa -i default \
  -c:v libx264 -preset ultrafast -tune zerolatency -b:v 2000k \
  -c:a aac -b:a 128k \
  -f flv \
  rtmp://live.server.com/live/key
```

---

## SRT (Secure Reliable Transport)

### Concepts

- Open-source protocol by Haivision. Designed for low-latency, reliable transmission over unreliable networks.
- Built on UDP with ARQ (Automatic Repeat reQuest) for reliability.
- Supports encryption (AES-128/256).
- Latency: 80–400ms (configurable).

### SRT with FFmpeg

```bash
# SRT receiver (listener mode)
ffmpeg -i "srt://0.0.0.0:1935?mode=listener&latency=200" \
  -c copy output.mp4

# SRT sender (caller mode)
ffmpeg -re -i input.mp4 \
  -c:v libx264 -b:v 4000k \
  -c:a aac -b:a 128k \
  -f mpegts \
  "srt://192.168.1.100:1935?mode=caller&latency=200"

# SRT with encryption
ffmpeg -re -i input.mp4 \
  -c copy -f mpegts \
  "srt://server:1935?passphrase=my_secret_password&pbkeylen=16"
```

---

## RTSP (Real Time Streaming Protocol)

### Concepts

- Used by IP cameras, surveillance systems, and media servers.
- Control protocol for initiating/controlling RTP media streams.
- Supports both TCP and UDP transport.

### RTSP with FFmpeg

```bash
# Read from RTSP camera
ffmpeg -rtsp_transport tcp -i rtsp://user:pass@192.168.1.100/stream \
  -c copy output.mp4

# Read RTSP and re-stream to RTMP
ffmpeg -rtsp_transport tcp -i rtsp://camera:554/stream \
  -c:v libx264 -preset ultrafast -b:v 2000k \
  -c:a aac -b:a 128k \
  -f flv rtmp://server/live/key

# Serve RTSP stream (using FFmpeg + ffserver or MediaMTX)
ffmpeg -re -i input.mp4 -c copy -f rtsp rtsp://localhost:8554/stream
```

---

## Adaptive Bitrate Streaming (ABR) Concepts

### How ABR Works

1. **Encoding**: Source is encoded at multiple bitrate/resolution combinations (the "bitrate ladder").
2. **Segmentation**: Each encoded version is split into fixed-duration segments (typically 2–10s).
3. **Manifest**: A text file (`.m3u8` for HLS, `.mpd` for DASH) describes available segments.
4. **Client**: Player monitors download bandwidth and buffer level, switching between renditions dynamically.
5. **CDN**: Segments and manifests served via HTTP CDN for global scale.

### Buffer-Based vs. Rate-Based Adaptation

| Strategy | Description | Behavior |
|---|---|---|
| Rate-based | Choose bitrate based on measured download speed | Simple; can oscillate |
| Buffer-based (BOLA) | Choose bitrate based on current buffer level | Stable; prevents rebuffering |
| Hybrid (DYNAMIC) | Combination of rate and buffer signals | Best real-world performance |

### Segment Duration Tradeoffs

| Duration | Latency | Startup Time | ABR Response | Server Load |
|---|---|---|---|---|
| 2s | Low | Fast | Fast | High |
| 4–6s | Medium | Medium | Medium | Medium |
| 10s | High | Slow | Slow | Low |

---

## Live Streaming Pipeline

### Typical Architecture

```
Camera/Source
    ↓
[Encoder/Broadcast Software] (OBS, FFmpeg, hardware encoder)
    ↓ RTMP/SRT
[Ingest Server] (Nginx-RTMP, Wowza, cloud)
    ↓
[Transcoder] (FFmpeg, cloud transcoder)
    ↓
[Packager] (HLS/DASH segmentation)
    ↓
[Origin Storage] (S3, NFS)
    ↓
[CDN] (CloudFront, Fastly, Akamai)
    ↓
[Player] (Video.js, HLS.js, Shaka Player)
```

### FFmpeg Live Transcoding + Packaging

```bash
# Receive RTMP, transcode, and package to HLS
ffmpeg -listen 1 -i rtmp://0.0.0.0:1935/live/stream \
  -filter_complex "[0:v]split=2[v1][v2];[v1]scale=1280:720[v1out];[v2]scale=854:480[v2out]" \
  -map "[v1out]" -c:v:0 libx264 -b:v:0 2500k -preset fast \
  -map "[v2out]" -c:v:1 libx264 -b:v:1 1000k -preset fast \
  -map 0:a -c:a aac -b:a 128k \
  -f hls -hls_time 4 -hls_flags delete_segments \
  -master_pl_name master.m3u8 \
  -var_stream_map "v:0,a:0 v:1,a:0" \
  -hls_segment_filename "/var/www/hls/stream_%v/seg_%03d.ts" \
  /var/www/hls/stream_%v/playlist.m3u8
```

---

## DRM (Digital Rights Management)

### Common DRM Systems

| DRM | Platform | Container |
|---|---|---|
| Widevine | Chrome, Android, Smart TVs | DASH + CENC |
| PlayReady | Windows, Xbox, Edge | DASH + CENC, Smooth Streaming |
| FairPlay | Apple (Safari, iOS, macOS) | HLS + AES-128 |

### HLS AES-128 Encryption

```bash
# Generate AES-128 key
openssl rand 16 > enc.key

# Create key info file (keyinfo.txt):
# https://example.com/enc.key   <- URL where player fetches key
# enc.key                        <- local path to key file
# $(openssl rand -hex 16)       <- IV (optional)

# Encrypt HLS segments
ffmpeg -i input.mp4 \
  -c:v libx264 -c:a aac \
  -f hls \
  -hls_key_info_file keyinfo.txt \
  -hls_segment_filename "seg_%03d.ts" \
  playlist.m3u8
```

---

## Streaming Quality of Service (QoS) Metrics

| Metric | Description | Good Value |
|---|---|---|
| Startup time | Time from play request to first frame | < 3s |
| Buffering ratio | % of playback time spent buffering | < 0.5% |
| Rebuffering events | Count of interruptions during playback | 0–1 per hour |
| Bitrate switches | Number of quality level changes | < 5 per hour |
| Average bitrate | Mean bitrate received during playback | > 80% of target |
| Error rate | % of failed manifest/segment requests | < 0.1% |
| CDN cache hit rate | % of requests served from cache | > 95% |

---

## Streaming Tools & Libraries

### Server-Side

| Tool | Purpose |
|---|---|
| **Nginx-RTMP** | RTMP ingest + HLS/DASH packaging |
| **MediaMTX** (formerly rtsp-simple-server) | Multi-protocol media server (RTMP, RTSP, SRT, WebRTC, HLS) |
| **Wowza Streaming Engine** | Enterprise streaming server |
| **Shaka Packager** | DASH/HLS packaging with DRM support |
| **Bento4** | MP4 tools for DASH/HLS packaging |
| **FFmpeg** | Swiss-army knife for transcoding and packaging |

### Client-Side (JavaScript)

| Library | Protocol | License |
|---|---|---|
| **HLS.js** | HLS | Apache 2.0 |
| **Shaka Player** | HLS + DASH + DRM | Apache 2.0 |
| **dash.js** | MPEG-DASH | Apache 2.0 |
| **Video.js** | Multiple (with plugins) | Apache 2.0 |
| **Plyr** | Multiple | MIT |
