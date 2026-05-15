# Containers & Codecs

A comprehensive reference for media container formats and codecs used in video and audio processing.

---

## Container Formats

Containers wrap one or more media streams (video, audio, subtitles, metadata) into a single file. The container format is **independent** of the codec used inside it.

### MP4 / MPEG-4 Part 14 (`.mp4`, `.m4v`, `.m4a`)

| Property | Details |
|---|---|
| Extension | `.mp4`, `.m4v`, `.m4a` |
| FFmpeg muxer | `mp4` |
| Widely supported | Yes — universal browser, device, and platform support |
| Streaming | Progressive download, DASH, HLS (fMP4) |
| Common codecs | H.264, H.265, AV1, AAC, MP3, AC-3 |
| Seeks | Fast (moov atom; use `-movflags +faststart` for web) |

**Key notes:**
- `moov` atom must be at the beginning of the file for progressive streaming. Use `-movflags +faststart` in FFmpeg.
- Fragmented MP4 (`fMP4`) is used in DASH and HLS and allows live streaming.

```bash
# Create web-optimized MP4 with faststart
ffmpeg -i input.mkv -c:v libx264 -c:a aac -movflags +faststart output.mp4
```

---

### MKV / Matroska (`.mkv`, `.mka`, `.mks`)

| Property | Details |
|---|---|
| Extension | `.mkv`, `.mka`, `.mks` |
| FFmpeg muxer | `matroska` |
| Widely supported | Desktop players; limited browser support |
| Streaming | Limited |
| Common codecs | Any (very permissive) |
| Seeks | Good |

**Key notes:**
- Open standard. Supports virtually every codec.
- Excellent for archival and offline storage.
- Supports multiple audio tracks, subtitle tracks, attachments, chapters.

```bash
# Remux MP4 to MKV without re-encoding
ffmpeg -i input.mp4 -c copy output.mkv
```

---

### WebM (`.webm`)

| Property | Details |
|---|---|
| Extension | `.webm` |
| FFmpeg muxer | `webm` |
| Widely supported | All modern browsers |
| Streaming | DASH |
| Common codecs | VP8, VP9, AV1 (video); Vorbis, Opus (audio) |
| Seeks | Good |

**Key notes:**
- Royalty-free container and codecs. Designed for the web.
- Google-backed open standard.

```bash
# Encode to VP9/Opus WebM
ffmpeg -i input.mp4 -c:v libvpx-vp9 -crf 33 -b:v 0 -c:a libopus output.webm
```

---

### MOV / QuickTime (`.mov`)

| Property | Details |
|---|---|
| Extension | `.mov` |
| FFmpeg muxer | `mov` |
| Widely supported | macOS/iOS native; limited elsewhere |
| Streaming | Limited |
| Common codecs | ProRes, H.264, H.265, AAC, PCM |

**Key notes:**
- Native Apple format. Shares structure with MP4.
- Apple ProRes is a professional intermediate codec stored in MOV.

```bash
# Export Apple ProRes 422
ffmpeg -i input.mp4 -c:v prores_ks -profile:v 2 -c:a pcm_s16le output.mov
```

---

### MPEG-TS / Transport Stream (`.ts`, `.m2ts`)

| Property | Details |
|---|---|
| Extension | `.ts`, `.m2ts` |
| FFmpeg muxer | `mpegts` |
| Widely supported | Broadcast, HLS |
| Streaming | Designed for live streaming and broadcast |
| Common codecs | H.264, H.265, MPEG-2 Video, AAC, AC-3, MP2 |

**Key notes:**
- Designed for lossy/unreliable transmission. Error-resilient.
- Fixed 188-byte packets.
- HLS uses MPEG-TS segments.

```bash
# Segment video to HLS using MPEG-TS
ffmpeg -i input.mp4 -c:v libx264 -c:a aac -f hls -hls_time 6 output.m3u8
```

---

### AVI (`.avi`)

| Property | Details |
|---|---|
| Extension | `.avi` |
| FFmpeg muxer | `avi` |
| Widely supported | Legacy |
| Streaming | Poor |
| Common codecs | DivX, Xvid, H.264, MP3, PCM |

**Key notes:**
- Legacy format with limited modern use. Poor support for modern codecs and HDR.

---

### FLV / Flash Video (`.flv`, `.f4v`)

| Property | Details |
|---|---|
| Extension | `.flv` |
| FFmpeg muxer | `flv` |
| Widely supported | Legacy; RTMP streaming |
| Common codecs | H.264, AAC, MP3 |

**Key notes:**
- Still used in RTMP live streaming input.

---

## Video Codecs

### H.264 / AVC (Advanced Video Coding)

| Property | Details |
|---|---|
| Standard | ITU-T H.264 / ISO MPEG-4 Part 10 |
| FFmpeg encoder | `libx264` |
| FFmpeg decoder | `h264` (built-in) |
| License | Patent-encumbered (royalties for encoding/distribution) |
| Common containers | MP4, MKV, MOV, TS |

**Key encoding parameters:**
```bash
ffmpeg -i input.mp4 \
  -c:v libx264 \
  -crf 23 \            # Quality (18–28 typical; lower = better)
  -preset slow \       # Encoding speed (ultrafast→veryslow)
  -profile:v high \    # Baseline | Main | High
  -level 4.1 \         # Compatibility constraint
  output.mp4
```

**Profiles:**
- `baseline` – Mobile devices, low latency
- `main` – Standard devices
- `high` – Best quality, modern devices

---

### H.265 / HEVC (High Efficiency Video Coding)

| Property | Details |
|---|---|
| Standard | ITU-T H.265 / ISO MPEG-H Part 2 |
| FFmpeg encoder | `libx265` |
| FFmpeg decoder | `hevc` (built-in) |
| License | Patent-encumbered |
| Common containers | MP4, MKV, MOV |

**Key notes:**
- ~50% better compression than H.264 at the same quality.
- Higher encoding complexity and CPU usage.
- Supported on most modern devices and browsers.

```bash
ffmpeg -i input.mp4 \
  -c:v libx265 \
  -crf 28 \            # Quality (24–32 typical)
  -preset slow \
  -tag:v hvc1 \        # Required for Apple device compatibility
  output.mp4
```

---

### VP9

| Property | Details |
|---|---|
| Developer | Google |
| FFmpeg encoder | `libvpx-vp9` |
| License | Royalty-free |
| Common containers | WebM, MP4 (rare) |

**Key notes:**
- ~40% better than VP8. Comparable to H.265.
- Preferred for YouTube and web video.
- Two-pass encoding recommended for best quality.

```bash
# Two-pass VP9 encoding
ffmpeg -i input.mp4 -c:v libvpx-vp9 -b:v 0 -crf 33 -pass 1 -an -f null /dev/null
ffmpeg -i input.mp4 -c:v libvpx-vp9 -b:v 0 -crf 33 -pass 2 -c:a libopus output.webm
```

---

### AV1

| Property | Details |
|---|---|
| Developer | Alliance for Open Media (AOM) |
| FFmpeg encoders | `libaom-av1`, `libsvtav1`, `librav1e` |
| License | Royalty-free |
| Common containers | WebM, MP4, MKV |

**Key notes:**
- ~30% better than VP9/H.265. The next-generation codec.
- Very slow software encoding; hardware acceleration becoming available.
- `libsvtav1` (SVT-AV1) offers much faster encoding than `libaom-av1`.

```bash
# AV1 encoding with SVT-AV1 (faster)
ffmpeg -i input.mp4 -c:v libsvtav1 -crf 35 -preset 6 -c:a libopus output.mp4
```

---

### VP8

| Property | Details |
|---|---|
| Developer | Google |
| FFmpeg encoder | `libvpx` |
| License | Royalty-free |
| Common containers | WebM |

Legacy predecessor to VP9. Still used for older browsers.

---

### MPEG-2 Video

| Property | Details |
|---|---|
| FFmpeg encoder | `mpeg2video` |
| Common containers | MPEG-TS, MPEG-PS, VOB |

Used in DVD, Blu-ray, and broadcast. Legacy format.

---

### ProRes (Apple ProRes)

| Property | Details |
|---|---|
| FFmpeg encoder | `prores_ks`, `prores` |
| Common containers | MOV |

Professional intermediate/editing codec. Not for distribution. Variants: 422 Proxy, 422 LT, 422, 422 HQ, 4444, 4444 XQ.

---

### DNxHD / DNxHR (Avid)

| Property | Details |
|---|---|
| FFmpeg encoder | `dnxhd` |
| Common containers | MOV, MXF |

Professional intermediate codec used in Avid editing workflows.

---

## Audio Codecs

### AAC (Advanced Audio Coding)

| Property | Details |
|---|---|
| FFmpeg encoder | `aac` (native), `libfdk_aac` (higher quality) |
| License | Patent-encumbered |
| Common containers | MP4, MOV, ADTS |
| Typical bit rates | 96–320 kbps |

The standard audio codec for MP4 files and streaming. Transparent quality at ~128–192 kbps stereo.

```bash
ffmpeg -i input.mp3 -c:a aac -b:a 192k output.m4a
```

---

### Opus

| Property | Details |
|---|---|
| FFmpeg encoder | `libopus` |
| License | Royalty-free |
| Common containers | WebM, MKV, OGG |
| Typical bit rates | 32–256 kbps |

Excellent quality at low bit rates. Preferred for WebRTC, WebM, and VoIP.

```bash
ffmpeg -i input.wav -c:a libopus -b:a 128k output.opus
```

---

### MP3 (MPEG-1 Audio Layer 3)

| Property | Details |
|---|---|
| FFmpeg encoder | `libmp3lame` |
| License | Patents expired |
| Common containers | MP3 (raw), MP4, AVI |
| Typical bit rates | 128–320 kbps |

```bash
ffmpeg -i input.wav -c:a libmp3lame -b:a 320k output.mp3
```

---

### Vorbis

| Property | Details |
|---|---|
| FFmpeg encoder | `libvorbis` |
| License | Royalty-free |
| Common containers | OGG, WebM |

Open-source codec. Largely superseded by Opus.

---

### AC-3 / Dolby Digital

| Property | Details |
|---|---|
| FFmpeg encoder | `ac3` |
| License | Patent-encumbered |
| Common containers | MP4, MKV, TS |
| Channels | Up to 5.1 |

Required for DVD and some broadcast workflows.

---

### EAC-3 / Dolby Digital Plus (E-AC-3)

| Property | Details |
|---|---|
| FFmpeg encoder | `eac3` |
| Channels | Up to 7.1 |

Used in streaming services (Netflix, Amazon Prime). Better than AC-3.

---

### FLAC (Free Lossless Audio Codec)

| Property | Details |
|---|---|
| FFmpeg encoder | `flac` |
| License | Royalty-free |
| Common containers | FLAC, MKV, OGG |

Lossless audio compression. Used for archival and mastering.

```bash
ffmpeg -i input.wav -c:a flac output.flac
```

---

### PCM (Pulse-Code Modulation)

Uncompressed audio. Multiple variants:
- `pcm_s16le` – 16-bit signed little-endian (CD quality)
- `pcm_s24le` – 24-bit signed little-endian (professional)
- `pcm_f32le` – 32-bit float little-endian (processing)

```bash
# Extract audio as uncompressed PCM WAV
ffmpeg -i input.mp4 -c:a pcm_s16le output.wav
```

---

## Codec Selection Guide

| Use Case | Video Codec | Audio Codec | Container |
|---|---|---|---|
| Web (maximum compatibility) | H.264 | AAC | MP4 |
| Web (modern browsers) | AV1 or VP9 | Opus | WebM |
| Streaming (HLS) | H.264 or H.265 | AAC | MPEG-TS / fMP4 |
| DASH streaming | H.264/H.265/AV1 | AAC/Opus | fMP4 / WebM |
| Desktop playback | H.265 | AAC | MP4 or MKV |
| Professional editing | ProRes / DNxHR | PCM | MOV / MXF |
| Archival | H.265 or AV1 | FLAC | MKV |
| Low-latency live streaming | H.264 (baseline, low GOP) | AAC | MPEG-TS (RTMP) |
| VoIP / WebRTC | VP8/H.264 | Opus | — |

---

## References

### Container Formats
- **MP4 / ISO BMFF Standard** — ISO/IEC 14496-12: https://www.iso.org/standard/83102.html
- **Matroska Specification** — https://matroska.org/technical/specs/index.html; IETF RFC 9559: https://www.rfc-editor.org/rfc/rfc9559
- **WebM Container Guidelines** — https://www.webmproject.org/docs/container/
- **MPEG-TS Standard** — ISO/IEC 13818-1: https://www.iso.org/standard/74427.html
- **FFmpeg MP4 Muxer** — https://ffmpeg.org/ffmpeg-formats.html#mp4
- **FFmpeg Matroska Muxer** — https://ffmpeg.org/ffmpeg-formats.html#matroska
- **`-movflags +faststart` explained** — https://trac.ffmpeg.org/wiki/Encode/H.264#faststartforwebvideo

### Video Codecs
- **H.264 Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/H.264
- **H.264 Standard (ITU-T H.264)** — https://www.itu.int/rec/T-REC-H.264/en
- **H.265/HEVC Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/H.265
- **H.265 Standard (ITU-T H.265)** — https://www.itu.int/rec/T-REC-H.265/en
- **VP9 Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/VP9
- **VP9 Spec** — https://www.webmproject.org/vp9/
- **AV1 Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/AV1
- **AV1 Specification** — https://aomediacodec.github.io/av1-spec/
- **x264 Options Reference** — https://www.videolan.org/developers/x264.html
- **x265 Options Reference** — https://x265.readthedocs.io/en/stable/

### Audio Codecs
- **AAC Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/AAC
- **AAC Standard (MPEG-4)** — ISO/IEC 14496-3: https://www.iso.org/standard/43345.html
- **Opus RFC** — IETF RFC 6716: https://datatracker.ietf.org/doc/html/rfc6716
- **MP3 Encoding Guide** — https://trac.ffmpeg.org/wiki/Encode/MP3
- **FLAC Format Specification** — https://xiph.org/flac/format.html
