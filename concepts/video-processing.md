# Video Processing Concepts

A reference guide for developers working with video processing pipelines using FFmpeg and related tools.

---

## Core Video Properties

### Resolution

Resolution defines the frame dimensions (width × height) in pixels.

| Name | Resolution | Aspect Ratio | Common Use |
|---|---|---|---|
| 240p | 426×240 | 16:9 | Low-quality streaming |
| 360p | 640×360 | 16:9 | Mobile streaming |
| 480p (SD) | 854×480 | 16:9 | Standard definition |
| 720p (HD) | 1280×720 | 16:9 | HD streaming |
| 1080p (FHD) | 1920×1080 | 16:9 | Full HD broadcast/streaming |
| 1440p (QHD) | 2560×1440 | 16:9 | Gaming monitors |
| 4K (UHD) | 3840×2160 | 16:9 | 4K streaming, cinema |
| 8K (FUHD) | 7680×4320 | 16:9 | Future displays |

```bash
# Resize to 1280x720 maintaining aspect ratio
ffmpeg -i input.mp4 -vf "scale=1280:720" output.mp4

# Resize to width 1280, auto-calculate height (keep aspect ratio)
ffmpeg -i input.mp4 -vf "scale=1280:-2" output.mp4

# Scale to 720p using force_original_aspect_ratio
ffmpeg -i input.mp4 -vf "scale=1280:720:force_original_aspect_ratio=decrease" output.mp4
```

---

### Frame Rate (FPS)

The number of frames displayed per second.

| FPS | Common Use |
|---|---|
| 23.976 | NTSC cinema (23.976 = 24000/1001) |
| 24 | Cinema |
| 25 | PAL broadcast |
| 29.97 | NTSC broadcast (30000/1001) |
| 30 | Web video, streaming |
| 50 | PAL high frame rate |
| 59.94 | NTSC high frame rate |
| 60 | Web streaming, gaming |
| 120, 240 | High-speed / slow-motion capture |

```bash
# Force output frame rate to 30fps
ffmpeg -i input.mp4 -r 30 output.mp4

# Convert 60fps to 30fps using frame decimation
ffmpeg -i input.mp4 -vf "fps=30" output.mp4
```

---

### Pixel Format (pix_fmt)

Defines how pixel data is stored in memory.

| Format | Description | Use Case |
|---|---|---|
| `yuv420p` | 8-bit YUV 4:2:0 | Standard web video (H.264/VP9) |
| `yuv422p` | 8-bit YUV 4:2:2 | Professional editing |
| `yuv444p` | 8-bit YUV 4:4:4 | High-quality editing, no chroma loss |
| `yuv420p10le` | 10-bit YUV 4:2:0 | HDR video (H.265, AV1) |
| `yuv444p10le` | 10-bit YUV 4:4:4 | Professional HDR |
| `rgb24` | 8-bit RGB | Processing, screenshots |
| `rgba` | 8-bit RGBA with alpha | Transparency support |
| `gbrp` | Planar GBR | Some screen recording codecs |

```bash
# Convert pixel format
ffmpeg -i input.mp4 -vf "format=yuv420p" output.mp4

# Encode to 10-bit for HDR
ffmpeg -i input.mp4 -c:v libx265 -pix_fmt yuv420p10le output.mp4
```

---

## Video Filtering

FFmpeg's `-vf` (video filter) and `-filter_complex` flags apply complex graph-based transformations.

### Scale & Resize

```bash
# Scale to 50% of original size
ffmpeg -i input.mp4 -vf "scale=iw/2:ih/2" output.mp4

# Scale to fit within 1920x1080 (letterbox/pillarbox if needed)
ffmpeg -i input.mp4 -vf "scale=1920:1080:force_original_aspect_ratio=decrease,pad=1920:1080:(ow-iw)/2:(oh-ih)/2" output.mp4

# Thumbnail: scale to 320px width, auto height
ffmpeg -i input.mp4 -vf "scale=320:-2" -frames:v 1 thumbnail.jpg
```

---

### Cropping

```bash
# Crop to center 1280x720 from a 1920x1080 source
ffmpeg -i input.mp4 -vf "crop=1280:720:320:180" output.mp4
# Syntax: crop=width:height:x_offset:y_offset

# Crop to 16:9 from arbitrary source
ffmpeg -i input.mp4 -vf "crop=ih*16/9:ih" output.mp4
```

---

### Padding / Letterboxing

```bash
# Add black letterbox bars to make 4:3 video fit 16:9
ffmpeg -i input_4x3.mp4 -vf "pad=iw:iw*9/16:(ow-iw)/2:(oh-ih)/2:black" output.mp4

# Pad to exact 1920x1080
ffmpeg -i input.mp4 -vf "pad=1920:1080:(1920-iw)/2:(1080-ih)/2" output.mp4
```

---

### Deinterlacing

Convert interlaced video (1080i, 480i) to progressive output.

```bash
# Yadif deinterlacer (most common)
ffmpeg -i interlaced.ts -vf "yadif" output.mp4

# Yadif: mode=1 for field-doubling (output at 2x fps)
ffmpeg -i interlaced.ts -vf "yadif=mode=1" output.mp4

# Bwdif (motion-compensated, better quality)
ffmpeg -i interlaced.ts -vf "bwdif" output.mp4
```

---

### Frame Rate Conversion

```bash
# Slow motion: stretch 60fps video to play at 30fps (2x slower)
ffmpeg -i input_60fps.mp4 -vf "setpts=2.0*PTS" -r 30 output.mp4

# Speed up: 2x speed
ffmpeg -i input.mp4 -vf "setpts=0.5*PTS" -r 60 output.mp4

# Motion interpolation using minterp filter
ffmpeg -i input_24fps.mp4 -vf "minterpolate=fps=60:mi_mode=mci" output_60fps.mp4
```

---

### Color Correction & Grading

```bash
# Adjust brightness, contrast, saturation
ffmpeg -i input.mp4 -vf "eq=brightness=0.06:contrast=1.2:saturation=1.5" output.mp4

# Color curves
ffmpeg -i input.mp4 -vf "curves=r='0/0 0.5/0.6 1/1':g='0/0 0.5/0.5 1/1'" output.mp4

# Hue rotation
ffmpeg -i input.mp4 -vf "hue=h=90:s=1" output.mp4

# Colorbalance (shadows/midtones/highlights)
ffmpeg -i input.mp4 -vf "colorbalance=rs=0.1:gs=-0.1:bs=0.1" output.mp4
```

---

### Overlay / Watermark

```bash
# Overlay a logo image (PNG with transparency) at top-right
ffmpeg -i input.mp4 -i logo.png \
  -filter_complex "overlay=W-w-10:10" \
  output.mp4

# Overlay at a specific time range (5s to 10s)
ffmpeg -i input.mp4 -i logo.png \
  -filter_complex "overlay=10:10:enable='between(t,5,10)'" \
  output.mp4

# Add text watermark
ffmpeg -i input.mp4 \
  -vf "drawtext=text='© 2024':fontcolor=white:fontsize=24:x=10:y=10:alpha=0.7" \
  output.mp4
```

---

### Text & Subtitles

```bash
# Burn in SRT subtitles (hard subtitles)
ffmpeg -i input.mp4 -vf "subtitles=subtitles.srt" output.mp4

# Burn in ASS subtitles with custom style
ffmpeg -i input.mp4 -vf "ass=subtitles.ass" output.mp4

# Add text with drawtext filter
ffmpeg -i input.mp4 \
  -vf "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf:\
text='Frame\: %{n}':fontcolor=white:fontsize=18:x=5:y=5" \
  output.mp4
```

---

### Thumbnail Extraction

```bash
# Extract a frame at 5 seconds
ffmpeg -ss 5 -i input.mp4 -frames:v 1 thumbnail.jpg

# Extract a frame every 10 seconds
ffmpeg -i input.mp4 -vf "fps=1/10" thumbnail_%04d.jpg

# Extract the best thumbnail (highest quality frame in first minute)
ffmpeg -i input.mp4 -vf "thumbnail,scale=320:-1" -frames:v 1 -ss 0 -t 60 best_thumbnail.jpg
```

---

### Concatenation

```bash
# Concat filter (for files with same codec/resolution)
ffmpeg -i part1.mp4 -i part2.mp4 \
  -filter_complex "[0:v][0:a][1:v][1:a]concat=n=2:v=1:a=1[v][a]" \
  -map "[v]" -map "[a]" output.mp4

# Concat demuxer (for re-encoding; all formats accepted)
# Create concat.txt:
# file 'part1.mp4'
# file 'part2.mp4'
ffmpeg -f concat -safe 0 -i concat.txt -c copy output.mp4
```

---

## Video Quality Metrics

### PSNR (Peak Signal-to-Noise Ratio)

Measures distortion between original and encoded video. Higher = better. >40 dB is generally excellent.

```bash
ffmpeg -i original.mp4 -i encoded.mp4 \
  -filter_complex "psnr" \
  -f null -
```

### SSIM (Structural Similarity Index)

Perceptual quality metric. Range: 0–1. >0.95 is excellent.

```bash
ffmpeg -i original.mp4 -i encoded.mp4 \
  -filter_complex "ssim" \
  -f null -
```

### VMAF (Video Multi-Method Assessment Fusion)

Netflix's perceptual quality metric. Score 0–100; >90 is excellent.

```bash
ffmpeg -i original.mp4 -i encoded.mp4 \
  -filter_complex "libvmaf=model_path=vmaf_v0.6.1.json" \
  -f null -
```

---

## HDR (High Dynamic Range) Processing

### HDR to SDR Tone Mapping

```bash
# Convert HDR10 (BT.2020, PQ) to SDR (BT.709)
ffmpeg -i hdr_input.mp4 \
  -vf "zscale=t=linear:npl=100,format=gbrpf32le,\
       zscale=p=bt709,tonemap=tonemap=hable:desat=0,\
       zscale=t=bt709:m=bt709:r=tv,format=yuv420p" \
  -c:v libx264 -crf 18 output_sdr.mp4
```

### SDR to HDR (HLG)

```bash
ffmpeg -i sdr_input.mp4 \
  -vf "zscale=t=arib-std-b67:p=bt2020:m=bt2020nc,format=yuv420p10le" \
  -c:v libx265 -x265-params "hdr-opt=1:repeat-headers=1:\
  colorprim=bt2020:transfer=arib-std-b67:colormatrix=bt2020nc" \
  output_hlg.mp4
```

---

## Adaptive Bitrate Streaming (ABR)

### ABR Ladder (Typical)

| Rendition | Resolution | Video Bitrate | Audio Bitrate |
|---|---|---|---|
| 240p | 426×240 | 200–400 kbps | 64 kbps |
| 360p | 640×360 | 400–800 kbps | 96 kbps |
| 480p | 854×480 | 800–1500 kbps | 128 kbps |
| 720p | 1280×720 | 1500–4000 kbps | 128 kbps |
| 1080p | 1920×1080 | 3000–8000 kbps | 192 kbps |
| 4K | 3840×2160 | 15000–40000 kbps | 256 kbps |

### HLS Multi-Bitrate Encoding

```bash
ffmpeg -i input.mp4 \
  -filter_complex \
    "[0:v]split=3[v1][v2][v3]; \
     [v1]scale=1280:720[v1out]; \
     [v2]scale=854:480[v2out]; \
     [v3]scale=640:360[v3out]" \
  -map "[v1out]" -c:v:0 libx264 -b:v:0 2800k \
  -map "[v2out]" -c:v:1 libx264 -b:v:1 1200k \
  -map "[v3out]" -c:v:2 libx264 -b:v:2 600k \
  -map 0:a -c:a aac -b:a 128k \
  -f hls -hls_time 6 -hls_playlist_type vod \
  -master_pl_name master.m3u8 \
  -hls_segment_filename "stream_%v/segment_%03d.ts" \
  -var_stream_map "v:0,a:0 v:1,a:0 v:2,a:0" \
  stream_%v/playlist.m3u8
```

---

## Encoding Performance Concepts

### Preset vs Quality Tradeoff

Encoding `preset` controls the speed-quality tradeoff. Slower presets achieve better compression at the same quality setting, but take longer.

| Preset | Speed | Compression Efficiency |
|---|---|---|
| `ultrafast` | Fastest | Worst |
| `superfast` | Very fast | Poor |
| `veryfast` | Fast | Fair |
| `faster` | Fast | Fair |
| `fast` | Fast | Good |
| `medium` | Balanced | Good (default) |
| `slow` | Slow | Better |
| `slower` | Slower | Better |
| `veryslow` | Slowest | Best |

### Hardware Acceleration (hwaccel)

```bash
# NVIDIA NVENC encoding
ffmpeg -i input.mp4 -c:v h264_nvenc -preset p6 -cq 23 output.mp4

# AMD AMF encoding
ffmpeg -i input.mp4 -c:v h264_amf -quality quality output.mp4

# Apple VideoToolbox (macOS)
ffmpeg -i input.mp4 -c:v h264_videotoolbox -b:v 4M output.mp4

# Intel QSV encoding
ffmpeg -i input.mp4 -c:v h264_qsv -global_quality 23 output.mp4

# VAAPI (Linux GPU acceleration)
ffmpeg -vaapi_device /dev/dri/renderD128 -i input.mp4 \
  -vf "format=nv12,hwupload" -c:v h264_vaapi -qp 23 output.mp4

# Hardware decoding + software encoding
ffmpeg -hwaccel cuda -i input.mp4 -c:v libx264 -crf 23 output.mp4
```

### Multi-threading

```bash
# Set encoding threads
ffmpeg -i input.mp4 -c:v libx264 -threads 8 output.mp4

# Set slice threading (for real-time applications)
ffmpeg -i input.mp4 -c:v libx264 -x264opts sliced_threads=1 output.mp4
```

---

## References

### FFmpeg Video Filtering
- **FFmpeg Filters Documentation** — https://ffmpeg.org/ffmpeg-filters.html
- **FFmpeg Filtering Guide (Wiki)** — https://trac.ffmpeg.org/wiki/FilteringGuide
- **scale filter** — https://ffmpeg.org/ffmpeg-filters.html#scale
- **crop filter** — https://ffmpeg.org/ffmpeg-filters.html#crop
- **pad filter** — https://ffmpeg.org/ffmpeg-filters.html#pad
- **overlay filter** — https://ffmpeg.org/ffmpeg-filters.html#overlay
- **drawtext filter** — https://ffmpeg.org/ffmpeg-filters.html#drawtext
- **yadif deinterlacer** — https://ffmpeg.org/ffmpeg-filters.html#yadif
- **bwdif deinterlacer** — https://ffmpeg.org/ffmpeg-filters.html#bwdif
- **thumbnail filter** — https://ffmpeg.org/ffmpeg-filters.html#thumbnail
- **Thumbnails how-to** — https://trac.ffmpeg.org/wiki/Create%20a%20thumbnail%20image%20every%20X%20seconds

### Quality Metrics
- **PSNR filter** — https://ffmpeg.org/ffmpeg-filters.html#psnr
- **SSIM filter** — https://ffmpeg.org/ffmpeg-filters.html#ssim
- **VMAF (Netflix)** — https://github.com/Netflix/vmaf
- **libvmaf filter** — https://ffmpeg.org/ffmpeg-filters.html#libvmaf
- **SSIM research paper** — Wang, Z. et al. "Image quality assessment: from error visibility to structural similarity" (2004): https://ece.uwaterloo.ca/~z70wang/research/ssim/

### HDR
- **ITU-R BT.2020** — https://www.itu.int/rec/R-REC-BT.2020/en
- **ITU-R BT.2100 (PQ and HLG)** — https://www.itu.int/rec/R-REC-BT.2100/en
- **SMPTE ST 2084 (PQ)** — https://ieeexplore.ieee.org/document/7291452
- **FFmpeg HDR/colorspace guide** — https://trac.ffmpeg.org/wiki/colorspace
- **zscale filter** — https://ffmpeg.org/ffmpeg-filters.html#zscale
- **tonemap filter** — https://ffmpeg.org/ffmpeg-filters.html#tonemap

### Hardware Acceleration
- **FFmpeg HW Accel Intro** — https://trac.ffmpeg.org/wiki/HWAccelIntro
- **NVIDIA NVENC/NVDEC** — https://trac.ffmpeg.org/wiki/HWAccelIntro#CUDA
- **VAAPI** — https://trac.ffmpeg.org/wiki/Hardware/VAAPI
- **VideoToolbox (macOS)** — https://trac.ffmpeg.org/wiki/HWAccelIntro#VideoToolbox
- **Intel QSV** — https://trac.ffmpeg.org/wiki/Hardware/QuickSync

### ABR Streaming
- **Apple HLS Authoring Specification** — https://developer.apple.com/documentation/http-live-streaming/hls-authoring-specification-for-apple-devices
- **DASH-IF Content Protection** — https://dashif.org/guidelines/
