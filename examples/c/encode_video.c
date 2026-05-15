/*
 * encode_video.c — Encode raw YUV frames to H.264 using libx264 via libavcodec
 * =============================================================================
 * Generates synthetic YUV420P video frames, encodes them with libx264 via the
 * avcodec API, and writes the compressed bitstream to an MP4 file.
 *
 * Equivalent CLI: ffmpeg -f lavfi -i testsrc=size=640x480:rate=25 \
 *                        -c:v libx264 -crf 23 output.mp4
 *
 * Key APIs used:
 *   avcodec_find_encoder_by_name()  — look up an encoder
 *   avcodec_alloc_context3()        — allocate encoder context
 *   avcodec_open2()                 — open the encoder
 *   av_frame_alloc() / av_frame_get_buffer() — allocate frame buffer
 *   avcodec_send_frame()            — feed raw frame to encoder
 *   avcodec_receive_packet()        — retrieve compressed packet
 *   avformat_alloc_output_context2()— create MP4 container
 *   avformat_write_header()         — write container header
 *   av_interleaved_write_frame()    — write compressed packet to container
 *   av_write_trailer()              — finalise container
 *
 * References:
 *   API:  https://ffmpeg.org/doxygen/trunk/group__lavc__encoding.html
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/encode_video.c
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/muxing.c
 *   Wiki: https://trac.ffmpeg.org/wiki/Encode/H.264
 *
 * Build:
 *   gcc encode_video.c -o encode_video \
 *       $(pkg-config --cflags --libs libavformat libavcodec libavutil libswscale)
 *
 * Usage:
 *   ./encode_video output.mp4
 *   ./encode_video output.mp4 640 480 25 50
 *             width^  height^  fps^  frames^
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>          /* av_opt_set() */
#include <libavutil/imgutils.h>     /* av_image_alloc() */
#include <libavutil/mathematics.h>  /* av_rescale_q() */

/* -------------------------------------------------------------------------
 * Generate a simple moving test pattern in YUV420P format.
 * Y: horizontal gradient + frame offset
 * U, V: static colour ramp
 * ------------------------------------------------------------------------- */
static void fill_yuv_image(AVFrame *frame, int frame_index, int width, int height)
{
    /* Y plane (luma) */
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            frame->data[0][y * frame->linesize[0] + x] =
                (uint8_t)((x + y + frame_index * 3) & 0xFF);

    /* U (Cb) and V (Cr) planes at half resolution for 4:2:0 */
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            frame->data[1][y * frame->linesize[1] + x] = (uint8_t)(128 + y + frame_index * 2);
            frame->data[2][y * frame->linesize[2] + x] = (uint8_t)( 64 + x + frame_index * 5);
        }
    }
}

/* -------------------------------------------------------------------------
 * Drain the encoder and write all pending packets to the container.
 * Called once normally, and once with frame==NULL to flush.
 * ------------------------------------------------------------------------- */
static int encode_and_write(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx,
                              AVStream *stream, AVFrame *frame)
{
    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
        return AVERROR(ENOMEM);

    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0 && ret != AVERROR_EOF) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error sending frame to encoder: %s\n", errbuf);
        av_packet_free(&pkt);
        return ret;
    }

    /*
     * avcodec_receive_packet() may return multiple packets per frame (rare)
     * or AVERROR(EAGAIN) if the encoder needs more frames first (common with
     * lookahead-based encoders like libx264 with B-frame reordering).
     */
    while ((ret = avcodec_receive_packet(codec_ctx, pkt)) == 0) {

        /*
         * Rescale the packet timestamps from the encoder's timebase
         * (typically 1/fps, e.g., 1/25) to the output stream's timebase
         * (typically 1/90000 for MP4 or 1/12800, depending on the muxer).
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/mathematics_8h.html
         */
        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;

        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Error writing packet: %s\n", errbuf);
            av_packet_free(&pkt);
            return ret;
        }
    }

    av_packet_free(&pkt);
    return (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) ? 0 : ret;
}

int main(int argc, char *argv[])
{
    const char *out_path = (argc >= 2) ? argv[1] : "output.mp4";
    int width   = (argc >= 3) ? atoi(argv[2]) : 640;
    int height  = (argc >= 4) ? atoi(argv[3]) : 480;
    int fps     = (argc >= 5) ? atoi(argv[4]) : 25;
    int n_frames = (argc >= 6) ? atoi(argv[5]) : 50;

    AVFormatContext *fmt_ctx   = NULL;
    AVCodecContext  *codec_ctx = NULL;
    AVFrame         *frame     = NULL;
    AVStream        *stream    = NULL;
    int              ret       = 0;

    printf("Encoding %d frames at %dx%d @ %d fps → %s\n",
           n_frames, width, height, fps, out_path);

    /* ------------------------------------------------------------------ */
    /* 1. Find the H.264 encoder (libx264)                                  */
    /* ------------------------------------------------------------------ */
    /*
     * avcodec_find_encoder_by_name() looks up an encoder by its short name.
     * Alternative: avcodec_find_encoder(AV_CODEC_ID_H264) picks any H.264
     * encoder that is compiled in (may select a HW encoder before libx264).
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html
     */
    const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        fprintf(stderr, "libx264 encoder not found. "
                        "Recompile FFmpeg with --enable-libx264.\n");
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* 2. Allocate the output container                                     */
    /* ------------------------------------------------------------------ */
    ret = avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, out_path);
    if (ret < 0 || !fmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Add a video stream and configure the encoder                      */
    /* ------------------------------------------------------------------ */
    stream = avformat_new_stream(fmt_ctx, NULL);
    if (!stream) {
        fprintf(stderr, "Could not allocate stream\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }
    stream->id = 0;

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /*
     * Configure encoder parameters.
     *
     * time_base: the timebase used for PTS values fed to the encoder.
     *   We use 1/fps so PTS = frame number (0, 1, 2, …).
     *
     * gop_size: distance between I-frames (keyframes). Shorter = better
     *   seek accuracy but lower compression. Default in libx264: 250.
     *
     * max_b_frames: maximum consecutive B-frames. B-frames improve
     *   compression but add encoder latency. Set to 0 for low-latency.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
     */
    codec_ctx->width      = width;
    codec_ctx->height     = height;
    codec_ctx->time_base  = (AVRational){ 1, fps };
    codec_ctx->framerate  = (AVRational){ fps, 1 };
    codec_ctx->gop_size   = fps * 2;    /* keyframe every 2 seconds */
    codec_ctx->max_b_frames = 2;
    codec_ctx->pix_fmt    = AV_PIX_FMT_YUV420P;

    /*
     * Set libx264-specific options using av_opt_set().
     * "crf" = Constant Rate Factor (quality). Lower = better, ~23 is default.
     * "preset" = encoding speed/quality tradeoff.
     *
     * Ref: https://trac.ffmpeg.org/wiki/Encode/H.264
     * Ref: https://ffmpeg.org/doxygen/trunk/group__opt__set__funcs.html
     */
    av_opt_set(codec_ctx->priv_data, "crf",    "23",      0);
    av_opt_set(codec_ctx->priv_data, "preset", "medium",  0);

    /*
     * Some muxers (e.g., MP4) require global headers in the codec context.
     * The AV_CODEC_FLAG_GLOBAL_HEADER flag tells the encoder to place
     * extra data (SPS/PPS for H.264) in codec_ctx->extradata rather than
     * in every packet, as required by the MP4 container.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/avcodec_8h.html
     */
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    /* ------------------------------------------------------------------ */
    /* 4. Open the encoder                                                  */
    /* ------------------------------------------------------------------ */
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open encoder: %s\n", errbuf);
        goto cleanup;
    }

    /*
     * Copy the encoder's codec parameters to the stream.
     * This populates stream->codecpar (SPS, PPS, width, height, etc.)
     * which the muxer uses to write the container header.
     */
    ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0) {
        fprintf(stderr, "Could not copy codec parameters\n");
        goto cleanup;
    }

    /*
     * The stream timebase should match the output container's expectation.
     * Most muxers will adjust it; setting it equal to the codec timebase
     * is a safe starting point.
     */
    stream->time_base = codec_ctx->time_base;

    av_dump_format(fmt_ctx, 0, out_path, 1);

    /* ------------------------------------------------------------------ */
    /* 5. Open output file and write container header                       */
    /* ------------------------------------------------------------------ */
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, out_path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Could not open output file '%s': %s\n", out_path, errbuf);
            goto cleanup;
        }
    }

    ret = avformat_write_header(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error writing container header\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 6. Allocate the input frame                                          */
    /* ------------------------------------------------------------------ */
    frame = av_frame_alloc();
    if (!frame) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }
    frame->format = codec_ctx->pix_fmt;
    frame->width  = codec_ctx->width;
    frame->height = codec_ctx->height;

    /*
     * av_frame_get_buffer() allocates the data buffers for the frame,
     * aligned to a multiple of 32 bytes for SIMD compatibility.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavu__frame.html
     */
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame buffers\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 7. Encode frames                                                     */
    /* ------------------------------------------------------------------ */
    for (int i = 0; i < n_frames; i++) {

        /*
         * av_frame_make_writable() ensures the frame's data buffers are
         * writable. Required before filling data; no-op if already writable.
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/group__lavu__frame.html
         */
        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            fprintf(stderr, "Could not make frame writable\n");
            goto cleanup;
        }

        fill_yuv_image(frame, i, width, height);

        /*
         * PTS (Presentation Timestamp) must be monotonically increasing.
         * Since our timebase is 1/fps, PTS == frame index gives exactly
         * one frame per timebase unit (i.e., 1/fps seconds per frame).
         */
        frame->pts = i;

        ret = encode_and_write(codec_ctx, fmt_ctx, stream, frame);
        if (ret < 0)
            goto cleanup;

        if ((i + 1) % 10 == 0)
            printf("  Encoded frame %d/%d\n", i + 1, n_frames);
    }

    /* ------------------------------------------------------------------ */
    /* 8. Flush the encoder (send NULL frame)                               */
    /* ------------------------------------------------------------------ */
    printf("Flushing encoder...\n");
    ret = encode_and_write(codec_ctx, fmt_ctx, stream, NULL);
    if (ret < 0)
        goto cleanup;

    /* ------------------------------------------------------------------ */
    /* 9. Write the container trailer                                       */
    /* ------------------------------------------------------------------ */
    av_write_trailer(fmt_ctx);
    printf("Done. Output written to: %s\n", out_path);
    ret = 0;

cleanup:
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    if (fmt_ctx && !(fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&fmt_ctx->pb);
    avformat_free_context(fmt_ctx);
    return ret < 0 ? 1 : 0;
}
