# FFmpeg Common Patterns

Real-world patterns and recipes for common video and audio processing tasks.

---

## Pattern 1: Web Video Delivery Pipeline

### Goal
Convert any input video into web-optimized MP4 (H.264 + AAC) and WebM (VP9 + Opus) with fast-start enabled.

```bash
# Step 1: Produce H.264/AAC MP4 for broad compatibility
ffmpeg -i input.mov \
  -c:v libx264 \
  -crf 23 \
  -preset slow \
  -profile:v high \
  -level 4.0 \
  -pix_fmt yuv420p \
  -movflags +faststart \
  -c:a aac \
  -b:a 192k \
  -ar 48000 \
  output_h264.mp4

# Step 2: Produce VP9/Opus WebM for modern browsers (two-pass for quality)
ffmpeg -i input.mov -c:v libvpx-vp9 -b:v 0 -crf 33 -pass 1 -an -f null /dev/null
ffmpeg -i input.mov \
  -c:v libvpx-vp9 \
  -b:v 0 -crf 33 \
  -pass 2 \
  -c:a libopus \
  -b:a 128k \
  output_vp9.webm
```

**Use in HTML:**
```html
<video controls>
  <source src="output_vp9.webm" type="video/webm">
  <source src="output_h264.mp4" type="video/mp4">
</video>
```

---

## Pattern 2: Multi-Bitrate HLS Ladder

### Goal
Generate a complete HLS adaptive bitrate package with multiple renditions from a single source.

```bash
ffmpeg -i input.mp4 \
  -filter_complex \
    "[0:v]split=4[v1][v2][v3][v4]; \
     [v1]scale=1920:1080[1080p]; \
     [v2]scale=1280:720[720p]; \
     [v3]scale=854:480[480p]; \
     [v4]scale=640:360[360p]" \
  \
  -map "[1080p]" -c:v:0 libx264 -b:v:0 4500k -maxrate:v:0 4500k -bufsize:v:0 9000k \
  -map "[720p]"  -c:v:1 libx264 -b:v:1 2500k -maxrate:v:1 2500k -bufsize:v:1 5000k \
  -map "[480p]"  -c:v:2 libx264 -b:v:2 1000k -maxrate:v:2 1000k -bufsize:v:2 2000k \
  -map "[360p]"  -c:v:3 libx264 -b:v:3  500k -maxrate:v:3  500k -bufsize:v:3 1000k \
  \
  -map 0:a -c:a:0 aac -b:a:0 192k \
  -map 0:a -c:a:1 aac -b:a:1 128k \
  -map 0:a -c:a:2 aac -b:a:2  96k \
  -map 0:a -c:a:3 aac -b:a:3  64k \
  \
  -x264opts "keyint=60:min-keyint=60:no-scenecut" \
  -g 60 -keyint_min 60 -sc_threshold 0 \
  \
  -f hls \
  -hls_time 6 \
  -hls_playlist_type vod \
  -hls_segment_type fmp4 \
  -hls_flags independent_segments \
  -master_pl_name master.m3u8 \
  -var_stream_map "v:0,a:0 v:1,a:1 v:2,a:2 v:3,a:3" \
  -hls_segment_filename "hls/stream_%v/seg_%03d.mp4" \
  hls/stream_%v/playlist.m3u8
```

---

## Pattern 3: Live Stream Ingest & Re-stream

### Goal
Receive a live RTMP stream, transcode it to multiple bitrates, and re-package as HLS.

```bash
ffmpeg \
  -listen 1 \
  -i rtmp://0.0.0.0:1935/live/stream_key \
  \
  -filter_complex \
    "[0:v]split=2[vhigh][vlow]; \
     [vhigh]scale=1280:720[v720p]; \
     [vlow]scale=854:480[v480p]" \
  \
  -map "[v720p]" -c:v:0 libx264 -b:v:0 2500k -preset fast -g 50 \
  -map "[v480p]" -c:v:1 libx264 -b:v:1 1000k -preset fast -g 50 \
  -map 0:a       -c:a:0 aac -b:a:0 128k \
  -map 0:a       -c:a:1 aac -b:a:1  96k \
  \
  -f hls \
  -hls_time 4 \
  -hls_flags delete_segments+append_list \
  -hls_list_size 5 \
  -master_pl_name /var/www/hls/master.m3u8 \
  -var_stream_map "v:0,a:0 v:1,a:1" \
  -hls_segment_filename "/var/www/hls/stream_%v/seg_%03d.ts" \
  /var/www/hls/stream_%v/playlist.m3u8
```

---

## Pattern 4: Video Thumbnails & Sprite Sheet

### Goal
Generate a VTT-based thumbnail sprite sheet for video player scrubbing preview.

```bash
# Step 1: Extract one thumbnail per 10 seconds
ffmpeg -i input.mp4 \
  -vf "fps=1/10,scale=160:90" \
  -q:v 3 \
  thumbnails/thumb_%04d.jpg

# Step 2: Assemble thumbnails into a sprite sheet using ImageMagick
montage thumbnails/thumb_*.jpg \
  -tile 10x \
  -geometry 160x90+0+0 \
  sprite.jpg

# Step 3: Generate corresponding VTT file (via script)
# VTT entry example:
# 00:00:00.000 --> 00:00:10.000
# sprite.jpg#xywh=0,0,160,90
```

---

## Pattern 5: Audio Podcast Mastering

### Goal
Process a raw voice recording for podcast delivery: noise gate → compression → EQ → loudness normalize.

```bash
ffmpeg -i raw_interview.wav \
  -af "\
    highpass=f=80, \
    agate=threshold=-35dB:ratio=10:attack=2:release=200, \
    acompressor=threshold=-18dB:ratio=3:attack=5:release=50:makeup=4dB, \
    equalizer=f=200:width_type=o:width=2:g=-2, \
    equalizer=f=2500:width_type=o:width=2:g=2, \
    equalizer=f=8000:width_type=o:width=2:g=1.5, \
    loudnorm=I=-16:LRA=11:TP=-1.5\
  " \
  -c:a libmp3lame -b:a 192k \
  podcast_ready.mp3
```

---

## Pattern 6: VMAF Quality Assessment

### Goal
Measure perceptual quality between an original and compressed version.

```bash
# Step 1: Align both files (ensure same resolution)
ffmpeg -i compressed.mp4 -vf "scale=1920:1080" compressed_1080p.mp4

# Step 2: Run VMAF comparison
ffmpeg \
  -i original_1080p.mp4 \
  -i compressed_1080p.mp4 \
  -filter_complex \
    "[0:v][1:v]libvmaf=model_path=/usr/share/vmaf/model/vmaf_v0.6.1.json:\
     log_path=vmaf_results.json:log_fmt=json:psnr=1:ssim=1" \
  -f null -

# Results in vmaf_results.json include:
# - VMAF score (0-100; >90 is excellent)
# - PSNR per frame
# - SSIM per frame
```

---

## Pattern 7: HDR to SDR Conversion (Tone Mapping)

### Goal
Convert HDR10 content (BT.2020, PQ transfer) to SDR (BT.709) for distribution.

```bash
ffmpeg -i hdr_source.mp4 \
  -vf "\
    zscale=t=linear:npl=100, \
    format=gbrpf32le, \
    zscale=p=bt709, \
    tonemap=tonemap=hable:desat=0, \
    zscale=t=bt709:m=bt709:r=tv, \
    format=yuv420p\
  " \
  -c:v libx264 -crf 18 -preset slow \
  -c:a copy \
  sdr_output.mp4
```

---

## Pattern 8: Video Concat Without Re-Encoding

### Goal
Join multiple video clips with identical codec/resolution/frame rate without re-encoding.

```bash
# Step 1: Create concat list
cat > filelist.txt << 'EOF'
file 'clip_001.mp4'
file 'clip_002.mp4'
file 'clip_003.mp4'
EOF

# Step 2: Concatenate (stream copy, very fast)
ffmpeg -f concat -safe 0 -i filelist.txt -c copy output.mp4
```

---

## Pattern 9: Screen Recording

### Goal
Capture screen + microphone on different platforms.

```bash
# Linux (X11 + ALSA)
ffmpeg \
  -f x11grab -r 30 -s 1920x1080 -i :0.0 \
  -f alsa -i default \
  -c:v libx264 -preset ultrafast -crf 28 \
  -c:a aac -b:a 128k \
  screen_recording.mp4

# macOS (AVFoundation)
ffmpeg \
  -f avfoundation -r 30 -i "1:0" \
  -c:v libx264 -preset ultrafast -crf 28 \
  -c:a aac -b:a 128k \
  screen_recording.mp4

# Windows (GDI grab + DirectShow audio)
ffmpeg \
  -f gdigrab -r 30 -i desktop \
  -f dshow -i audio="Microphone (Realtek)" \
  -c:v libx264 -preset ultrafast -crf 28 \
  -c:a aac -b:a 128k \
  screen_recording.mp4
```

---

## Pattern 10: Video Watermarking

### Goal
Add a persistent semi-transparent watermark to a video.

```bash
# Add PNG logo as watermark (top-right corner, 70% opacity)
ffmpeg -i input.mp4 -i watermark.png \
  -filter_complex \
    "[1:v]format=rgba,colorchannelmixer=aa=0.7[wm]; \
     [0:v][wm]overlay=W-w-20:20" \
  -c:v libx264 -crf 23 \
  -c:a copy \
  watermarked.mp4

# Add text watermark
ffmpeg -i input.mp4 \
  -vf "drawtext=\
    fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf:\
    text='© 2024 Company Name':\
    fontcolor=white@0.5:\
    fontsize=28:\
    x=W-tw-20:y=H-th-20" \
  -c:v libx264 -crf 23 -c:a copy \
  watermarked.mp4
```

---

## Pattern 11: Audio Extraction & Sync Fix

### Goal
Extract audio from video, fix sync issues, and merge back.

```bash
# Extract audio
ffmpeg -i video.mp4 -vn -c:a pcm_s16le extracted_audio.wav

# Fix audio/video sync (shift audio forward by 250ms)
ffmpeg -i video.mp4 \
  -itsoffset 0.250 -i video.mp4 \
  -map 0:v -map 1:a \
  -c:v copy -c:a aac \
  synced_output.mp4

# Alternatively using audio delay filter
ffmpeg -i video.mp4 \
  -af "adelay=250|250" \
  -c:v copy -c:a aac \
  synced_output.mp4
```

---

## Pattern 12: Batch Transcoding with Progress

### Goal
Transcode a folder of videos with progress tracking.

```bash
#!/bin/bash
# batch_transcode.sh

INPUT_DIR="./input"
OUTPUT_DIR="./output"
mkdir -p "$OUTPUT_DIR"

for input_file in "$INPUT_DIR"/*.{mp4,mkv,mov,avi}; do
  [ -f "$input_file" ] || continue

  filename=$(basename "$input_file")
  name="${filename%.*}"
  output_file="$OUTPUT_DIR/${name}_h264.mp4"

  echo "Processing: $filename"

  ffmpeg -i "$input_file" \
    -c:v libx264 -crf 23 -preset fast \
    -c:a aac -b:a 192k \
    -movflags +faststart \
    -progress pipe:1 \
    -loglevel error \
    "$output_file"

  echo "Done: $output_file"
done

echo "Batch complete."
```

---

## Pattern 13: Subtitle Handling

### Goal
Add, extract, and burn in subtitles.

```bash
# Add external SRT subtitles as a soft subtitle stream
ffmpeg -i input.mp4 -i subtitles.srt \
  -c:v copy -c:a copy \
  -c:s mov_text \
  -metadata:s:s:0 language=eng \
  output_with_subs.mp4

# Extract subtitles from MKV
ffmpeg -i input.mkv -map 0:s:0 -c:s srt output_subtitles.srt

# Burn in (hard-code) SRT subtitles
ffmpeg -i input.mp4 \
  -vf "subtitles=subtitles.srt:force_style='FontName=DejaVu Sans,Fontsize=22,PrimaryColour=&HFFFFFF&'" \
  -c:v libx264 -crf 23 \
  -c:a copy \
  output_hardcoded_subs.mp4

# Convert SRT to WebVTT
ffmpeg -i subtitles.srt subtitles.vtt
```

---

## Pattern 14: Encoding for Different Targets

### Netflix / Streaming Service

```bash
ffmpeg -i source.mp4 \
  -c:v libx264 \
  -crf 18 \
  -preset slow \
  -profile:v high -level 4.1 \
  -pix_fmt yuv420p \
  -x264opts "nal-hrd=cbr:force-cfr=1" \
  -b:v 8M -minrate 8M -maxrate 8M -bufsize 16M \
  -c:a libfdk_aac -b:a 256k -ar 48000 -ac 2 \
  output_netflix.mp4
```

### Broadcast (EBU-compliant)

```bash
ffmpeg -i source.mp4 \
  -c:v libx264 -b:v 50M -profile:v high422 -level 4.2 \
  -pix_fmt yuv422p \
  -c:a pcm_s24le -ar 48000 \
  -af "loudnorm=I=-23:LRA=20:TP=-1:linear=true" \
  broadcast_output.mxf
```

### Mobile Optimized

```bash
ffmpeg -i source.mp4 \
  -c:v libx264 -crf 28 -preset medium \
  -profile:v baseline -level 3.1 \
  -vf "scale=854:480:flags=bicubic" \
  -pix_fmt yuv420p \
  -movflags +faststart \
  -c:a aac -b:a 96k -ar 44100 \
  output_mobile.mp4
```

### Low-Latency Live (OBS-compatible ingest)

```bash
ffmpeg -re -i input.mp4 \
  -c:v libx264 \
  -preset ultrafast \
  -tune zerolatency \
  -b:v 3000k \
  -g 60 -keyint_min 60 \
  -x264opts "nal-hrd=cbr:force-cfr=1" \
  -maxrate 3000k -bufsize 6000k \
  -c:a aac -b:a 160k -ar 48000 \
  -f flv \
  rtmp://ingest.server.com/live/stream_key
```
