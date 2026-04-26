# FFmpeg Modules & Libraries

A reference guide for the core libraries and components that make up the FFmpeg framework.

---

## FFmpeg Component Overview

FFmpeg is composed of several command-line tools and shared libraries:

| Component | Type | Purpose |
|---|---|---|
| `ffmpeg` | CLI tool | Transcode, convert, filter, and stream media |
| `ffprobe` | CLI tool | Inspect and analyze media files |
| `ffplay` | CLI tool | Media player (using SDL) |
| `libavcodec` | Library | Encoding and decoding |
| `libavformat` | Library | Container muxing and demuxing |
| `libavfilter` | Library | Audio/video filtering |
| `libavdevice` | Library | Input/output from hardware devices |
| `libavutil` | Library | Core utilities (math, logging, pixel formats, etc.) |
| `libswscale` | Library | Image scaling and pixel format conversion |
| `libswresample` | Library | Audio resampling, mixing, and format conversion |
| `libpostproc` | Library | Post-processing filters (legacy) |

---

## Core Libraries

### libavcodec

The encoding and decoding library. Contains hundreds of codec implementations.

**Key components:**
- `AVCodec` — Codec descriptor (encoder or decoder)
- `AVCodecContext` — Codec configuration and state
- `AVPacket` — Encoded data (compressed)
- `AVFrame` — Decoded data (raw pixels or samples)

**Common codecs available:**

| Codec | Encoder Name | Decoder Name | Notes |
|---|---|---|---|
| H.264 | `libx264` | `h264` | Requires libx264 |
| H.265/HEVC | `libx265` | `hevc` | Requires libx265 |
| VP8 | `libvpx` | `vp8` | Requires libvpx |
| VP9 | `libvpx-vp9` | `vp9` | Requires libvpx |
| AV1 | `libsvtav1`, `libaom-av1`, `librav1e` | `libaom-av1` | Multiple options |
| AAC | `aac`, `libfdk_aac` | `aac` | `libfdk_aac` = higher quality |
| MP3 | `libmp3lame` | `mp3` | Requires libmp3lame |
| Opus | `libopus` | `libopus` | Requires libopus |
| Vorbis | `libvorbis` | `vorbis` | Requires libvorbis |
| FLAC | `flac` | `flac` | Built-in |
| PCM s16 | `pcm_s16le` | `pcm_s16le` | Built-in |

```bash
# List all available encoders
ffmpeg -encoders

# List all available decoders
ffmpeg -decoders

# Show capabilities of a specific codec
ffmpeg -h encoder=libx264
ffmpeg -h decoder=h264
```

---

### libavformat

Handles container format muxing (writing) and demuxing (reading).

**Key components:**
- `AVFormatContext` — Container state and metadata
- `AVStream` — A single media stream within a container
- `AVInputFormat` — Demuxer descriptor
- `AVOutputFormat` — Muxer descriptor

**Common formats:**

| Format | Muxer | Demuxer | Notes |
|---|---|---|---|
| MP4 | `mp4` | `mov,mp4,m4a,3gp,3g2,mj2` | With `-movflags +faststart` for web |
| Matroska | `matroska` | `matroska,webm` | Universal container |
| WebM | `webm` | `matroska,webm` | Web-optimized |
| HLS | `hls` | `hls` | HTTP Live Streaming |
| MPEG-DASH | `dash` | — | Dynamic Adaptive Streaming |
| RTMP | `flv` | `flv` | Live ingest |
| MPEG-TS | `mpegts` | `mpegts` | Broadcast, HLS segments |
| Ogg | `ogg` | `ogg` | Container for Vorbis/Opus/Theora |
| FLV | `flv` | `flv` | Flash Video |
| WAV | `wav` | `wav` | Uncompressed audio |

```bash
# List all available muxers
ffmpeg -muxers

# List all available demuxers
ffmpeg -demuxers

# Show options for a specific format
ffmpeg -h muxer=mp4
ffmpeg -h demuxer=matroska
```

---

### libavfilter

Graph-based audio and video filtering. All `-vf`, `-af`, and `-filter_complex` operations use this library.

**Key concepts:**
- `AVFilterGraph` — A collection of connected filter nodes
- `AVFilter` — A single filter definition
- `AVFilterContext` — Instantiated filter with its configuration
- `AVFilterLink` — A connection between two filter pads

```bash
# List all available filters
ffmpeg -filters

# Show details of a specific filter
ffmpeg -h filter=scale
ffmpeg -h filter=loudnorm
```

**Common filter categories:**

| Category | Examples |
|---|---|
| Scale / Geometry | `scale`, `crop`, `pad`, `rotate`, `transpose` |
| Color / Grade | `eq`, `curves`, `hue`, `colorbalance`, `lut3d` |
| Frame Rate | `fps`, `setpts`, `minterpolate` |
| Overlay / Text | `overlay`, `drawtext`, `drawbox`, `subtitles` |
| Deinterlace | `yadif`, `bwdif` |
| Stack / Tile | `hstack`, `vstack`, `xstack` |
| Analysis | `psnr`, `ssim`, `blackdetect`, `scdet` |
| Volume | `volume`, `loudnorm`, `dynaudnorm` |
| Dynamics | `acompressor`, `alimiter`, `agate` |
| EQ | `equalizer`, `bass`, `treble`, `highpass`, `lowpass` |
| Effects | `aecho`, `chorus`, `afftdn` |
| Channels | `pan`, `channelsplit`, `amerge`, `amix` |
| Speed/Pitch | `atempo`, `asetrate` |
| Visualization | `showwaves`, `showspectrum`, `ebur128` |

---

### libswscale

Image scaling and pixel format conversion.

**Capabilities:**
- Scale video frames to any resolution.
- Convert between pixel formats (e.g., `yuv420p` → `rgb24`).
- Apply various scaling algorithms.

**Scaling algorithms (`flags`):**

| Flag | Algorithm | Quality | Speed |
|---|---|---|---|
| `fast_bilinear` | Fast bilinear | Fair | Very fast |
| `bilinear` | Bilinear | Good | Fast |
| `bicubic` | Bicubic | Better | Medium |
| `neighbor` | Nearest neighbor | Poor | Fastest |
| `area` | Area (box) | Good for downscale | Medium |
| `lanczos` | Lanczos | Best | Slow |
| `spline` | Spline | Very good | Slow |

```bash
ffmpeg -i input.mp4 -vf "scale=1280:720:flags=lanczos" output.mp4
```

---

### libswresample

Audio resampling and format conversion.

**Capabilities:**
- Convert sample rate (e.g., 44100 → 48000 Hz).
- Convert sample format (e.g., `s16` → `fltp`).
- Convert channel layout (e.g., stereo → mono, 5.1 → stereo).
- Multiple resampling algorithms.

**Resampling engines:**

| Engine | Quality | Notes |
|---|---|---|
| `swr` (default) | Good | Built-in |
| `soxr` | Excellent | Requires SoX Resampler library |

```bash
# Use SoX high-quality resampler
ffmpeg -i input.wav -af "aresample=48000:resampler=soxr:precision=28" output.wav
```

---

### libavdevice

Input/output from hardware devices and system streams.

**Common input devices:**

| Device | Platform | Description |
|---|---|---|
| `v4l2` | Linux | Video4Linux2 — webcams, capture cards |
| `alsa` | Linux | ALSA audio capture |
| `pulse` | Linux | PulseAudio input |
| `avfoundation` | macOS | Camera + screen capture |
| `dshow` | Windows | DirectShow (webcams, microphones) |
| `gdigrab` | Windows | GDI screen capture |
| `x11grab` | Linux | X11 screen capture |
| `lavfi` | All | Virtual input from filter graph |

```bash
# List available devices
ffmpeg -f v4l2 -list_formats all -i /dev/video0   # Linux webcam
ffmpeg -f avfoundation -list_devices true -i ""   # macOS

# Capture from webcam (Linux)
ffmpeg -f v4l2 -framerate 30 -video_size 1280x720 -i /dev/video0 \
  -f alsa -i default output.mp4

# Capture screen (macOS)
ffmpeg -f avfoundation -i "1:0" -vcodec libx264 -preset ultrafast screen.mp4

# Capture screen (Linux X11)
ffmpeg -f x11grab -r 30 -s 1920x1080 -i :0.0 output.mp4
```

---

### libavutil

Core utility library used by all other FFmpeg components.

**Key utilities:**

| Component | Purpose |
|---|---|
| `AVDictionary` | Key-value metadata storage |
| `AVLog` | Logging infrastructure |
| `AVRational` | Fractional arithmetic (timebase, frame rate) |
| `AVPixFmtDescriptor` | Pixel format descriptions |
| `AVChannelLayout` | Audio channel layout definitions |
| `av_rescale_q()` | Rescale timestamps between timebases |
| `av_gettime()` | Get monotonic clock time |
| `av_malloc()` / `av_free()` | Memory allocation aligned for SIMD |

---

## External Codec Libraries

These are third-party libraries that FFmpeg can link against to support additional codecs:

### Video Encoders

| Library | Codec | Notes |
|---|---|---|
| `libx264` | H.264 | Most widely used. GPL license |
| `libx265` | H.265/HEVC | GPL license |
| `libvpx` | VP8/VP9 | BSD license (Google) |
| `libaom` | AV1 | BSD license (Alliance for Open Media) |
| `libsvtav1` | AV1 | BSD+Patent license (Intel, Netflix) |
| `librav1e` | AV1 | BSD license (Xiph/Mozilla) |
| `libtheora` | Theora | BSD license |
| `libwebp` | WebP | BSD license |

### Audio Encoders

| Library | Codec | Notes |
|---|---|---|
| `libfdk_aac` | AAC | Fraunhofer AAC encoder; best quality; requires separate compilation |
| `libmp3lame` | MP3 | LGPL license |
| `libopus` | Opus | BSD license |
| `libvorbis` | Vorbis | BSD license |
| `libspeex` | Speex | BSD license (voice codec) |

### Special Purpose Libraries

| Library | Purpose |
|---|---|
| `libass` | ASS/SSA subtitle rendering |
| `zlib` | DEFLATE compression (used in PNG, ZIP) |
| `libbluray` | Blu-ray disc reading |
| `libsrt` | SRT protocol support |
| `libzmq` | ZeroMQ messaging (for `zmq` filter) |
| `librubberband` | High-quality pitch shifting |

---

## Hardware Acceleration APIs

| API | Platforms | Description |
|---|---|---|
| `cuda` / `nvenc` / `nvdec` | NVIDIA GPU | CUDA-based encode/decode |
| `amf` | AMD GPU | Advanced Media Framework |
| `videotoolbox` | macOS/iOS | Apple hardware codec |
| `vaapi` | Linux (Intel/AMD/NVIDIA) | VA-API GPU acceleration |
| `qsv` | Intel | Intel Quick Sync Video |
| `dxva2` | Windows | DirectX Video Acceleration 2 |
| `d3d11va` | Windows | Direct3D 11 Video Acceleration |
| `opencl` | Cross-platform | GPU-based filters |
| `vulkan` | Cross-platform | New GPU acceleration API |

```bash
# List all available hardware acceleration methods
ffmpeg -hwaccels

# List hardware encoders
ffmpeg -encoders | grep -E "nvenc|amf|videotoolbox|qsv|vaapi|cuvid"
```

---

## FFmpeg Build Configuration

Check what libraries and features are compiled into your FFmpeg build:

```bash
# Show build configuration
ffmpeg -buildconf

# Show version and enabled libraries
ffmpeg -version

# Check if specific codec/format is available
ffmpeg -codecs | grep libx264
ffmpeg -formats | grep hls
```

### Typical Full-Featured Build Flags

```
--enable-gpl
--enable-nonfree           # for libfdk_aac
--enable-libx264
--enable-libx265
--enable-libvpx
--enable-libopus
--enable-libfdk-aac
--enable-libmp3lame
--enable-libvorbis
--enable-libass
--enable-libfreetype
--enable-libsvtav1
--enable-libaom
--enable-libsrt
--enable-nvenc             # NVIDIA GPU
--enable-vaapi             # Linux GPU
--enable-videotoolbox      # macOS
```

---

## References

### Library Documentation
- **libavformat** — https://ffmpeg.org/doxygen/trunk/group__libavf.html
- **libavcodec** — https://ffmpeg.org/doxygen/trunk/group__lavc.html
- **libavfilter** — https://ffmpeg.org/doxygen/trunk/group__lavfi.html
- **libavutil** — https://ffmpeg.org/doxygen/trunk/group__lavu.html
- **libswscale** — https://ffmpeg.org/doxygen/trunk/group__libsws.html
- **libswresample** — https://ffmpeg.org/doxygen/trunk/group__lavu__sampmanip.html
- **libavdevice** — https://ffmpeg.org/doxygen/trunk/group__lavd.html
- **AVCodecContext fields** — https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
- **AVStream** — https://ffmpeg.org/doxygen/trunk/structAVStream.html
- **AVPacket** — https://ffmpeg.org/doxygen/trunk/structAVPacket.html
- **AVFrame** — https://ffmpeg.org/doxygen/trunk/structAVFrame.html

### Codec Libraries
- **x264** — https://www.videolan.org/developers/x264.html
- **x265** — https://x265.readthedocs.io/en/stable/
- **libvpx (VP8/VP9)** — https://www.webmproject.org/code/
- **libaom (AV1)** — https://aomedia.googlesource.com/aom
- **SVT-AV1** — https://github.com/AOMediaCodec/SVT-AV1
- **rav1e** — https://github.com/xiph/rav1e
- **libfdk-aac** — https://github.com/mstorsjo/fdk-aac
- **libopus** — https://opus-codec.org

### Hardware Acceleration
- **NVENC/NVDEC** — https://developer.nvidia.com/nvidia-video-codec-sdk
- **AMF** — https://github.com/GPUOpen-LibrariesAndSDKs/AMF
- **VideoToolbox** — https://developer.apple.com/documentation/videotoolbox
- **VAAPI** — https://intel.github.io/libva/
- **Intel QSV** — https://www.intel.com/content/www/us/en/architecture-and-technology/quick-sync-video/quick-sync-video-general.html
- **FFmpeg HW Accel Intro** — https://trac.ffmpeg.org/wiki/HWAccelIntro

### Build Guides
- **Compilation Guide (Ubuntu)** — https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu
- **Compilation Guide (macOS)** — https://trac.ffmpeg.org/wiki/CompilationGuide/macOS
- **Compilation Guide (Windows)** — https://trac.ffmpeg.org/wiki/CompilationGuide/MinGW
