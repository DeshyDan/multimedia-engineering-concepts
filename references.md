# References & Further Reading

This document lists the primary sources for all information in this collection, along with links to official documentation, source code, and standards.

---

## FFmpeg Official Resources

### Documentation & Website

| Resource | URL |
|---|---|
| FFmpeg Homepage | https://ffmpeg.org |
| FFmpeg Documentation | https://ffmpeg.org/documentation.html |
| FFmpeg Man Pages | https://ffmpeg.org/ffmpeg.html |
| ffprobe Man Page | https://ffmpeg.org/ffprobe.html |
| ffplay Man Page | https://ffmpeg.org/ffplay.html |
| FFmpeg Filters Documentation | https://ffmpeg.org/ffmpeg-filters.html |
| FFmpeg Codecs Documentation | https://ffmpeg.org/ffmpeg-codecs.html |
| FFmpeg Formats Documentation | https://ffmpeg.org/ffmpeg-formats.html |
| FFmpeg Protocols Documentation | https://ffmpeg.org/ffmpeg-protocols.html |

### C API (Doxygen)

The FFmpeg C API is documented via Doxygen. The `trunk` URL always reflects the latest development version; replace `trunk` with a version number (e.g., `6.1`) for a specific release.

| Library | Doxygen API Reference |
|---|---|
| **libavformat** (containers, muxing/demuxing) | https://ffmpeg.org/doxygen/trunk/group__libavf.html |
| **libavcodec** (encoding/decoding) | https://ffmpeg.org/doxygen/trunk/group__lavc.html |
| **libavfilter** (filtering) | https://ffmpeg.org/doxygen/trunk/group__lavfi.html |
| **libavutil** (utilities) | https://ffmpeg.org/doxygen/trunk/group__lavu.html |
| **libswscale** (image scaling/conversion) | https://ffmpeg.org/doxygen/trunk/group__libsws.html |
| **libswresample** (audio resampling) | https://ffmpeg.org/doxygen/trunk/group__lavu__sampmanip.html |
| **libavdevice** (hardware devices) | https://ffmpeg.org/doxygen/trunk/group__lavd.html |
| AVFormatContext | https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html |
| AVStream | https://ffmpeg.org/doxygen/trunk/structAVStream.html |
| AVCodecContext | https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html |
| AVCodecParameters | https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html |
| AVFrame | https://ffmpeg.org/doxygen/trunk/structAVFrame.html |
| AVPacket | https://ffmpeg.org/doxygen/trunk/structAVPacket.html |
| AVFilterGraph | https://ffmpeg.org/doxygen/trunk/structAVFilterGraph.html |

### Key Decoding API Functions

| Function | Purpose | Link |
|---|---|---|
| `avformat_open_input()` | Open a media container | https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49 |
| `avformat_find_stream_info()` | Probe stream information | https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb |
| `av_find_best_stream()` | Select the best stream | https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaa6a9aacc54b9cc13ca0e4bb48e58d55e |
| `av_read_frame()` | Read the next packet | https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61 |
| `avcodec_send_packet()` | Feed packet to decoder | https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3 |
| `avcodec_receive_frame()` | Get decoded frame | https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c |

### Key Encoding API Functions

| Function | Purpose | Link |
|---|---|---|
| `avcodec_find_encoder_by_name()` | Look up an encoder | https://ffmpeg.org/doxygen/trunk/group__lavc__core.html |
| `avcodec_open2()` | Open a codec | https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d |
| `avcodec_send_frame()` | Feed raw frame to encoder | https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#ga951f35d3da73aa01de3a2c42d80b5e82 |
| `avcodec_receive_packet()` | Get encoded packet | https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#ga5b9c7b9f5b3e5f0b2dc49a69c5a5c1ea |
| `avformat_write_header()` | Write container header | https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html |
| `av_interleaved_write_frame()` | Write a packet to output | https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493f160e1086abb3d0dec |
| `av_write_trailer()` | Finalise container | https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13 |

### Key Filtering API Functions

| Function | Purpose | Link |
|---|---|---|
| `avfilter_graph_alloc()` | Create a filter graph | https://ffmpeg.org/doxygen/trunk/group__lavfi.html |
| `avfilter_graph_parse_ptr()` | Parse filter description | https://ffmpeg.org/doxygen/trunk/group__lavfi.html#ga76f6b80b5bb595e3a95f5659e4fe9ffc |
| `avfilter_graph_config()` | Validate and configure graph | https://ffmpeg.org/doxygen/trunk/group__lavfi.html#ga23fd10b21ee7fca8d0f3f7296d4a4c6d |
| `av_buffersrc_add_frame_flags()` | Push frame into graph | https://ffmpeg.org/doxygen/trunk/buffersrc_8h.html |
| `av_buffersink_get_frame()` | Pull frame from graph | https://ffmpeg.org/doxygen/trunk/buffersink_8h.html |

---

## FFmpeg Source Code

The FFmpeg source code is the ultimate reference for implementation details.

| Resource | URL |
|---|---|
| FFmpeg GitHub mirror | https://github.com/FFmpeg/FFmpeg |
| FFmpeg official Git | https://git.ffmpeg.org/ffmpeg.git |
| **Official C API examples** | https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples |
| `demuxing_decoding.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/demuxing_decoding.c |
| `remuxing.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/remuxing.c |
| `decode_video.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/decode_video.c |
| `encode_video.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/encode_video.c |
| `muxing.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/muxing.c |
| `transcoding.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcoding.c |
| `filtering_video.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/filtering_video.c |
| `filtering_audio.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/filtering_audio.c |
| `resampling_audio.c` | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/resampling_audio.c |
| `hw_decode.c` (GPU decode) | https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/hw_decode.c |

---

## FFmpeg Wiki

The FFmpeg wiki contains practical guides maintained by the community.

| Topic | URL |
|---|---|
| FFmpeg Wiki home | https://trac.ffmpeg.org/wiki |
| H.264 Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/H.264 |
| H.265/HEVC Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/H.265 |
| VP9 Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/VP9 |
| AV1 Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/AV1 |
| AAC Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/AAC |
| MP3 Encoding Guide | https://trac.ffmpeg.org/wiki/Encode/MP3 |
| Streaming Guide | https://trac.ffmpeg.org/wiki/StreamingGuide |
| HLS Streaming | https://trac.ffmpeg.org/wiki/creating%20a%20HLS%20stream |
| Filtering Guide | https://trac.ffmpeg.org/wiki/FilteringGuide |
| Seeking | https://trac.ffmpeg.org/wiki/Seeking |
| Concatenation | https://trac.ffmpeg.org/wiki/Concatenate |
| Hardware Acceleration | https://trac.ffmpeg.org/wiki/HWAccelIntro |
| NVIDIA NVENC | https://trac.ffmpeg.org/wiki/HWAccelIntro#CUDA |
| Compilation (Ubuntu) | https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu |
| Compilation (macOS) | https://trac.ffmpeg.org/wiki/CompilationGuide/macOS |
| Using libavformat/libavcodec | https://trac.ffmpeg.org/wiki/Using%20libavformat%20and%20libavcodec |
| Creating thumbnails | https://trac.ffmpeg.org/wiki/Create%20a%20thumbnail%20image%20every%20X%20seconds |
| Loudness normalization | https://trac.ffmpeg.org/wiki/AudioVolume |

---

## Codec Standards & Specifications

### Video Codecs

| Codec | Standard | Specification / Reference |
|---|---|---|
| H.264 / AVC | ITU-T H.264 | https://www.itu.int/rec/T-REC-H.264/en |
| H.264 / AVC | ISO MPEG-4 Part 10 | https://www.iso.org/standard/75400.html |
| H.265 / HEVC | ITU-T H.265 | https://www.itu.int/rec/T-REC-H.265/en |
| H.265 / HEVC | ISO MPEG-H Part 2 | https://www.iso.org/standard/69668.html |
| VP8 | IETF RFC 6386 | https://datatracker.ietf.org/doc/html/rfc6386 |
| VP9 | WebM Project Spec | https://www.webmproject.org/vp9/ |
| AV1 | AOM Specification | https://aomedia.org/av1/specification/ |
| AV1 | AV1 Bitstream Spec | https://aomediacodec.github.io/av1-spec/ |
| MPEG-2 Video | ISO/IEC 13818-2 | https://www.iso.org/standard/61152.html |

### Audio Codecs

| Codec | Standard / Reference |
|---|---|
| AAC | ISO/IEC 13818-7 (MPEG-2 AAC), 14496-3 (MPEG-4 AAC): https://www.iso.org/standard/43345.html |
| MP3 | ISO/IEC 11172-3 (MPEG-1 Audio Layer 3): https://www.iso.org/standard/22412.html |
| Opus | IETF RFC 6716: https://datatracker.ietf.org/doc/html/rfc6716 |
| Opus | IETF RFC 7845 (in Ogg): https://datatracker.ietf.org/doc/html/rfc7845 |
| Vorbis | Xiph.org spec: https://xiph.org/vorbis/doc/Vorbis_I_spec.html |
| FLAC | Xiph.org spec: https://xiph.org/flac/format.html |
| AC-3 / Dolby Digital | ATSC A/52: https://www.atsc.org/atsc-documents/a52-digital-audio-compression-standard-ac-3/ |

### Container Formats

| Format | Standard / Reference |
|---|---|
| MP4 / ISO BMFF | ISO/IEC 14496-12: https://www.iso.org/standard/83102.html |
| Matroska (MKV) | IETF RFC 9559: https://www.rfc-editor.org/rfc/rfc9559 |
| Matroska Spec | https://matroska.org/technical/specs/index.html |
| WebM | WebM Container Guidelines: https://www.webmproject.org/docs/container/ |
| MPEG-TS | ISO/IEC 13818-1 (MPEG-2 Systems): https://www.iso.org/standard/74427.html |
| FLV | Adobe F4V/FLV Spec: https://www.adobe.com/content/dam/acom/en/devnet/flv/video_file_format_spec_v10.pdf |

---

## Streaming Protocols & Standards

### HLS (HTTP Live Streaming)

| Resource | URL |
|---|---|
| IETF RFC 8216 (HLS) | https://datatracker.ietf.org/doc/html/rfc8216 |
| Apple HLS Overview | https://developer.apple.com/documentation/http-live-streaming |
| Apple HLS Authoring Spec | https://developer.apple.com/documentation/http-live-streaming/hls-authoring-specification-for-apple-devices |
| Low-Latency HLS | https://developer.apple.com/documentation/http-live-streaming/enabling-low-latency-hls |
| M3U8 Playlist Spec (IETF) | https://datatracker.ietf.org/doc/html/rfc8216#section-4 |

### MPEG-DASH

| Resource | URL |
|---|---|
| ISO/IEC 23009-1 (DASH) | https://www.iso.org/standard/83314.html |
| DASH Industry Forum | https://dashif.org |
| DASH-IF Guidelines | https://dashif.org/guidelines/ |
| DASH Content Protection | https://www.w3.org/TR/encrypted-media/ |
| Shaka Packager (DASH/HLS) | https://github.com/shaka-project/shaka-packager |

### RTMP

| Resource | URL |
|---|---|
| Adobe RTMP Spec | https://rtmp.veriskope.com/docs/spec/ |
| Nginx-RTMP module | https://github.com/arut/nginx-rtmp-module |

### SRT (Secure Reliable Transport)

| Resource | URL |
|---|---|
| SRT Alliance | https://www.srtalliance.org |
| SRT Protocol Specification | https://datatracker.ietf.org/doc/html/draft-sharabayko-srt |
| SRT GitHub | https://github.com/Haivision/srt |
| SRT in FFmpeg | https://ffmpeg.org/ffmpeg-protocols.html#srt |

### WebRTC

| Resource | URL |
|---|---|
| W3C WebRTC API | https://www.w3.org/TR/webrtc/ |
| IETF RFC 8825 (WebRTC Overview) | https://datatracker.ietf.org/doc/html/rfc8825 |
| WHIP (WebRTC HTTP Ingest) | https://datatracker.ietf.org/doc/html/draft-ietf-wish-whip |

---

## Audio Standards

### Loudness & Metering

| Standard | Title | Link |
|---|---|---|
| EBU R128 | Loudness Normalisation (European broadcast) | https://tech.ebu.ch/publications/r128 |
| EBU Tech 3341 | Loudness Metering ('EBU Mode') | https://tech.ebu.ch/publications/tech3341 |
| EBU Tech 3343 | Practical Guidelines for EBU R128 | https://tech.ebu.ch/publications/tech3343 |
| ATSC A/85 | Techniques for Establishing and Maintaining Audio Loudness (US broadcast) | https://www.atsc.org/atsc-documents/a85-techniques-for-establishing-and-maintaining-audio-loudness-for-digital-television/ |
| ITU-R BS.1770 | Algorithms to measure audio programme loudness | https://www.itu.int/rec/R-REC-BS.1770/en |
| AES17 | Digital Audio Engineering Standard (dynamic range measurement) | https://www.aes.org/publications/standards/ |

---

## Color Science & HDR

| Resource | URL |
|---|---|
| ITU-R BT.709 (HD color) | https://www.itu.int/rec/R-REC-BT.709/en |
| ITU-R BT.2020 (UHD/HDR color) | https://www.itu.int/rec/R-REC-BT.2020/en |
| ITU-R BT.2100 (HDR transfer functions: PQ and HLG) | https://www.itu.int/rec/R-REC-BT.2100/en |
| SMPTE ST 2084 (PQ transfer function) | https://ieeexplore.ieee.org/document/7291452 |
| HDR10 / SMPTE ST 2086 (mastering display metadata) | https://ieeexplore.ieee.org/document/7291428 |
| Dolby Vision spec | https://professionalsupport.dolby.com/s/article/Dolby-Vision-Bitstream-Specification |
| zscale (zimg) library (used by FFmpeg zscale filter) | https://github.com/sekrit-twc/zimg |
| FFmpeg HDR to SDR guide | https://trac.ffmpeg.org/wiki/colorspace |

---

## Quality Metrics

| Metric | Reference |
|---|---|
| PSNR (Peak Signal-to-Noise Ratio) | https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio |
| SSIM (Structural Similarity Index Measure) | Wang, Z. et al. (2004). https://ece.uwaterloo.ca/~z70wang/research/ssim/ |
| VMAF (Video Multi-Method Assessment Fusion) | Netflix VMAF: https://github.com/Netflix/vmaf |
| VMAF Research Paper | Li, Z. et al. (2018). https://arxiv.org/abs/2001.02163 |
| VMAF Documentation | https://github.com/Netflix/vmaf/blob/master/resource/doc/index.md |

---

## Third-Party Encoder Libraries

| Library | Codec | Homepage | Source Code |
|---|---|---|---|
| x264 | H.264 | https://www.videolan.org/developers/x264.html | https://code.videolan.org/videolan/x264 |
| x265 | H.265 | https://www.videolan.org/developers/x265.html | https://bitbucket.org/multicoreware/x265_git |
| libvpx (VP8/VP9) | VP8, VP9 | https://www.webmproject.org/code/ | https://chromium.googlesource.com/webm/libvpx |
| libaom (AV1) | AV1 | https://aomedia.org | https://aomedia.googlesource.com/aom |
| SVT-AV1 | AV1 | https://github.com/AOMediaCodec/SVT-AV1 | https://github.com/AOMediaCodec/SVT-AV1 |
| rav1e | AV1 | https://github.com/xiph/rav1e | https://github.com/xiph/rav1e |
| libfdk-aac | AAC | https://www.iis.fraunhofer.de/en/ff/amm/broadcast-streaming/aacenc.html | https://github.com/mstorsjo/fdk-aac |
| libopus | Opus | https://opus-codec.org | https://gitlab.xiph.org/xiph/opus |
| LAME (MP3) | MP3 | https://lame.sourceforge.io | https://sourceforge.net/projects/lame/ |

---

## Streaming Server Software

| Software | Purpose | URL |
|---|---|---|
| Nginx-RTMP | RTMP ingest + HLS output | https://github.com/arut/nginx-rtmp-module |
| MediaMTX | Multi-protocol media server | https://github.com/bluenviron/mediamtx |
| Wowza | Enterprise streaming server | https://www.wowza.com |
| Shaka Packager | DASH/HLS packager with DRM | https://github.com/shaka-project/shaka-packager |
| Bento4 | MP4/DASH tools | https://www.bento4.com |
| OBS Studio | Live streaming / recording | https://obsproject.com |

---

## Client-Side Libraries

| Library | Protocol | URL |
|---|---|---|
| HLS.js | HLS | https://github.com/video-dev/hls.js |
| Shaka Player | HLS + DASH + DRM | https://github.com/shaka-project/shaka-player |
| dash.js | MPEG-DASH | https://github.com/Dash-Industry-Forum/dash.js |
| Video.js | Multi-protocol (plugins) | https://videojs.com |
| Plyr | Multi-protocol | https://plyr.io |
| Media Source Extensions (W3C) | Base API for ABR in browsers | https://www.w3.org/TR/media-source/ |

---

## Books & Learning Resources

| Title | Author | Notes |
|---|---|---|
| *Programming with FFmpeg* | Various (FFmpeg docs) | https://ffmpeg.org/documentation.html |
| *The H.264 Advanced Video Compression Standard* | Iain E. Richardson (2010) | Comprehensive H.264 textbook |
| *Video Demystified* | Keith Jack | Container formats and fundamentals |
| FFmpeg Libav Tutorial | Fabio Madeira | https://github.com/leandromoreira/ffmpeg-libav-tutorial |
| dranger FFmpeg tutorial (classic) | Stephen Dranger | http://dranger.com/ffmpeg/ (archived) |
| Digital Video and HD | Charles Poynton | Color science and video fundamentals |
| IETF RFC index for media | Various | https://datatracker.ietf.org/wg/mmusic/documents/ |
