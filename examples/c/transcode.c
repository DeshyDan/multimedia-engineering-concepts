/*
 * transcode.c — Full transcode pipeline: decode → (optional filter) → encode
 * ===========================================================================
 * Opens a source video, decodes the first video stream, optionally applies
 * a scale filter, re-encodes with libx264, and writes to an MP4 container.
 *
 * This demonstrates the complete libavformat + libavcodec pipeline required
 * for any real-world transcoder (VOD conversion, ABR packaging, etc.).
 *
 * Equivalent CLI:
 *   ffmpeg -i input.mp4 -vf "scale=1280:720" -c:v libx264 -crf 23 output.mp4
 *
 * Key APIs used:
 *   avformat_open_input()            — open source container
 *   avformat_find_stream_info()      — inspect streams
 *   av_find_best_stream()            — pick best video stream
 *   avcodec_parameters_to_context()  — copy stream params to decoder context
 *   avcodec_open2()                  — open decoder / encoder
 *   av_read_frame()                  — read compressed packets
 *   avcodec_send_packet()            — send packet to decoder
 *   avcodec_receive_frame()          — get decoded frame
 *   sws_scale()                      — scale/convert pixel format (libswscale)
 *   avcodec_send_frame()             — send raw frame to encoder
 *   avcodec_receive_packet()         — get encoded packet
 *   av_interleaved_write_frame()     — write packet to output container
 *
 * References:
 *   Transcode doc: https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
 *   Example:       https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcoding.c
 *   libswscale:    https://ffmpeg.org/doxygen/trunk/swscale_8h.html
 *   H.264 wiki:    https://trac.ffmpeg.org/wiki/Encode/H.264
 *
 * Build:
 *   gcc transcode.c -o transcode \
 *       $(pkg-config --cflags --libs libavformat libavcodec libavutil libswscale)
 *
 * Usage:
 *   ./transcode input.mp4 output.mp4
 *   ./transcode input.mp4 output.mp4 1280 720    # scale to 1280x720
 */

#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>

/* -------------------------------------------------------------------------
 * Context that holds all state for the decode side
 * ------------------------------------------------------------------------- */
typedef struct DecodeCtx {
    AVFormatContext  *fmt_ctx;
    AVCodecContext   *codec_ctx;
    int               stream_idx;
    AVStream         *stream;
} DecodeCtx;

/* -------------------------------------------------------------------------
 * Context that holds all state for the encode side
 * ------------------------------------------------------------------------- */
typedef struct EncodeCtx {
    AVFormatContext  *fmt_ctx;
    AVCodecContext   *codec_ctx;
    AVStream         *stream;
} EncodeCtx;

/* -------------------------------------------------------------------------
 * Open the input file, find the best video stream, and open its decoder.
 * ------------------------------------------------------------------------- */
static int open_decoder(const char *path, DecodeCtx *ctx)
{
    int ret;

    ret = avformat_open_input(&ctx->fmt_ctx, path, NULL, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open input '%s': %s\n", path, errbuf);
        return ret;
    }

    ret = avformat_find_stream_info(ctx->fmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to get stream info\n");
        return ret;
    }

    const AVCodec *dec = NULL;
    ctx->stream_idx = av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_VIDEO,
                                          -1, -1, &dec, 0);
    if (ctx->stream_idx < 0) {
        fprintf(stderr, "No video stream found in '%s'\n", path);
        return ctx->stream_idx;
    }
    ctx->stream = ctx->fmt_ctx->streams[ctx->stream_idx];

    ctx->codec_ctx = avcodec_alloc_context3(dec);
    if (!ctx->codec_ctx)
        return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(ctx->codec_ctx, ctx->stream->codecpar);
    if (ret < 0)
        return ret;

    ret = avcodec_open2(ctx->codec_ctx, dec, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open decoder: %s\n", errbuf);
    }
    return ret;
}

/* -------------------------------------------------------------------------
 * Create the output MP4 container with a single H.264 video stream.
 * ------------------------------------------------------------------------- */
static int open_encoder(const char *path, EncodeCtx *ctx,
                         int width, int height, AVRational framerate)
{
    int ret;

    ret = avformat_alloc_output_context2(&ctx->fmt_ctx, NULL, NULL, path);
    if (ret < 0 || !ctx->fmt_ctx) {
        fprintf(stderr, "Could not create output context for '%s'\n", path);
        return ret < 0 ? ret : AVERROR(ENOMEM);
    }

    const AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    if (!enc) {
        fprintf(stderr, "libx264 encoder not found\n");
        return AVERROR_ENCODER_NOT_FOUND;
    }

    ctx->stream = avformat_new_stream(ctx->fmt_ctx, NULL);
    if (!ctx->stream)
        return AVERROR(ENOMEM);

    ctx->codec_ctx = avcodec_alloc_context3(enc);
    if (!ctx->codec_ctx)
        return AVERROR(ENOMEM);

    ctx->codec_ctx->width      = width;
    ctx->codec_ctx->height     = height;
    ctx->codec_ctx->pix_fmt    = AV_PIX_FMT_YUV420P;
    ctx->codec_ctx->time_base  = av_inv_q(framerate);  /* 1/fps */
    ctx->codec_ctx->framerate  = framerate;
    ctx->codec_ctx->gop_size   = (int)av_q2d(framerate) * 2;
    ctx->codec_ctx->max_b_frames = 2;

    av_opt_set(ctx->codec_ctx->priv_data, "crf",    "23",     0);
    av_opt_set(ctx->codec_ctx->priv_data, "preset", "medium", 0);

    if (ctx->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        ctx->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(ctx->codec_ctx, enc, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open encoder: %s\n", errbuf);
        return ret;
    }

    ret = avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codec_ctx);
    if (ret < 0)
        return ret;

    ctx->stream->time_base = ctx->codec_ctx->time_base;

    if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ctx->fmt_ctx->pb, path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Could not open output file '%s': %s\n", path, errbuf);
            return ret;
        }
    }

    return avformat_write_header(ctx->fmt_ctx, NULL);
}

/* -------------------------------------------------------------------------
 * Feed one decoded frame through the encoder and write packets to output.
 * Pass frame=NULL to flush the encoder.
 * ------------------------------------------------------------------------- */
static int encode_write_frame(EncodeCtx *enc_ctx, AVFrame *frame,
                               int64_t *pts_counter)
{
    if (frame) {
        frame->pts       = (*pts_counter)++;
        frame->pict_type = AV_PICTURE_TYPE_NONE; /* let encoder decide */
    }

    int ret = avcodec_send_frame(enc_ctx->codec_ctx, frame);
    if (ret < 0 && ret != AVERROR_EOF)
        return ret;

    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
        return AVERROR(ENOMEM);

    while ((ret = avcodec_receive_packet(enc_ctx->codec_ctx, pkt)) == 0) {
        av_packet_rescale_ts(pkt,
                             enc_ctx->codec_ctx->time_base,
                             enc_ctx->stream->time_base);
        pkt->stream_index = enc_ctx->stream->index;

        ret = av_interleaved_write_frame(enc_ctx->fmt_ctx, pkt);
        if (ret < 0)
            break;
    }

    av_packet_free(&pkt);
    return (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) ? 0 : ret;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input> <output> [out_width] [out_height]\n",
                argv[0]);
        return 1;
    }

    const char *in_path  = argv[1];
    const char *out_path = argv[2];
    int out_w = (argc >= 4) ? atoi(argv[3]) : 0;   /* 0 = keep source dims */
    int out_h = (argc >= 5) ? atoi(argv[4]) : 0;

    DecodeCtx  dec  = {0};
    EncodeCtx  enc  = {0};
    struct SwsContext *sws_ctx = NULL;
    AVFrame   *dec_frame  = NULL;
    AVFrame   *enc_frame  = NULL;
    AVPacket  *pkt        = NULL;
    int64_t    pts_counter = 0;
    int        ret         = 0;

    /* ------------------------------------------------------------------ */
    /* Open decoder                                                         */
    /* ------------------------------------------------------------------ */
    ret = open_decoder(in_path, &dec);
    if (ret < 0)
        goto cleanup;

    /* Determine output dimensions (default: same as input) */
    if (out_w == 0) out_w = dec.codec_ctx->width;
    if (out_h == 0) out_h = dec.codec_ctx->height;

    /* Determine frame rate from stream; fall back to 25/1 */
    AVRational framerate = dec.stream->r_frame_rate;
    if (framerate.num == 0 || framerate.den == 0)
        framerate = (AVRational){ 25, 1 };

    printf("Input:  %s  %dx%d  %.3f fps  codec=%s\n",
           in_path,
           dec.codec_ctx->width, dec.codec_ctx->height,
           av_q2d(framerate),
           dec.codec_ctx->codec->name);
    printf("Output: %s  %dx%d  (CRF 23, libx264)\n", out_path, out_w, out_h);

    /* ------------------------------------------------------------------ */
    /* Open encoder                                                         */
    /* ------------------------------------------------------------------ */
    ret = open_encoder(out_path, &enc, out_w, out_h, framerate);
    if (ret < 0)
        goto cleanup;

    /* ------------------------------------------------------------------ */
    /* Set up libswscale for pixel format + optional resize conversion      */
    /* ------------------------------------------------------------------ */
    /*
     * sws_getContext() returns a scaler/converter context.
     * We always convert to YUV420P as required by most H.264 encoders.
     * If the source is already YUV420P at the correct size, this is a no-op.
     *
     * SWS_LANCZOS gives the best downscale quality.
     * SWS_BICUBIC is a good general-purpose choice.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/swscale_8h.html
     */
    sws_ctx = sws_getContext(
        dec.codec_ctx->width,  dec.codec_ctx->height,  dec.codec_ctx->pix_fmt,
        out_w,                 out_h,                  AV_PIX_FMT_YUV420P,
        SWS_LANCZOS, NULL, NULL, NULL);
    if (!sws_ctx) {
        fprintf(stderr, "Could not initialise swscale context\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* Allocate the encoder's input frame */
    enc_frame = av_frame_alloc();
    if (!enc_frame) { ret = AVERROR(ENOMEM); goto cleanup; }
    enc_frame->format = AV_PIX_FMT_YUV420P;
    enc_frame->width  = out_w;
    enc_frame->height = out_h;
    ret = av_frame_get_buffer(enc_frame, 32);
    if (ret < 0) goto cleanup;

    dec_frame = av_frame_alloc();
    pkt       = av_packet_alloc();
    if (!dec_frame || !pkt) { ret = AVERROR(ENOMEM); goto cleanup; }

    /* ------------------------------------------------------------------ */
    /* Main transcode loop                                                  */
    /* ------------------------------------------------------------------ */
    int frame_count = 0;

    while ((ret = av_read_frame(dec.fmt_ctx, pkt)) >= 0) {

        /* Skip non-video packets */
        if (pkt->stream_index != dec.stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        /* --- Decode --- */
        ret = avcodec_send_packet(dec.codec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN))
            break;

        while ((ret = avcodec_receive_frame(dec.codec_ctx, dec_frame)) == 0) {

            /* --- Scale / convert pixel format --- */
            av_frame_make_writable(enc_frame);
            /*
             * sws_scale() converts / scales src → dst.
             * srcSlice[0..7]  = source data planes
             * srcStride[0..7] = source line sizes
             * srcSliceY       = first source row (0 for full frame)
             * srcSliceH       = number of source rows
             *
             * Ref: https://ffmpeg.org/doxygen/trunk/swscale_8h.html#a5a9e9a30d2b63f69a19e41e41cf70d5d
             */
            sws_scale(sws_ctx,
                      (const uint8_t * const *)dec_frame->data, dec_frame->linesize,
                      0, dec.codec_ctx->height,
                      enc_frame->data, enc_frame->linesize);

            /* --- Encode --- */
            ret = encode_write_frame(&enc, enc_frame, &pts_counter);
            if (ret < 0) goto cleanup;

            av_frame_unref(dec_frame);
            frame_count++;

            if (frame_count % 50 == 0)
                printf("  Transcoded %d frames\n", frame_count);
        }

        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
            break;
    }

    /* ------------------------------------------------------------------ */
    /* Flush decoder then encoder                                           */
    /* ------------------------------------------------------------------ */
    avcodec_send_packet(dec.codec_ctx, NULL);
    while (avcodec_receive_frame(dec.codec_ctx, dec_frame) == 0) {
        av_frame_make_writable(enc_frame);
        sws_scale(sws_ctx,
                  (const uint8_t * const *)dec_frame->data, dec_frame->linesize,
                  0, dec.codec_ctx->height,
                  enc_frame->data, enc_frame->linesize);
        encode_write_frame(&enc, enc_frame, &pts_counter);
        av_frame_unref(dec_frame);
        frame_count++;
    }

    encode_write_frame(&enc, NULL, &pts_counter);   /* flush encoder */
    av_write_trailer(enc.fmt_ctx);

    printf("Transcode complete: %d frames → %s\n", frame_count, out_path);
    ret = 0;

cleanup:
    sws_freeContext(sws_ctx);
    av_frame_free(&dec_frame);
    av_frame_free(&enc_frame);
    av_packet_free(&pkt);
    avcodec_free_context(&dec.codec_ctx);
    avformat_close_input(&dec.fmt_ctx);
    avcodec_free_context(&enc.codec_ctx);
    if (enc.fmt_ctx && !(enc.fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&enc.fmt_ctx->pb);
    avformat_free_context(enc.fmt_ctx);
    return ret < 0 ? 1 : 0;
}
