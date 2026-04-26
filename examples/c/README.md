# FFmpeg C API Examples

Practical C programs that demonstrate the FFmpeg C API (libavformat, libavcodec, libavfilter, libswscale).
Each example is heavily commented and includes links to the relevant FFmpeg documentation and source.

---

## Prerequisites

You need the FFmpeg development headers and libraries installed.

### Linux (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install \
  libavcodec-dev \
  libavformat-dev \
  libavfilter-dev \
  libavutil-dev \
  libswscale-dev \
  pkg-config \
  build-essential
```

### macOS
```bash
brew install ffmpeg pkg-config
```

### Fedora / RHEL / Rocky Linux
```bash
sudo dnf install ffmpeg-devel gcc pkg-config
```

### Windows (MSYS2 / MinGW-w64)
```bash
pacman -S mingw-w64-x86_64-ffmpeg mingw-w64-x86_64-gcc pkg-config
```

---

## Build

```bash
# Build all examples
make

# Build a single example
make probe
make transcode

# Clean binaries
make clean
```

---

## Examples

### `probe.c` — Inspect a media file

Equivalent to `ffprobe -v quiet -show_format -show_streams input.mp4`.

Demonstrates:
- `avformat_open_input()` — open any media container
- `avformat_find_stream_info()` — populate stream metadata
- Iterating streams with `AVStream` / `AVCodecParameters`
- Reading metadata with `AVDictionary`
- Handling video, audio, and subtitle stream types

```bash
./probe input.mp4
./probe input.mkv
./probe rtmp://server/live/stream
```

**Key API references:**
- [`avformat_open_input`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49)
- [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb)
- [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html)
- [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html)

---

### `remux.c` — Remux between containers (stream copy)

Equivalent to `ffmpeg -i input.mkv -c copy output.mp4`.

Demonstrates:
- Opening a source container and reading its stream structure
- Creating an output container with `avformat_alloc_output_context2()`
- Copying codec parameters with `avcodec_parameters_copy()` (no decode/encode)
- Rescaling timestamps between timebases with `av_rescale_q_rnd()`
- Writing interleaved packets with `av_interleaved_write_frame()`
- Finalising the container with `av_write_trailer()`

```bash
./remux input.mkv output.mp4
./remux input.mp4 output.ts
```

**Key API references:**
- [`avformat_alloc_output_context2`](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#gad2742b8a7e76ac4adb7e5bd3c4c7dcc9)
- [`avcodec_parameters_copy`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe)
- [`av_rescale_q_rnd`](https://ffmpeg.org/doxygen/trunk/mathematics_8h.html)
- [`av_interleaved_write_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493f160e1086abb3d0dec)
- [Official remuxing example](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/remuxing.c)

---

### `decode_video.c` — Decode video frames to raw YUV

Equivalent to `ffmpeg -i input.mp4 -c:v rawvideo -pix_fmt yuv420p output.yuv`.

Demonstrates:
- `av_find_best_stream()` — select the best video stream automatically
- `avcodec_parameters_to_context()` — configure the decoder from stream info
- The modern **send/receive API** (`avcodec_send_packet` / `avcodec_receive_frame`)
- Flushing the decoder by sending a `NULL` packet at end-of-stream
- Writing planar YUV420P frame data to a file

```bash
./decode_video input.mp4                    # print frame info only
./decode_video input.mp4 output.yuv         # write raw YUV420P

# Play back raw YUV file
ffplay -f rawvideo -pixel_format yuv420p -video_size 1920x1080 output.yuv
```

**Key API references:**
- [`av_find_best_stream`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaa6a9aacc54b9cc13ca0e4bb48e58d55e)
- [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
- [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
- [Official decode_video example](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/decode_video.c)

---

### `encode_video.c` — Encode raw YUV frames to H.264 MP4

Equivalent to `ffmpeg -f lavfi -i testsrc=size=640x480:rate=25 -c:v libx264 -crf 23 output.mp4`.

Demonstrates:
- Finding an encoder with `avcodec_find_encoder_by_name()`
- Configuring H.264 encoder parameters (CRF, preset, GOP, B-frames)
- Setting codec-private options with `av_opt_set()`
- Allocating and filling frame buffers with `av_frame_get_buffer()`
- The **send/receive API** for encoding
- `AV_CODEC_FLAG_GLOBAL_HEADER` — required for MP4/MOV containers
- `avcodec_parameters_from_context()` — populate output stream from encoder

```bash
./encode_video output.mp4
./encode_video output.mp4 1920 1080 30 120     # 1080p, 30fps, 120 frames
```

**Key API references:**
- [`avcodec_find_encoder_by_name`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html)
- [`avcodec_send_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#ga951f35d3da73aa01de3a2c42d80b5e82)
- [`avcodec_receive_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#ga5b9c7b9f5b3e5f0b2dc49a69c5a5c1ea)
- [`av_opt_set`](https://ffmpeg.org/doxygen/trunk/group__opt__set__funcs.html)
- [Official muxing example](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/muxing.c)

---

### `transcode.c` — Full decode → scale → encode pipeline

Equivalent to `ffmpeg -i input.mp4 -vf "scale=1280:720" -c:v libx264 -crf 23 output.mp4`.

Demonstrates:
- Complete decode/encode loop using separate `DecodeCtx` and `EncodeCtx` structs
- Using `libswscale` (`sws_getContext` / `sws_scale`) for resize and pixel format conversion
- Timestamp handling across the full pipeline
- Flushing both decoder and encoder at end-of-stream

```bash
./transcode input.mp4 output.mp4               # same resolution
./transcode input.mp4 output_720p.mp4 1280 720 # scale to 720p
```

**Key API references:**
- [`sws_getContext`](https://ffmpeg.org/doxygen/trunk/swscale_8h.html)
- [`sws_scale`](https://ffmpeg.org/doxygen/trunk/swscale_8h.html#a5a9e9a30d2b63f69a19e41e41cf70d5d)
- [Official transcoding example](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcoding.c)

---

### `filtering.c` — Apply video filters using libavfilter

Equivalent to `ffmpeg -i input.mp4 -vf "scale=640:480" -c:v libx264 -crf 23 output.mp4`.

Demonstrates:
- Allocating a filter graph with `avfilter_graph_alloc()`
- Creating `buffersrc` (input) and `buffersink` (output) filter nodes
- Parsing a filter description string with `avfilter_graph_parse_ptr()`
- Configuring the graph with `avfilter_graph_config()`
- Pushing frames with `av_buffersrc_add_frame_flags()`
- Pulling filtered frames with `av_buffersink_get_frame()`
- Any valid FFmpeg filter graph string is supported (scale, hflip, eq, overlay, etc.)

```bash
./filtering input.mp4 output.mp4
./filtering input.mp4 output.mp4 "scale=1280:720,hflip"
./filtering input.mp4 output.mp4 "eq=brightness=0.1:contrast=1.2:saturation=1.5"
./filtering input.mp4 output.mp4 "yadif"
```

**Key API references:**
- [`avfilter_graph_alloc`](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)
- [`avfilter_graph_parse_ptr`](https://ffmpeg.org/doxygen/trunk/group__lavfi.html#ga76f6b80b5bb595e3a95f5659e4fe9ffc)
- [`av_buffersrc_add_frame_flags`](https://ffmpeg.org/doxygen/trunk/buffersrc_8h.html)
- [`av_buffersink_get_frame`](https://ffmpeg.org/doxygen/trunk/buffersink_8h.html)
- [Official filtering_video example](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/filtering_video.c)
- [FFmpeg filters documentation](https://ffmpeg.org/ffmpeg-filters.html)

---

## FFmpeg C API Quick Reference

### Core Data Structures

| Type | Description | Header |
|---|---|---|
| `AVFormatContext` | Container (mux/demux) state | `libavformat/avformat.h` |
| `AVStream` | A single stream within a container | `libavformat/avformat.h` |
| `AVCodecContext` | Decoder or encoder state | `libavcodec/avcodec.h` |
| `AVCodecParameters` | Codec parameters (codec-independent) | `libavcodec/codec_par.h` |
| `AVPacket` | Encoded data packet | `libavcodec/packet.h` |
| `AVFrame` | Decoded audio/video frame | `libavutil/frame.h` |
| `AVFilterGraph` | Graph of connected filters | `libavfilter/avfilter.h` |
| `AVFilterContext` | Single filter instance | `libavfilter/avfilter.h` |
| `SwsContext` | libswscale context for scaling | `libswscale/swscale.h` |

### Memory Management

FFmpeg uses reference-counted buffers. Always:
- `av_frame_alloc()` / `av_frame_free()` for frames
- `av_packet_alloc()` / `av_packet_free()` for packets
- `av_frame_unref()` after using a frame (release buffer reference)
- `av_packet_unref()` after processing a packet

### Error Handling

```c
int ret = some_ffmpeg_function(...);
if (ret < 0) {
    char errbuf[128];
    av_strerror(ret, errbuf, sizeof(errbuf));
    fprintf(stderr, "Error: %s\n", errbuf);
}
```

Common error codes:
- `AVERROR(EAGAIN)` — try again (needs more input or output was pulled)
- `AVERROR_EOF` — end of stream / decoder fully flushed
- `AVERROR(ENOMEM)` — out of memory
- `AVERROR_DECODER_NOT_FOUND` — no decoder found for codec
- `AVERROR_ENCODER_NOT_FOUND` — no encoder found
