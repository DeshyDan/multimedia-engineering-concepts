# Audio Processing Concepts

A reference guide for developers working with audio processing pipelines using FFmpeg and related tools.

---

## Core Audio Properties

### Sample Rate

The number of audio samples captured per second (measured in Hz).

| Sample Rate | Common Use |
|---|---|
| 8,000 Hz | Telephony, voice codecs (G.711) |
| 11,025 Hz | Legacy multimedia |
| 22,050 Hz | Legacy multimedia |
| 32,000 Hz | Broadcast radio |
| 44,100 Hz | CD audio, consumer music |
| 48,000 Hz | Professional audio, video production, streaming |
| 88,200 Hz | High-resolution audio mastering |
| 96,000 Hz | Professional high-resolution audio |
| 192,000 Hz | Audiophile/mastering |

```bash
# Resample to 48000 Hz
ffmpeg -i input.mp3 -ar 48000 output.wav

# Resample using high-quality resampler
ffmpeg -i input.wav -af "aresample=48000:resampler=soxr" output.wav
```

---

### Bit Depth

The number of bits used to represent each audio sample. More bits = greater dynamic range.

| Bit Depth | Dynamic Range | Common Use |
|---|---|---|
| 8-bit | ~48 dB | Legacy, telephony |
| 16-bit | ~96 dB | CD audio, consumer |
| 24-bit | ~144 dB | Professional recording/mastering |
| 32-bit float | ~1528 dB effective | DAW processing (never clips) |

```bash
# Convert to 24-bit PCM
ffmpeg -i input.wav -c:a pcm_s24le output_24bit.wav

# Convert to 32-bit float
ffmpeg -i input.wav -c:a pcm_f32le output_32f.wav
```

---

### Channels

| Channel Config | Count | Layout |
|---|---|---|
| Mono | 1 | `mono` / `FC` |
| Stereo | 2 | `stereo` / `FL+FR` |
| 2.1 | 3 | `FL+FR+LFE` |
| 4.0 Quad | 4 | `FL+FR+BL+BR` |
| 5.0 Surround | 5 | `FL+FR+FC+BL+BR` |
| 5.1 Surround | 6 | `FL+FR+FC+LFE+BL+BR` |
| 7.1 Surround | 8 | `FL+FR+FC+LFE+BL+BR+SL+SR` |

```bash
# Convert stereo to mono (mix down)
ffmpeg -i input.mp4 -ac 1 output.mp4

# Mix stereo to mono using pan filter
ffmpeg -i input.wav -af "pan=mono|c0=0.5*c0+0.5*c1" output.wav

# Convert mono to stereo (duplicate channel)
ffmpeg -i mono.wav -af "pan=stereo|c0=c0|c1=c0" stereo.wav
```

---

## Audio Filtering

### Volume Adjustment

```bash
# Increase volume by 6 dB
ffmpeg -i input.wav -af "volume=6dB" output.wav

# Set volume to 50%
ffmpeg -i input.wav -af "volume=0.5" output.wav

# Dynamic volume using expressions
ffmpeg -i input.wav -af "volume=enable='between(t,10,20)':volume=2" output.wav
```

---

### Loudness Normalization (EBU R128 / LUFS)

LUFS (Loudness Units Full Scale) is the standard for broadcast loudness. Key targets:
- `-14 LUFS` — Spotify, YouTube
- `-16 LUFS` — Apple Music, TIDAL
- `-23 LUFS` — EBU R128 broadcast standard
- `-24 LUFS` — ATSC A/85 (US broadcast)

```bash
# Measure loudness first (two-pass)
ffmpeg -i input.wav -af "loudnorm=print_format=json" -f null -

# Normalize to -14 LUFS (single pass, less accurate)
ffmpeg -i input.wav -af "loudnorm=I=-14:LRA=11:TP=-1.5" output.wav

# Two-pass loudness normalization (recommended)
# Pass 1 — measure
ffmpeg -i input.wav \
  -af "loudnorm=I=-14:LRA=11:TP=-1.5:print_format=json" \
  -f null - 2>&1 | tail -12

# Pass 2 — apply with measured values
ffmpeg -i input.wav \
  -af "loudnorm=I=-14:LRA=11:TP=-1.5:\
  measured_I=-18.5:measured_LRA=7.2:measured_TP=-2.0:\
  measured_thresh=-29.3:offset=0.5:linear=true" \
  output.wav
```

---

### Equalization (EQ)

```bash
# High-pass filter (remove frequencies below 80 Hz)
ffmpeg -i input.wav -af "highpass=f=80" output.wav

# Low-pass filter (remove frequencies above 8000 Hz)
ffmpeg -i input.wav -af "lowpass=f=8000" output.wav

# Band-pass filter (keep only 300 Hz - 3400 Hz — telephone effect)
ffmpeg -i input.wav -af "highpass=f=300,lowpass=f=3400" output.wav

# Equalizer (boost/cut specific frequency bands)
ffmpeg -i input.wav \
  -af "equalizer=f=100:width_type=o:width=2:g=3,\
       equalizer=f=1000:width_type=o:width=2:g=-2,\
       equalizer=f=10000:width_type=o:width=2:g=4" \
  output.wav

# Treble boost
ffmpeg -i input.wav -af "treble=g=5" output.wav

# Bass boost
ffmpeg -i input.wav -af "bass=g=5:f=110" output.wav
```

---

### Noise Reduction

```bash
# Noise gate (silence audio below a threshold)
ffmpeg -i input.wav -af "agate=threshold=0.01:ratio=10" output.wav

# Silence removal (trim silences longer than 2 seconds below -30 dB)
ffmpeg -i input.wav \
  -af "silenceremove=stop_periods=-1:stop_duration=2:stop_threshold=-30dB" \
  output.wav

# Dynamic noise reduction using afftdn (spectral subtraction)
ffmpeg -i noisy.wav -af "afftdn=nf=-25" output.wav
```

---

### Compression (Dynamic Range Compression)

```bash
# Simple audio compressor
ffmpeg -i input.wav \
  -af "acompressor=threshold=-20dB:ratio=4:attack=5:release=50:makeup=3dB" \
  output.wav

# Limiter (hard ceiling at -1 dBFS)
ffmpeg -i input.wav -af "alimiter=limit=0.891:level=disabled" output.wav
```

---

### Reverb & Effects

```bash
# Reverb using aecho (simple echo/reverb)
ffmpeg -i input.wav -af "aecho=0.8:0.88:60:0.4" output.wav

# Room reverb simulation
ffmpeg -i input.wav \
  -af "aecho=0.8:0.9:50|70|100:0.4|0.3|0.25" \
  output.wav

# Chorus effect
ffmpeg -i input.wav \
  -af "chorus=0.5:0.9:50|60|40:0.4|0.32|0.3:0.25|0.4|0.3:2|2.3|1.3" \
  output.wav

# Flanger effect
ffmpeg -i input.wav -af "flanger" output.wav
```

---

### Pitch Shifting & Time Stretching

```bash
# Change tempo without changing pitch (time-stretch to 0.75x speed)
ffmpeg -i input.wav -af "atempo=0.75" output.wav

# For values outside 0.5-2.0 range, chain multiple atempo filters
ffmpeg -i input.wav -af "atempo=0.5,atempo=0.8" output.wav  # 0.4x speed

# Pitch shift (change pitch without changing tempo)
# Using rubberband (if available)
ffmpeg -i input.wav -af "rubberband=pitch=1.5" output.wav

# Simple pitch shift using asetrate + atempo
ffmpeg -i input.wav -af "asetrate=44100*1.25,atempo=1/1.25" output.wav
```

---

### Audio Mixing & Merging

```bash
# Merge two audio files side by side (stereo from two mono sources)
ffmpeg -i left.wav -i right.wav \
  -filter_complex "[0:a][1:a]amerge=inputs=2,pan=stereo|c0<c0|c1<c1[a]" \
  -map "[a]" output.wav

# Mix (sum) two audio tracks
ffmpeg -i music.wav -i voice.wav \
  -filter_complex "amix=inputs=2:duration=longest:dropout_transition=2" \
  output.wav

# Mix with custom volumes
ffmpeg -i music.wav -i voice.wav \
  -filter_complex "[0:a]volume=0.3[music];[1:a]volume=1.0[voice];[music][voice]amix=inputs=2" \
  output.wav
```

---

### Audio Delay & Synchronization

```bash
# Add 500ms delay to audio
ffmpeg -i input.mp4 -af "adelay=500|500" output.mp4

# Fix audio/video sync (shift audio by 250ms)
ffmpeg -i input.mp4 -itsoffset 0.250 -i input.mp4 \
  -map 1:v -map 0:a -c copy output.mp4
```

---

## Audio Analysis

### Waveform Visualization

```bash
# Generate audio waveform image
ffmpeg -i input.wav \
  -filter_complex "showwavespic=s=1280x200:colors=0x00aaff" \
  waveform.png

# Animated waveform video
ffmpeg -i input.wav \
  -filter_complex "[0:a]showwaves=s=1280x200:mode=line:rate=25:colors=0x00aaff[v]" \
  -map "[v]" -c:v libx264 -r 25 waveform.mp4
```

---

### Spectrum / Spectrogram

```bash
# Generate spectrogram image
ffmpeg -i input.wav \
  -lavfi "showspectrumpic=s=1024x512:mode=combined:color=rainbow" \
  spectrogram.png

# Animated spectrogram video
ffmpeg -i input.wav \
  -filter_complex "[0:a]showspectrum=s=1280x480:mode=combined:color=rainbow:slide=scroll[v]" \
  -map "[v]" -c:v libx264 -r 25 spectrum.mp4
```

---

### Loudness Measurement

```bash
# Measure EBU R128 loudness statistics
ffmpeg -i input.wav -af "ebur128=peak=true" -f null - 2>&1

# Get audio duration and level stats
ffmpeg -i input.wav -af "astats" -f null - 2>&1
```

---

## Audio Extraction & Conversion

```bash
# Extract audio from video (copy stream, no re-encode)
ffmpeg -i input.mp4 -vn -c:a copy output.aac

# Extract and convert audio
ffmpeg -i input.mp4 -vn -c:a libmp3lame -b:a 192k output.mp3

# Extract audio as WAV
ffmpeg -i input.mp4 -vn -c:a pcm_s16le output.wav

# Split audio into segments (10-second chunks)
ffmpeg -i input.wav -f segment -segment_time 10 -c copy segment_%03d.wav
```

---

## Broadcast Audio Standards

### EBU R128 (European Broadcasting Union)
- **Integrated loudness**: -23 LUFS ± 1 LU
- **True peak**: ≤ -1 dBFS
- **Loudness Range (LRA)**: ≤ 20 LU

### ATSC A/85 (US broadcast)
- **Integrated loudness**: -24 LKFS ± 2 LU
- **True peak**: ≤ -2 dBTP

### Streaming Platforms
| Platform | Target LUFS | True Peak |
|---|---|---|
| Spotify | -14 LUFS | -1 dBTP |
| Apple Music | -16 LUFS | -1 dBTP |
| YouTube | -14 LUFS | -1 dBTP |
| Netflix | -27 LUFS | -2 dBTP |
| Tidal | -14 LUFS | -1 dBTP |
| Amazon Music | -14 LUFS | -2 dBTP |

---

## Common Audio Processing Patterns

### Podcast / Voice Preparation

```bash
# Podcast mastering chain:
# 1. High-pass at 80Hz (remove rumble)
# 2. Noise gate
# 3. Dynamic compression
# 4. EQ boost presence
# 5. Loudness normalize to -16 LUFS
# 6. True-peak limit at -1.5 dBFS
ffmpeg -i raw_voice.wav \
  -af "highpass=f=80,\
       agate=threshold=-30dB:ratio=10:attack=2:release=200,\
       acompressor=threshold=-18dB:ratio=3:attack=5:release=50:makeup=4dB,\
       equalizer=f=2500:width_type=o:width=2:g=2,\
       loudnorm=I=-16:LRA=11:TP=-1.5" \
  podcast_master.wav
```

### Music Streaming Preparation

```bash
# Master for streaming platforms (-14 LUFS, true peak -1 dBTP)
ffmpeg -i mix.wav \
  -af "loudnorm=I=-14:LRA=9:TP=-1.0:linear=true" \
  -c:a pcm_s24le master_for_streaming.wav
```
