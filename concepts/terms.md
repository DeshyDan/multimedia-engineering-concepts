# Multimedia Engineering — Core Terms & Glossary

A reference glossary of fundamental terms used in video and audio processing engineering.

---

## A

**AAC (Advanced Audio Coding)**
A lossy audio codec designed to be the successor to MP3. Achieves better quality at similar bit rates. Widely used in streaming and mobile.

**ABR (Average Bit Rate)**
An encoding mode that targets an average bit rate over the entire stream while allowing local variation. A compromise between CBR and VBR.

**Aspect Ratio**
The proportional relationship between a video frame's width and height (e.g., 16:9, 4:3, 21:9). Can be stored as SAR (Sample Aspect Ratio) or DAR (Display Aspect Ratio).

**Audio Channel**
A single audio signal track. Mono = 1, Stereo = 2, 5.1 Surround = 6, 7.1 Surround = 8.

**Audio Sample**
A single measurement of audio amplitude at a point in time. The resolution of a sample is determined by bit depth (e.g., 16-bit, 24-bit, 32-bit float).

---

## B

**B-Frame (Bi-directional Predicted Frame)**
A compressed video frame that can reference both past and future frames. Achieves high compression but increases latency. Part of MPEG group-of-pictures (GOP) structures.

**Bitrate**
The number of bits processed per unit of time, typically expressed in kilobits per second (kbps) or megabits per second (Mbps). Higher bitrate generally means better quality but larger file size.

**Bit Depth**
- *Audio*: The number of bits per sample. Common values: 8, 16, 24, 32-bit. Higher depth means greater dynamic range.
- *Video*: Bits per color channel (e.g., 8-bit = 256 shades per channel; 10-bit = 1024 shades). HDR requires 10-bit or higher.

**Buffer (Decoder Buffer / VBV Buffer)**
A memory buffer that holds encoded data before it is decoded. VBV (Video Buffering Verifier) constraints are used to ensure that a decoder with limited buffer size can decode a stream without underflow or overflow.

---

## C

**CBR (Constant Bit Rate)**
Encoding mode where the bit rate remains constant throughout. Required for broadcast and some streaming protocols. Achieved with VBV constraints.

**Chroma Subsampling**
The practice of encoding color (chroma) information at a lower resolution than luma (brightness) information, exploiting the fact that the human eye is less sensitive to color detail.
- `4:4:4` – No subsampling; full color resolution.
- `4:2:2` – Chroma sampled at half horizontal resolution.
- `4:2:0` – Chroma sampled at half horizontal and half vertical resolution. Most common for consumer video.

**Codec**
Short for **co**der/**dec**oder. Software/hardware that encodes and decodes digital media. Examples: H.264, H.265, VP9, AV1, AAC, Opus.

**Color Space**
A mathematical model for representing colors. Common color spaces:
- `BT.601` – Standard definition video.
- `BT.709` – High definition video (most common).
- `BT.2020` – Ultra-HD / HDR video.
- `sRGB` – Standard for computer displays and web.

**Color Range**
- *Limited/TV range*: Y: 16–235, Cb/Cr: 16–240 (for 8-bit video).
- *Full/PC range*: 0–255. Mismatches cause washed-out or crushed visuals.

**Container (Format)**
A file format that wraps encoded media streams with metadata. The container is independent of the codec(s) inside it. Examples: MP4, MKV, WebM, MOV, AVI, TS.

**CRF (Constant Rate Factor)**
A quality-based encoding mode in x264/x265/libaom-av1. Lower values = better quality. x264 range: 0–51 (18–28 typical); x265 range: 0–51 (24–32 typical).

---

## D

**DAR (Display Aspect Ratio)**
The ratio at which the video is intended to be displayed, accounting for non-square pixels (SAR). `DAR = width * SAR_num / (height * SAR_den)`.

**Demuxing**
The process of separating interleaved streams (video, audio, subtitles) from a container file into individual elementary streams.

**DTS (Decode Timestamp)**
The time at which a packet should be decoded. Different from PTS when B-frames are present (decoding order differs from presentation order).

**Dynamic Range**
The ratio between the loudest and quietest signals a system can handle. In audio, measured in dB. In video, the number of distinct brightness levels from black to white.

---

## E

**Elementary Stream**
A single encoded media stream (e.g., one video stream or one audio stream) before it is wrapped in a container or multiplexed.

**Encoding**
The process of converting raw media data (uncompressed frames/samples) into a compressed format using a codec.

---

## F

**FPS (Frames Per Second)**
The number of video frames displayed per second. Common values: 24 (cinema), 25 (PAL), 29.97 (NTSC), 30, 50, 60, 120.

**Frame**
A single still image in a video sequence. Video is composed of a sequence of frames played at a defined rate (FPS).

**Frame Types**
- **I-Frame (Intra-coded)**: Self-contained; can be decoded without reference to other frames. Used as seek/random-access points.
- **P-Frame (Predicted)**: References one previous frame. Smaller than I-frames.
- **B-Frame (Bi-directional)**: References past and future frames. Smallest but increases latency.

---

## G

**GOP (Group of Pictures)**
A sequence of frames between two I-frames. Defines compression efficiency and seek granularity. Shorter GOPs improve seekability but reduce compression.

---

## H

**HDR (High Dynamic Range)**
Video that captures a broader range of luminance and color than standard dynamic range (SDR). Standards: HDR10, HDR10+, Dolby Vision, HLG. Requires 10-bit or higher encoding.

**HLS (HTTP Live Streaming)**
An Apple-developed adaptive streaming protocol. Segments media into small `.ts` or `.fmp4` chunks described by `.m3u8` playlists.

---

## I

**I-Frame Interval**
The frequency at which I-frames appear in the stream. Also called keyframe interval. Set via `-g` in FFmpeg.

**Interlaced Video**
Video where each frame is split into two fields (odd/even lines), transmitted alternately. Common in broadcast (1080i, 480i). Requires deinterlacing for progressive display.

---

## K

**Keyframe**
See I-Frame. In streaming contexts, keyframes are critical for adaptive bitrate switching and seeking.

---

## L

**Latency**
The delay between capture/input and playback/output. Affected by buffer sizes, encoder lookahead, GOP length, and network conditions.

**Level (Codec)**
A set of constraints that define maximum bit rate, resolution, and frame rate combinations for a given profile. E.g., H.264 Level 4.1 supports up to 1080p60.

**LUFS (Loudness Units Full Scale)**
A standardized unit for measuring integrated loudness of audio. Used in broadcast loudness normalization (EBU R128, ATSC A/85, etc.). Typical targets: -14 LUFS (streaming), -23 LUFS (broadcast).

---

## M

**Muxing (Multiplexing)**
Combining multiple elementary streams (video, audio, subtitles) into a single container file or transport stream.

**Metadata**
Non-media data stored in a container: title, artist, duration, creation date, language tags, rotation, etc.

---

## P

**Packet**
The fundamental unit of encoded data exchanged between demuxer, decoder, encoder, and muxer in a media pipeline. Contains one or more encoded frames.

**PCM (Pulse Code Modulation)**
Uncompressed digital audio. Raw audio samples stored directly. Used in WAV, AIFF, and as an intermediate format during processing.

**Pixel Format (pix_fmt)**
Defines how pixel data is stored: color model, subsampling, and bit depth. Common formats: `yuv420p`, `yuv444p`, `yuv420p10le`, `rgb24`, `rgba`.

**Profile (Codec)**
A subset of the codec's features that trades flexibility for compatibility. E.g., H.264 Baseline/Main/High; H.265 Main/Main10.

**Progressive Video**
Video where each frame is a complete image (as opposed to interlaced). Standard for internet and modern displays.

**PTS (Presentation Timestamp)**
The time at which a decoded frame or sample should be presented (played back) to the viewer/listener.

---

## R

**Raw Audio**
Unencoded audio samples as PCM data, represented as bytes. Typical raw format: 48 kHz, 16/24/32-bit, stereo.

**Raw Video**
Unencoded video frames (YUV or RGB data). Extremely large but used as intermediate format during processing.

**Resolution**
The number of pixels in a video frame (width × height). Common resolutions: 640×480 (SD), 1280×720 (720p HD), 1920×1080 (1080p FHD), 3840×2160 (4K UHD).

**Re-encode / Transcode**
Decoding media from one format and re-encoding it into another format or set of parameters.

---

## S

**Sample Rate**
The number of audio samples captured per second (Hz). Common values: 8000 Hz (telephony), 44100 Hz (CD), 48000 Hz (professional video/film).

**SAR (Sample Aspect Ratio)**
The ratio of a pixel's width to its height. Square pixels have SAR 1:1. Anamorphic video may have non-square pixels.

**Seek**
Jumping to a specific position in a media file. Accurate seeking requires an I-frame at or before the seek point.

**Stream**
A continuous sequence of encoded media data (one video stream, one audio stream, one subtitle stream, etc.) within or outside a container.

**Subtitle / Closed Caption**
Text overlay tracks synchronized with video. Formats: SRT, ASS/SSA, WebVTT, DVB, EIA-608/708.

---

## T

**Timecode**
A sequence of numeric codes representing hours, minutes, seconds, and frames (HH:MM:SS:FF). Used to identify positions in video precisely.

**Timebase**
The unit of time used by timestamps (PTS/DTS) in a stream. Expressed as a rational number (e.g., 1/90000 for MPEG-TS, 1/12800 for some MP4 streams).

**Transcoding**
See Re-encode. Specifically implies a full decode → process → encode pipeline, as opposed to remuxing (copying streams without re-encoding).

**Transport Stream (MPEG-TS)**
An MPEG-2 container designed for broadcast and streaming over unreliable channels. Used by HLS. Supports error recovery.

---

## V

**VBR (Variable Bit Rate)**
Encoding mode where the bit rate varies based on scene complexity. Produces better quality at a given average file size compared to CBR.

**VBV (Video Buffering Verifier)**
A set of parameters (`vbv_maxrate`, `vbv_bufsize`) that constrain encoder output to ensure decoders with a fixed buffer size can decode the stream smoothly.

**Video Frame**
See Frame.

---

## Y

**YUV / YCbCr**
A color model that separates luminance (Y) from chrominance (Cb = blue-difference, Cr = red-difference). Used by most video codecs and formats. More efficient for compression than RGB because the human eye is more sensitive to brightness than color.
