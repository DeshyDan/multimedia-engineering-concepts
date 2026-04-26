# FFmpeg Filters Reference

A comprehensive reference for FFmpeg audio and video filters.

---

## Overview

FFmpeg filters are applied using:
- `-vf` / `-af` for simple single-input filters
- `-filter_complex` for multi-input/multi-output filter graphs

### Filter Graph Syntax

```
-filter_complex "filter1=param1=val:param2=val,filter2=param[label];[label]filter3"
```

- `,` chains filters sequentially.
- `;` separates filter chains.
- `[label]` names a stream connection between chains.
- `[0:v]`, `[1:a]` reference input pads by index.

---

## Video Filters

### Scaling & Geometry

#### `scale`
Resize video frames.
```bash
# Scale to 1280x720
-vf "scale=1280:720"

# Scale width to 1280, auto-calculate height (divisible by 2)
-vf "scale=1280:-2"

# Scale to 50%
-vf "scale=iw/2:ih/2"

# Scale with bicubic algorithm
-vf "scale=1920:1080:flags=bicubic"

# Scale with Lanczos (best quality for downscaling)
-vf "scale=1280:720:flags=lanczos"

# Force aspect ratio: fit within 1920x1080
-vf "scale=1920:1080:force_original_aspect_ratio=decrease"
```

#### `crop`
Crop a region from the frame.
```bash
# Syntax: crop=w:h:x:y
-vf "crop=640:480:0:0"       # Top-left 640x480
-vf "crop=iw/2:ih"           # Left half
-vf "crop=iw:ih/2:0:ih/2"   # Bottom half
```

#### `pad`
Add padding (black bars) around the video.
```bash
# Pad to 1920x1080 (center)
-vf "pad=1920:1080:(ow-iw)/2:(oh-ih)/2"

# Add black letterbox to 16:9
-vf "pad=iw:iw*9/16:(ow-iw)/2:(oh-ih)/2:black"
```

#### `rotate`
Rotate video by arbitrary angle (in radians).
```bash
-vf "rotate=PI/2"      # 90° clockwise
-vf "rotate=PI"        # 180°
-vf "rotate=PI/2:c=black:ow=ih:oh=iw"  # 90° with proper output dimensions
```

#### `transpose`
Fast 90-degree rotation and flip.
```bash
-vf "transpose=0"  # Counterclockwise + vertical flip
-vf "transpose=1"  # Clockwise (90°)
-vf "transpose=2"  # Counterclockwise (270°)
-vf "transpose=3"  # Clockwise + vertical flip
```

#### `hflip` / `vflip`
Horizontal and vertical flip.
```bash
-vf "hflip"   # Mirror horizontally
-vf "vflip"   # Flip vertically
```

---

### Frame Rate

#### `fps`
Force a specific output frame rate.
```bash
-vf "fps=30"
-vf "fps=24000/1001"   # 23.976 (NTSC cinema)
```

#### `minterpolate`
Motion-compensated frame interpolation (increase frame rate).
```bash
-vf "minterpolate=fps=60:mi_mode=mci:mc_mode=aobmc:vsbmc=1"
```

#### `setpts`
Modify presentation timestamps (change speed without altering fps).
```bash
-vf "setpts=2.0*PTS"    # Slow down to 50% speed
-vf "setpts=0.5*PTS"    # Speed up to 200% speed
-vf "setpts=PTS/TB/AVTB"  # Reset timestamps
```

---

### Deinterlacing

#### `yadif`
Yet Another Deinterlacing Filter — most common deinterlacer.
```bash
-vf "yadif"              # Single rate output (same fps)
-vf "yadif=mode=1"       # Double rate (2x fps)
-vf "yadif=mode=0:parity=0"  # Top-field-first
```

#### `bwdif`
Bob Weaver Deinterlacing Filter — better motion rendering.
```bash
-vf "bwdif"
-vf "bwdif=mode=1"  # Double rate
```

---

### Overlay & Composition

#### `overlay`
Overlay one video or image on top of another.
```bash
# Syntax: overlay=x:y
# Overlay at top-left
-filter_complex "[0:v][1:v]overlay=0:0"

# Overlay at top-right
-filter_complex "[0:v][1:v]overlay=W-w-10:10"

# Overlay at center
-filter_complex "[0:v][1:v]overlay=(W-w)/2:(H-h)/2"

# Time-limited overlay (show from 5s to 15s)
-filter_complex "[0:v][1:v]overlay=10:10:enable='between(t,5,15)'"
```

#### `drawtext`
Render text onto video frames.
```bash
-vf "drawtext=text='Hello World':fontcolor=white:fontsize=48:x=10:y=10"

# With font file
-vf "drawtext=fontfile=/path/to/font.ttf:text='Hello':fontsize=48:fontcolor=yellow"

# Scrolling text ticker
-vf "drawtext=text='Breaking News':fontsize=36:fontcolor=white:\
x='w-mod(t*100\\,w+text_w)':y='h-line_h-10'"

# Timestamp overlay
-vf "drawtext=text='%{pts\\:hms}':fontsize=24:fontcolor=white:x=5:y=5"

# Frame number
-vf "drawtext=text='Frame\\: %{n}':fontsize=20:fontcolor=white:x=5:y=5"
```

#### `drawbox`
Draw a colored rectangle.
```bash
-vf "drawbox=x=10:y=10:w=200:h=100:color=red:t=3"
# t=fill for filled box
-vf "drawbox=x=10:y=10:w=200:h=100:color=red@0.5:t=fill"
```

---

### Color Processing

#### `eq`
Adjust brightness, contrast, saturation, and gamma.
```bash
-vf "eq=brightness=0.05:contrast=1.1:saturation=1.3:gamma=1.0"
```

#### `colorbalance`
Adjust color balance in shadows/midtones/highlights.
```bash
-vf "colorbalance=rs=0.1:gs=0.0:bs=-0.1:rm=0:gm=0:bm=0:rh=0:gh=0:bh=0"
# rs/gs/bs = shadows, rm/gm/bm = midtones, rh/gh/bh = highlights
```

#### `curves`
Apply color curve adjustments (similar to Photoshop curves).
```bash
# Lift shadows slightly
-vf "curves=r='0/0.1 1/1':g='0/0.1 1/1':b='0/0.1 1/1'"

# S-curve for contrast
-vf "curves=all='0/0 0.25/0.15 0.75/0.85 1/1'"
```

#### `hue`
Rotate hue and adjust saturation.
```bash
-vf "hue=h=30:s=1.2"   # 30° hue shift, 20% more saturation
```

#### `colorchannelmixer`
Mix color channels (useful for color grading effects).
```bash
# Swap red and blue channels (cold look)
-vf "colorchannelmixer=rr=0:rb=1:br=1:bb=0"
```

#### `lut3d`
Apply a 3D LUT (Look-Up Table) for color grading.
```bash
-vf "lut3d=film_look.cube"
```

#### `format`
Convert pixel format.
```bash
-vf "format=yuv420p"
-vf "format=yuv420p10le"   # 10-bit for HDR
```

---

### Subtitles

#### `subtitles`
Burn in SRT/ASS/SSA subtitles.
```bash
-vf "subtitles=subtitles.srt"
-vf "subtitles=subtitles.srt:force_style='Fontsize=24,PrimaryColour=&H00FFFFFF'"
```

#### `ass`
Burn in ASS/SSA formatted subtitles.
```bash
-vf "ass=subtitles.ass"
```

---

### Analysis & Quality

#### `psnr`
Compute PSNR between two video streams.
```bash
-filter_complex "[0:v][1:v]psnr"
```

#### `ssim`
Compute SSIM between two video streams.
```bash
-filter_complex "[0:v][1:v]ssim"
```

#### `thumbnail`
Select the most representative frame in a segment.
```bash
-vf "thumbnail=100"  # Analyze 100 frames, output best one
-frames:v 1          # Output only 1 frame
```

#### `blackdetect`
Detect silent/black segments.
```bash
-vf "blackdetect=d=0.05:pix_th=0.1" -f null -
```

#### `scdet`
Detect scene changes.
```bash
-vf "scdet=threshold=10" -f null -
```

---

### Stacking & Tiling

#### `hstack` / `vstack`
Stack videos horizontally or vertically.
```bash
-filter_complex "[0:v][1:v]hstack=inputs=2[v]"
-filter_complex "[0:v][1:v][2:v]hstack=inputs=3[v]"
-filter_complex "[0:v][1:v]vstack=inputs=2[v]"
```

#### `xstack`
Arrange videos in an arbitrary grid layout.
```bash
# 2x2 grid
-filter_complex "[0:v][1:v][2:v][3:v]xstack=inputs=4:layout=0_0|w0_0|0_h0|w0_h0[v]"
```

---

## Audio Filters

### Volume & Normalization

#### `volume`
Adjust audio volume.
```bash
-af "volume=2.0"      # Double volume
-af "volume=0.5"      # Half volume
-af "volume=-3dB"     # Reduce by 3 dB
-af "volume=6dB"      # Boost by 6 dB
```

#### `loudnorm`
EBU R128 loudness normalization.
```bash
-af "loudnorm=I=-14:LRA=11:TP=-1.5"

# Two-pass mode (use measured values from pass 1)
-af "loudnorm=I=-14:LRA=11:TP=-1.5:\
measured_I=-18.3:measured_LRA=7.2:measured_TP=-2.1:measured_thresh=-28.9:\
offset=0.3:linear=true:print_format=summary"
```

#### `dynaudnorm`
Dynamic audio normalizer (per-frame normalization).
```bash
-af "dynaudnorm=f=150:g=15"
```

---

### Equalization & Filtering

#### `highpass` / `lowpass`
Simple first-order high/low pass filters.
```bash
-af "highpass=f=80"      # Remove below 80 Hz
-af "lowpass=f=8000"     # Remove above 8 kHz
```

#### `equalizer`
Parametric EQ band.
```bash
# Boost 1 kHz by 3 dB with Q=1
-af "equalizer=f=1000:width_type=q:width=1:g=3"
```

#### `bass` / `treble`
Shelving EQ for bass and treble.
```bash
-af "bass=g=5:f=100"       # Bass boost at 100 Hz
-af "treble=g=4:f=8000"    # Treble boost at 8 kHz
```

---

### Dynamics Processing

#### `acompressor`
Dynamic range compressor.
```bash
-af "acompressor=threshold=-20dB:ratio=4:attack=5:release=50:makeup=3dB"
# threshold: level above which compression kicks in
# ratio: compression amount (4:1 means every 4dB above = 1dB output)
# attack/release: in milliseconds
# makeup: output gain
```

#### `alimiter`
Hard limiter (brick-wall ceiling).
```bash
-af "alimiter=limit=0.891:level=disabled"
# 0.891 linear ≈ -1 dBFS
```

#### `agate`
Noise gate (silence audio below threshold).
```bash
-af "agate=threshold=0.01:ratio=100:attack=2:release=200"
```

---

### Channel Operations

#### `pan`
Remix audio channels.
```bash
# Stereo to mono (mix)
-af "pan=mono|c0=0.5*c0+0.5*c1"

# Extract left channel only
-af "pan=stereo|c0=c0|c1=c0"

# Extract right channel only
-af "pan=stereo|c0=c1|c1=c1"

# 5.1 downmix to stereo
-af "pan=stereo|FL<FL+0.5*FC+0.6*BL+0.6*SL|FR<FR+0.5*FC+0.6*BR+0.6*SR"
```

#### `channelsplit`
Split multi-channel audio into individual streams.
```bash
-filter_complex "[0:a]channelsplit=channel_layout=stereo[left][right]"
```

#### `amerge`
Merge multiple mono streams into multi-channel.
```bash
-filter_complex "[0:a][1:a]amerge=inputs=2[stereo]"
```

#### `amix`
Mix multiple audio inputs into one.
```bash
-filter_complex "amix=inputs=2:duration=longest:dropout_transition=2"
```

---

### Time & Speed

#### `atempo`
Change audio playback speed (preserves pitch).
```bash
-af "atempo=1.5"        # 1.5x speed
-af "atempo=0.75"       # 0.75x speed (slower)
-af "atempo=2.0,atempo=2.0"  # 4x speed (chain for values > 2.0)
```

#### `asetrate`
Change sample rate without resampling (shifts pitch).
```bash
-af "asetrate=44100*1.5"   # Pitch up 1.5x (chipmunk)
```

#### `adelay`
Add delay to audio channels.
```bash
-af "adelay=500|500"       # 500ms delay on both channels
-af "adelay=0|200"         # Delay right channel by 200ms
```

---

### Effects

#### `aecho`
Echo/reverb effect.
```bash
-af "aecho=0.8:0.88:60:0.4"
# in_gain:out_gain:delays:decays
```

#### `chorus`
Chorus effect.
```bash
-af "chorus=0.5:0.9:50|60|40:0.4|0.32|0.3:0.25|0.4|0.3:2|2.3|1.3"
```

#### `afftdn`
Spectral noise reduction using FFT.
```bash
-af "afftdn=nf=-25"   # Noise floor at -25 dB
```

#### `silenceremove`
Remove silence from audio.
```bash
-af "silenceremove=stop_periods=-1:stop_duration=2:stop_threshold=-30dB"
```

---

### Visualization

#### `showwaves`
Display audio waveform as video.
```bash
-filter_complex "[0:a]showwaves=s=1280x200:mode=line:colors=white[v]"
```

#### `showspectrum`
Display audio frequency spectrum as video.
```bash
-filter_complex "[0:a]showspectrum=s=1280x480:mode=combined:color=rainbow:slide=scroll[v]"
```

#### `ebur128`
Display EBU R128 loudness meter.
```bash
-filter_complex "[0:a]ebur128=video=1:size=1280x480[v][a]"
```
