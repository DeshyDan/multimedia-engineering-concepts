# FFmpeg Command Reference

A comprehensive reference for common FFmpeg commands used in video and audio processing.

---

## FFmpeg Basics

### Command Structure

```
ffmpeg [global_options] [input_options] -i input [output_options] output
```

Key concepts:
- **Global options** affect overall behavior (e.g., `-y` to overwrite output).
- **Input options** apply to the next `-i` input file.
- **Output options** apply to the next output file.
- Multiple `-i` inputs and multiple output files are supported.

### Essential Flags

| Flag | Description |
|---|---|
| `-i <file>` | Input file |
| `-y` | Overwrite output without asking |
| `-n` | Do not overwrite output (fail if exists) |
| `-v quiet` | Suppress output |
| `-v verbose` | Verbose output |
| `-progress pipe:1` | Machine-readable progress to stdout |
| `-stats` | Print encoding statistics |
| `-hide_banner` | Suppress FFmpeg version banner |
| `-loglevel error` | Only show errors |

---

## Probing / Inspecting Media

### ffprobe — Inspect media files

```bash
# Show all streams info (human readable)
ffprobe -v quiet -show_streams -pretty input.mp4

# JSON output (useful for scripting)
ffprobe -v quiet -print_format json -show_format -show_streams input.mp4

# Show only video stream info
ffprobe -v quiet -select_streams v:0 -show_entries \
  stream=codec_name,width,height,r_frame_rate,bit_rate \
  -print_format json input.mp4

# Get duration in seconds
ffprobe -v quiet -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 input.mp4

# Get all format tags (metadata)
ffprobe -v quiet -print_format json -show_entries format_tags input.mp4

# Get frame count
ffprobe -v quiet -select_streams v:0 \
  -count_frames -show_entries stream=nb_read_frames \
  -print_format csv input.mp4
```

---

## Stream Copying (Remuxing)

Copying streams avoids re-encoding — very fast, lossless, no quality change.

```bash
# Remux (copy all streams to new container)
ffmpeg -i input.mkv -c copy output.mp4

# Copy video, re-encode audio
ffmpeg -i input.mkv -c:v copy -c:a aac -b:a 192k output.mp4

# Extract video stream only (no audio)
ffmpeg -i input.mp4 -c:v copy -an video_only.mp4

# Extract audio stream only
ffmpeg -i input.mp4 -vn -c:a copy audio_only.aac

# Trim without re-encoding
ffmpeg -ss 00:01:00 -to 00:03:00 -i input.mp4 -c copy trimmed.mp4

# Trim (input seeking — fast, may be inaccurate at non-keyframe positions)
ffmpeg -ss 60 -i input.mp4 -t 120 -c copy output.mp4
```

---

## Transcoding

### Basic Transcode

```bash
# Convert MKV to MP4 (H.264 + AAC)
ffmpeg -i input.mkv \
  -c:v libx264 -crf 23 -preset medium \
  -c:a aac -b:a 192k \
  output.mp4

# Convert to H.265/HEVC
ffmpeg -i input.mp4 \
  -c:v libx265 -crf 28 -preset slow \
  -c:a copy \
  output_hevc.mp4

# Convert to VP9 + Opus (WebM)
ffmpeg -i input.mp4 \
  -c:v libvpx-vp9 -crf 33 -b:v 0 \
  -c:a libopus -b:a 128k \
  output.webm

# Convert to AV1 (slow but excellent quality)
ffmpeg -i input.mp4 \
  -c:v libsvtav1 -crf 35 -preset 6 \
  -c:a libopus -b:a 128k \
  output_av1.mp4
```

### Two-Pass Encoding

Two-pass encoding produces more accurate bit rate control, useful for fixed-size delivery.

```bash
# H.264 two-pass at 2000 kbps
ffmpeg -i input.mp4 -c:v libx264 -b:v 2000k -pass 1 -an -f null /dev/null
ffmpeg -i input.mp4 -c:v libx264 -b:v 2000k -pass 2 -c:a aac -b:a 128k output.mp4
```

---

## Cutting & Trimming

```bash
# Trim from 1 minute to 3 minutes (re-encode for accuracy)
ffmpeg -i input.mp4 -ss 00:01:00 -to 00:03:00 \
  -c:v libx264 -c:a aac output.mp4

# Trim using input seeking (fast, seeks to nearest keyframe before -ss)
ffmpeg -ss 00:01:00 -i input.mp4 -to 00:02:00 -c copy output.mp4

# Trim first 30 seconds
ffmpeg -i input.mp4 -t 30 -c copy output.mp4

# Remove first 10 seconds
ffmpeg -ss 10 -i input.mp4 -c copy output.mp4
```

---

## Audio Operations

```bash
# Add audio to silent video
ffmpeg -i video.mp4 -i audio.mp3 -c:v copy -c:a aac -shortest output.mp4

# Replace audio in video
ffmpeg -i video.mp4 -i new_audio.wav \
  -map 0:v -map 1:a -c:v copy -c:a aac output.mp4

# Remove audio from video
ffmpeg -i input.mp4 -c:v copy -an output.mp4

# Convert audio format
ffmpeg -i input.wav -c:a libmp3lame -b:a 320k output.mp3

# Adjust audio volume
ffmpeg -i input.mp4 -af "volume=2.0" output.mp4

# Normalize audio
ffmpeg -i input.mp4 -af "loudnorm=I=-14:LRA=11:TP=-1.5" output.mp4
```

---

## Image / Thumbnail Operations

```bash
# Extract single frame at timestamp
ffmpeg -ss 00:00:05 -i input.mp4 -frames:v 1 -q:v 2 thumbnail.jpg

# Extract frames every 1 second
ffmpeg -i input.mp4 -vf "fps=1" frame_%04d.jpg

# Extract keyframes only
ffmpeg -i input.mp4 -vf "select='eq(pict_type,I)'" -vsync vfr frame_%04d.jpg

# Create video from images
ffmpeg -framerate 24 -pattern_type glob -i "frames/*.jpg" \
  -c:v libx264 -pix_fmt yuv420p output.mp4

# Create GIF from video (with palette for quality)
ffmpeg -i input.mp4 -ss 5 -t 3 \
  -filter_complex "[0:v] fps=15,scale=480:-1,split [a][b];[a] palettegen [p];[b][p] paletteuse" \
  output.gif
```

---

## Filters & Processing

```bash
# Resize
ffmpeg -i input.mp4 -vf "scale=1280:720" output.mp4

# Rotate
ffmpeg -i input.mp4 -vf "rotate=PI/2" output.mp4  # 90 degrees

# Flip horizontal
ffmpeg -i input.mp4 -vf "hflip" output.mp4

# Flip vertical
ffmpeg -i input.mp4 -vf "vflip" output.mp4

# Transpose (rotate 90° clockwise)
ffmpeg -i input.mp4 -vf "transpose=1" output.mp4
# 0=ccw+flip, 1=cw, 2=ccw, 3=cw+flip

# Fade in/out
ffmpeg -i input.mp4 -vf "fade=t=in:st=0:d=2,fade=t=out:st=8:d=2" output.mp4

# Stack videos horizontally
ffmpeg -i left.mp4 -i right.mp4 \
  -filter_complex "[0:v][1:v]hstack=inputs=2[v]" \
  -map "[v]" output.mp4

# Stack videos vertically
ffmpeg -i top.mp4 -i bottom.mp4 \
  -filter_complex "[0:v][1:v]vstack=inputs=2[v]" \
  -map "[v]" output.mp4
```

---

## Concatenation

```bash
# Concat via filter_complex (re-encodes, accepts any inputs)
ffmpeg -i part1.mp4 -i part2.mp4 -i part3.mp4 \
  -filter_complex "[0:v][0:a][1:v][1:a][2:v][2:a]concat=n=3:v=1:a=1[v][a]" \
  -map "[v]" -map "[a]" output.mp4

# Concat demuxer (stream copy, fastest, requires same codec/resolution)
# Create list.txt:
# file 'part1.mp4'
# file 'part2.mp4'
# file 'part3.mp4'
ffmpeg -f concat -safe 0 -i list.txt -c copy output.mp4
```

---

## Streaming

```bash
# Stream to RTMP
ffmpeg -re -i input.mp4 \
  -c:v libx264 -preset veryfast -b:v 3000k -g 60 \
  -c:a aac -b:a 128k \
  -f flv rtmp://server/live/key

# Stream to HLS (live)
ffmpeg -re -i input.mp4 \
  -c:v libx264 -b:v 2500k -g 50 \
  -c:a aac -b:a 128k \
  -f hls -hls_time 4 -hls_flags delete_segments \
  http://server/hls/stream.m3u8

# UDP/RTP output
ffmpeg -re -i input.mp4 \
  -c:v libx264 -b:v 2000k \
  -c:a aac -b:a 128k \
  -f mpegts udp://239.0.0.1:1234?pkt_size=1316
```

---

## Hardware Acceleration

```bash
# List available hardware encoders
ffmpeg -encoders | grep -E "nvenc|amf|videotoolbox|qsv|vaapi"

# NVIDIA NVENC (GPU encode)
ffmpeg -i input.mp4 \
  -c:v h264_nvenc -preset p6 -rc vbr -cq 23 \
  -c:a copy output.mp4

# Apple VideoToolbox (macOS GPU)
ffmpeg -i input.mp4 \
  -c:v h264_videotoolbox -b:v 4M -allow_sw 1 \
  -c:a copy output.mp4

# VAAPI (Linux GPU)
ffmpeg -vaapi_device /dev/dri/renderD128 \
  -i input.mp4 \
  -vf "format=nv12,hwupload" \
  -c:v h264_vaapi -qp 23 \
  -c:a copy output.mp4

# NVDEC hardware decode + NVENC hardware encode (zero-copy GPU pipeline)
ffmpeg -hwaccel cuda -hwaccel_output_format cuda \
  -i input.mp4 \
  -c:v h264_nvenc -preset p4 -cq 23 \
  output.mp4
```

---

## Metadata

```bash
# Read all metadata
ffprobe -v quiet -print_format json -show_format input.mp4 | python3 -m json.tool

# Write metadata tags
ffmpeg -i input.mp4 \
  -metadata title="My Video" \
  -metadata artist="Studio Name" \
  -metadata year="2024" \
  -c copy output.mp4

# Copy metadata from one file to another
ffmpeg -i input.mp4 -i meta_source.mp4 \
  -map_metadata 1 -c copy output.mp4

# Remove all metadata
ffmpeg -i input.mp4 -map_metadata -1 -c copy output.mp4
```

---

## Batch Processing (Shell Examples)

```bash
# Convert all MKV files in a directory to MP4
for f in *.mkv; do
  ffmpeg -i "$f" -c:v libx264 -crf 23 -c:a aac "${f%.mkv}.mp4"
done

# Parallel batch processing (4 jobs at once)
ls *.mp4 | xargs -P 4 -I{} ffmpeg -i {} -c:v libx265 -crf 28 -c:a copy {}.hevc.mp4

# Create thumbnail for each video
for f in *.mp4; do
  ffmpeg -ss 5 -i "$f" -frames:v 1 -q:v 2 "${f%.mp4}_thumb.jpg"
done
```

---

## Debugging & Analysis

```bash
# Show detailed decoder information
ffmpeg -debug pict -i input.mp4 -c copy -f null -

# Analyze bitstream
ffprobe -show_frames -select_streams v:0 -print_format json input.mp4 | head -100

# Check for B-frames
ffprobe -show_frames -select_streams v:0 \
  -show_entries frame=pict_type -of compact input.mp4 | head -30

# Show packet/frame timestamps
ffprobe -show_packets -select_streams v:0 -print_format json input.mp4 | head -50

# Measure encoding time
time ffmpeg -i input.mp4 -c:v libx264 -crf 23 output.mp4

# Dry run (check filter graph without encoding)
ffmpeg -i input.mp4 -vf "scale=1280:-2,fps=30" -f null -
```

---

## References

- **FFmpeg Man Page** — https://ffmpeg.org/ffmpeg.html
- **ffprobe Man Page** — https://ffmpeg.org/ffprobe.html
- **FFmpeg All Options** — https://ffmpeg.org/ffmpeg-all.html
- **FFmpeg Codecs Documentation** — https://ffmpeg.org/ffmpeg-codecs.html
- **FFmpeg Formats Documentation** — https://ffmpeg.org/ffmpeg-formats.html
- **H.264 Encoding (Wiki)** — https://trac.ffmpeg.org/wiki/Encode/H.264
- **H.265 Encoding (Wiki)** — https://trac.ffmpeg.org/wiki/Encode/H.265
- **Seeking (Wiki)** — https://trac.ffmpeg.org/wiki/Seeking
- **Concatenation (Wiki)** — https://trac.ffmpeg.org/wiki/Concatenate
- **HW Acceleration Intro** — https://trac.ffmpeg.org/wiki/HWAccelIntro
- **Thumbnails (Wiki)** — https://trac.ffmpeg.org/wiki/Create%20a%20thumbnail%20image%20every%20X%20seconds
- **GIF creation (Wiki)** — https://trac.ffmpeg.org/wiki/Slideshow
