/*
 * decode_video.c — Decode video frames to raw YUV using libavcodec
 * ================================================================
 * Opens a video file, finds the first video stream, opens the decoder,
 * and decodes all frames. Each decoded frame is written as raw YUV data
 * to a file (or the frame dimensions printed if no output file is given).
 *
 * Equivalent CLI: ffmpeg -i input.mp4 -c:v rawvideo -pix_fmt yuv420p output.yuv
 *
 * Key APIs used:
 *   avformat_open_input()           — open container
 *   av_find_best_stream()           — find the best video stream
 *   avcodec_find_decoder()          — get decoder for codec
 *   avcodec_alloc_context3()        — allocate codec context
 *   avcodec_parameters_to_context() — copy stream params to codec context
 *   avcodec_open2()                 — open the decoder
 *   avcodec_send_packet()           — feed compressed packet to decoder
 *   avcodec_receive_frame()         — retrieve decoded frame
 *
 * References:
 *   API:  https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/decode_video.c
 *   Wiki: https://trac.ffmpeg.org/wiki/Using%20libavcodec%20Decoding
 *
 * Build:
 *   gcc decode_video.c -o decode_video \
 *       $(pkg-config --cflags --libs libavformat libavcodec libavutil)
 *
 * Usage:
 *   ./decode_video input.mp4                  # print frame info only
 *   ./decode_video input.mp4 output.yuv       # write raw YUV420P frames
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>    /* av_image_get_buffer_size() */

/* Write one YUV420P frame to a file (Y plane, then U plane, then V plane) */
static int write_yuv420p_frame(FILE *fp, AVFrame *frame)
{
    int width  = frame->width;
    int height = frame->height;

    /* Y (luma) plane: full width × height */
    for (int y = 0; y < height; y++) {
        if (fwrite(frame->data[0] + y * frame->linesize[0], 1, width, fp) != (size_t)width)
            return -1;
    }

    /* U (Cb) plane: half width × half height for 4:2:0 */
    for (int y = 0; y < height / 2; y++) {
        if (fwrite(frame->data[1] + y * frame->linesize[1], 1, width / 2, fp) != (size_t)(width / 2))
            return -1;
    }

    /* V (Cr) plane: half width × half height for 4:2:0 */
    for (int y = 0; y < height / 2; y++) {
        if (fwrite(frame->data[2] + y * frame->linesize[2], 1, width / 2, fp) != (size_t)(width / 2))
            return -1;
    }

    return 0;
}

/* Decode all buffered frames from the codec context */
static int decode_frames(AVCodecContext *codec_ctx, AVFrame *frame,
                          FILE *out_fp, int *frame_count)
{
    int ret;
    while ((ret = avcodec_receive_frame(codec_ctx, frame)) == 0) {

        (*frame_count)++;
        printf("  Frame #%d  pts=%-8lld  fmt=%s  %dx%d\n",
               *frame_count,
               (long long)frame->pts,
               av_get_pix_fmt_name((enum AVPixelFormat)frame->format),
               frame->width,
               frame->height);

        if (out_fp) {
            /*
             * Only YUV420P is handled by write_yuv420p_frame().
             * Other pixel formats would need swscale conversion first.
             */
            if (frame->format != AV_PIX_FMT_YUV420P) {
                fprintf(stderr, "  Warning: pixel format is not yuv420p; "
                                "skipping raw write for this frame.\n");
            } else {
                if (write_yuv420p_frame(out_fp, frame) < 0)
                    fprintf(stderr, "  Warning: write error for frame %d\n", *frame_count);
            }
        }

        av_frame_unref(frame);
    }

    /* AVERROR(EAGAIN) means the decoder needs more input — not an error */
    if (ret == AVERROR(EAGAIN))
        return 0;

    /* AVERROR_EOF is returned after flushing (NULL packet) — not an error */
    if (ret == AVERROR_EOF)
        return 0;

    return ret;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [output.yuv]\n", argv[0]);
        return 1;
    }

    const char *in_path  = argv[1];
    const char *out_path = (argc >= 3) ? argv[2] : NULL;

    AVFormatContext *fmt_ctx   = NULL;
    AVCodecContext  *codec_ctx = NULL;
    AVFrame         *frame     = NULL;
    AVPacket        *pkt       = NULL;
    FILE            *out_fp    = NULL;
    int              ret       = 0;
    int              frame_count = 0;

    /* ------------------------------------------------------------------ */
    /* 1. Open the input file and find stream info                          */
    /* ------------------------------------------------------------------ */
    ret = avformat_open_input(&fmt_ctx, in_path, NULL, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open '%s': %s\n", in_path, errbuf);
        return 1;
    }

    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream info\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 2. Select the best video stream                                      */
    /* ------------------------------------------------------------------ */
    /*
     * av_find_best_stream() searches for the stream with the highest
     * resolution/quality score. Pass NULL for the wanted_nb_codec to
     * accept any decoder. The returned decoder is stored in &codec.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gaa6a9aacc54b9cc13ca0e4bb48e58d55e
     */
    const AVCodec *codec = NULL;
    int video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO,
                                               -1, -1, &codec, 0);
    if (video_stream_idx < 0) {
        fprintf(stderr, "Could not find a video stream in '%s'\n", in_path);
        ret = video_stream_idx;
        goto cleanup;
    }

    AVStream *video_stream = fmt_ctx->streams[video_stream_idx];
    printf("Video stream #%d: %s  %dx%d  %.3f fps\n",
           video_stream_idx,
           codec->name,
           video_stream->codecpar->width,
           video_stream->codecpar->height,
           (double)video_stream->r_frame_rate.num /
               (double)video_stream->r_frame_rate.den);

    /* ------------------------------------------------------------------ */
    /* 3. Allocate and configure the codec context                          */
    /* ------------------------------------------------------------------ */
    /*
     * avcodec_alloc_context3() allocates an AVCodecContext initialised
     * with codec-specific defaults.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga9f6ea8f69fb3a20cc740daf52f7bf47d
     */
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate codec context\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /*
     * Copy codec parameters from the stream to the codec context.
     * This fills fields like width, height, pix_fmt, sample_rate etc.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d6b
     */
    ret = avcodec_parameters_to_context(codec_ctx, video_stream->codecpar);
    if (ret < 0) {
        fprintf(stderr, "Could not copy codec parameters\n");
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 4. Open the decoder                                                  */
    /* ------------------------------------------------------------------ */
    /*
     * avcodec_open2() initialises the codec context and prepares it for
     * decoding/encoding. The third argument allows setting AVOptions.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
     */
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open codec: %s\n", errbuf);
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 5. Allocate frame and packet buffers                                 */
    /* ------------------------------------------------------------------ */
    frame = av_frame_alloc();
    pkt   = av_packet_alloc();
    if (!frame || !pkt) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    if (out_path) {
        out_fp = fopen(out_path, "wb");
        if (!out_fp) {
            fprintf(stderr, "Could not open output file '%s'\n", out_path);
            ret = AVERROR(EIO);
            goto cleanup;
        }
        printf("Writing raw YUV420P frames to: %s\n", out_path);
        printf("Playback: ffplay -f rawvideo -pixel_format yuv420p -video_size %dx%d %s\n",
               video_stream->codecpar->width,
               video_stream->codecpar->height,
               out_path);
    }

    printf("\nDecoding frames...\n");

    /* ------------------------------------------------------------------ */
    /* 6. Read packets and decode frames                                    */
    /* ------------------------------------------------------------------ */
    /*
     * The send/receive API (introduced in FFmpeg 3.1) decouples packet
     * feeding from frame retrieval. One packet may produce multiple frames
     * (e.g., after a B-frame reorder buffer flush), and multiple packets
     * may be needed to produce one frame (e.g., B-frames at start of GOP).
     *
     * Send-receive loop:
     *   av_read_frame() → av_packet → avcodec_send_packet()
     *                                  ↓
     *                              avcodec_receive_frame() (loop until EAGAIN)
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html
     */
    while ((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {

        /* Skip packets that don't belong to our video stream */
        if (pkt->stream_index != video_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        /*
         * avcodec_send_packet() sends a compressed packet to the decoder.
         * Returns 0 on success, AVERROR(EAGAIN) if the decoder is full
         * (unlikely with normal send/receive loops), or a negative error.
         */
        ret = avcodec_send_packet(codec_ctx, pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Error sending packet to decoder: %s\n", errbuf);
            av_packet_unref(pkt);
            break;
        }

        av_packet_unref(pkt);   /* release reference after sending */

        /* Drain all available decoded frames */
        ret = decode_frames(codec_ctx, frame, out_fp, &frame_count);
        if (ret < 0)
            break;
    }

    /* ------------------------------------------------------------------ */
    /* 7. Flush the decoder (drain remaining buffered frames)               */
    /* ------------------------------------------------------------------ */
    /*
     * Sending NULL to avcodec_send_packet() signals end-of-stream to the
     * decoder. Some codecs (especially those with B-frames like H.264/H.265)
     * buffer frames internally and only release them on flush.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395797f7f2d92b79ec1fb8b36aabd8e
     */
    avcodec_send_packet(codec_ctx, NULL);
    decode_frames(codec_ctx, frame, out_fp, &frame_count);

    printf("\nDecoded %d frames.\n", frame_count);
    if (out_path)
        printf("Output written to: %s\n", out_path);

    ret = 0;   /* success */

cleanup:
    if (out_fp)   fclose(out_fp);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    return ret < 0 ? 1 : 0;
}
