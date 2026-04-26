/*
 * filtering.c — Apply audio/video filters using libavfilter
 * ===========================================================
 * Demonstrates how to construct a libavfilter graph, connect a decoder's
 * output through a filter chain, and feed the filtered frames to an encoder.
 *
 * The default filter applied here is: scale=640:480,drawtext=text='Hello FFmpeg'
 * You can replace FILTER_DESCR with any valid FFmpeg filter graph string.
 *
 * Equivalent CLI:
 *   ffmpeg -i input.mp4 \
 *     -vf "scale=640:480,drawtext=text='Hello FFmpeg':fontcolor=white:fontsize=24" \
 *     -c:v libx264 -crf 23 output.mp4
 *
 * Key APIs used (libavfilter):
 *   avfilter_graph_alloc()           — allocate a filter graph
 *   avfilter_graph_parse_ptr()       — parse a filter graph string
 *   avfilter_graph_config()          — validate and configure the graph
 *   av_buffersrc_add_frame_flags()   — push a decoded frame into the graph
 *   av_buffersink_get_frame()        — pull a filtered frame from the graph
 *   avfilter_graph_free()            — free the graph
 *
 * References:
 *   API:  https://ffmpeg.org/doxygen/trunk/group__lavfi.html
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/filtering_video.c
 *   Wiki: https://trac.ffmpeg.org/wiki/FilteringGuide
 *   Filters: https://ffmpeg.org/ffmpeg-filters.html
 *
 * Build:
 *   gcc filtering.c -o filtering \
 *       $(pkg-config --cflags --libs libavformat libavcodec libavfilter libavutil libswscale)
 *
 * Usage:
 *   ./filtering input.mp4 output.mp4
 *   ./filtering input.mp4 output.mp4 "scale=1280:720,hflip"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>     /* av_buffersink_get_frame() */
#include <libavfilter/buffersrc.h>      /* av_buffersrc_add_frame_flags() */
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>          /* av_get_pix_fmt_name() */

#define DEFAULT_FILTER "scale=640:480"

/* =========================================================================
 * FilterGraph — holds the libavfilter graph and its source/sink pads
 * ========================================================================= */
typedef struct FilterGraph {
    AVFilterGraph   *graph;
    AVFilterContext *src_ctx;   /* buffersrc  — frames go IN  here */
    AVFilterContext *sink_ctx;  /* buffersink — frames come OUT here */
} FilterGraph;

/* -------------------------------------------------------------------------
 * Build and configure a video filter graph.
 *
 * @param fg            Output: filled FilterGraph structure.
 * @param codec_ctx     Decoder codec context (provides width/height/pix_fmt).
 * @param time_base     Stream timebase (used by the buffersrc).
 * @param filter_descr  Filter chain string, e.g. "scale=1280:720,hflip".
 * ------------------------------------------------------------------------- */
static int build_filter_graph(FilterGraph *fg,
                               const AVCodecContext *codec_ctx,
                               AVRational time_base,
                               const char *filter_descr)
{
    int ret;

    /*
     * Allocate a filter graph.
     * A graph is a directed acyclic graph (DAG) of connected filter nodes.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavfi.html
     */
    fg->graph = avfilter_graph_alloc();
    if (!fg->graph)
        return AVERROR(ENOMEM);

    /* ------------------------------------------------------------------ */
    /* Create the buffersrc filter — the graph's input pad                  */
    /* ------------------------------------------------------------------ */
    /*
     * The "buffer" filter acts as the source: decoded frames are pushed
     * into it with av_buffersrc_add_frame_flags().
     *
     * The args string describes the input video properties so that the
     * graph can validate connections between filters.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/buffersrc_8h.html
     */
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             codec_ctx->width,
             codec_ctx->height,
             codec_ctx->pix_fmt,
             time_base.num,
             time_base.den,
             codec_ctx->sample_aspect_ratio.num,
             codec_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&fg->src_ctx, buffersrc,
                                       "in", args, NULL, fg->graph);
    if (ret < 0) {
        fprintf(stderr, "Could not create buffersrc filter\n");
        return ret;
    }

    /* ------------------------------------------------------------------ */
    /* Create the buffersink filter — the graph's output pad               */
    /* ------------------------------------------------------------------ */
    /*
     * The "buffersink" filter collects filtered frames. Pull frames from it
     * with av_buffersink_get_frame().
     *
     * We set acceptable pixel formats via the "pix_fmts" option.
     * Only yuv420p is listed here — the graph will insert a format
     * conversion filter automatically if needed.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/buffersink_8h.html
     */
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&fg->sink_ctx, buffersink,
                                       "out", NULL, NULL, fg->graph);
    if (ret < 0) {
        fprintf(stderr, "Could not create buffersink filter\n");
        return ret;
    }

    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    ret = av_opt_set_int_list(fg->sink_ctx, "pix_fmts",
                              pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        fprintf(stderr, "Could not set output pixel formats\n");
        return ret;
    }

    /* ------------------------------------------------------------------ */
    /* Parse the user-provided filter description and connect src → sink    */
    /* ------------------------------------------------------------------ */
    /*
     * avfilter_graph_parse_ptr() inserts the filter chain specified by
     * filter_descr between the src and sink pads we created above.
     *
     * AVFilterInOut structs represent named pads that the caller provides.
     * The "in" pad is connected to buffersrc; the "out" pad to buffersink.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavfi.html#ga76f6b80b5bb595e3a95f5659e4fe9ffc
     */
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    if (!outputs || !inputs) {
        ret = AVERROR(ENOMEM);
        goto parse_fail;
    }

    outputs->name       = av_strdup("in");
    outputs->filter_ctx = fg->src_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = fg->sink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    ret = avfilter_graph_parse_ptr(fg->graph, filter_descr, &inputs, &outputs, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not parse filter description '%s'\n", filter_descr);
        goto parse_fail;
    }

    /* ------------------------------------------------------------------ */
    /* Validate and configure the graph                                     */
    /* ------------------------------------------------------------------ */
    /*
     * avfilter_graph_config() checks that all filter connections are valid
     * (compatible formats, sizes, etc.) and finalises the graph topology.
     * It must be called before any frames are processed.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavfi.html#ga23fd10b21ee7fca8d0f3f7296d4a4c6d
     */
    ret = avfilter_graph_config(fg->graph, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not configure filter graph\n");
        goto parse_fail;
    }

    printf("Filter graph configured: %s\n", filter_descr);
    printf("Output format: %s  %dx%d\n",
           av_get_pix_fmt_name(av_buffersink_get_format(fg->sink_ctx)),
           av_buffersink_get_w(fg->sink_ctx),
           av_buffersink_get_h(fg->sink_ctx));

parse_fail:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    return ret;
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
                "Usage: %s <input> <output> [filter_description]\n"
                "Example: %s input.mp4 output.mp4 \"scale=1280:720,hflip\"\n",
                argv[0], argv[0]);
        return 1;
    }

    const char *in_path      = argv[1];
    const char *out_path     = argv[2];
    const char *filter_descr = (argc >= 4) ? argv[3] : DEFAULT_FILTER;

    /* --- Decode side --- */
    AVFormatContext  *in_fmt_ctx   = NULL;
    AVCodecContext   *dec_ctx      = NULL;
    int               video_idx    = -1;

    /* --- Filter graph --- */
    FilterGraph       fg           = {0};

    /* --- Encode side --- */
    AVFormatContext  *out_fmt_ctx  = NULL;
    AVCodecContext   *enc_ctx      = NULL;
    AVStream         *out_stream   = NULL;

    AVFrame  *dec_frame  = NULL;
    AVFrame  *filt_frame = NULL;
    AVPacket *pkt        = NULL;
    int64_t   pts_out    = 0;
    int       ret        = 0;
    int       frame_count = 0;

    /* ------------------------------------------------------------------ */
    /* 1. Open input and decoder                                            */
    /* ------------------------------------------------------------------ */
    ret = avformat_open_input(&in_fmt_ctx, in_path, NULL, NULL);
    if (ret < 0) { fprintf(stderr, "Cannot open '%s'\n", in_path); goto cleanup; }

    avformat_find_stream_info(in_fmt_ctx, NULL);

    const AVCodec *dec = NULL;
    video_idx = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (video_idx < 0) {
        fprintf(stderr, "No video stream in '%s'\n", in_path);
        ret = video_idx;
        goto cleanup;
    }

    AVStream *in_stream = in_fmt_ctx->streams[video_idx];
    dec_ctx = avcodec_alloc_context3(dec);
    avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);

    ret = avcodec_open2(dec_ctx, dec, NULL);
    if (ret < 0) { fprintf(stderr, "Cannot open decoder\n"); goto cleanup; }

    printf("Input: %s  %s  %dx%d\n",
           in_path, dec->name, dec_ctx->width, dec_ctx->height);

    /* ------------------------------------------------------------------ */
    /* 2. Build the filter graph                                            */
    /* ------------------------------------------------------------------ */
    ret = build_filter_graph(&fg, dec_ctx, in_stream->time_base, filter_descr);
    if (ret < 0) goto cleanup;

    /* ------------------------------------------------------------------ */
    /* 3. Create output container + encoder                                 */
    /* ------------------------------------------------------------------ */
    ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_path);
    if (ret < 0) { fprintf(stderr, "Cannot create output context\n"); goto cleanup; }

    const AVCodec *enc = avcodec_find_encoder_by_name("libx264");
    if (!enc) {
        fprintf(stderr, "libx264 not found\n");
        ret = AVERROR_ENCODER_NOT_FOUND;
        goto cleanup;
    }

    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    enc_ctx    = avcodec_alloc_context3(enc);

    /* Read output dimensions from the buffersink (set by the filter graph) */
    enc_ctx->width      = av_buffersink_get_w(fg.sink_ctx);
    enc_ctx->height     = av_buffersink_get_h(fg.sink_ctx);
    enc_ctx->pix_fmt    = (enum AVPixelFormat)av_buffersink_get_format(fg.sink_ctx);
    enc_ctx->time_base  = av_buffersink_get_time_base(fg.sink_ctx);
    enc_ctx->framerate  = av_buffersink_get_frame_rate(fg.sink_ctx);
    enc_ctx->gop_size   = 50;
    enc_ctx->max_b_frames = 2;

    av_opt_set(enc_ctx->priv_data, "crf",    "23",     0);
    av_opt_set(enc_ctx->priv_data, "preset", "medium", 0);

    if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(enc_ctx, enc, NULL);
    if (ret < 0) { fprintf(stderr, "Cannot open encoder\n"); goto cleanup; }

    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    out_stream->time_base = enc_ctx->time_base;

    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_fmt_ctx->pb, out_path, AVIO_FLAG_WRITE);
        if (ret < 0) { fprintf(stderr, "Cannot open output file\n"); goto cleanup; }
    }

    ret = avformat_write_header(out_fmt_ctx, NULL);
    if (ret < 0) { fprintf(stderr, "Cannot write header\n"); goto cleanup; }

    /* ------------------------------------------------------------------ */
    /* 4. Allocate frame and packet buffers                                 */
    /* ------------------------------------------------------------------ */
    dec_frame  = av_frame_alloc();
    filt_frame = av_frame_alloc();
    pkt        = av_packet_alloc();
    if (!dec_frame || !filt_frame || !pkt) { ret = AVERROR(ENOMEM); goto cleanup; }

    /* ------------------------------------------------------------------ */
    /* 5. Main decode → filter → encode loop                               */
    /* ------------------------------------------------------------------ */
    while ((ret = av_read_frame(in_fmt_ctx, pkt)) >= 0) {

        if (pkt->stream_index != video_idx) {
            av_packet_unref(pkt);
            continue;
        }

        avcodec_send_packet(dec_ctx, pkt);
        av_packet_unref(pkt);

        while (avcodec_receive_frame(dec_ctx, dec_frame) == 0) {

            dec_frame->pts = dec_frame->best_effort_timestamp;

            /*
             * Push the decoded frame into the filter graph via buffersrc.
             * AV_BUFFERSRC_FLAG_KEEP_REF keeps a reference to the frame's data
             * so we can call av_frame_unref() without disturbing the filter graph.
             *
             * Ref: https://ffmpeg.org/doxygen/trunk/buffersrc_8h.html#a1eb1e4e5bbf68fbd92c0f5a56e7e0f48
             */
            ret = av_buffersrc_add_frame_flags(fg.src_ctx, dec_frame,
                                               AV_BUFFERSRC_FLAG_KEEP_REF);
            if (ret < 0) {
                fprintf(stderr, "Error feeding frame to filter graph\n");
                av_frame_unref(dec_frame);
                goto cleanup;
            }

            /*
             * Pull filtered frames from buffersink until none are available.
             * The graph may buffer frames internally, so we loop until EAGAIN.
             *
             * Ref: https://ffmpeg.org/doxygen/trunk/buffersink_8h.html
             */
            while ((ret = av_buffersink_get_frame(fg.sink_ctx, filt_frame)) >= 0) {

                filt_frame->pts       = pts_out++;
                filt_frame->pict_type = AV_PICTURE_TYPE_NONE;

                /* Encode and write the filtered frame */
                avcodec_send_frame(enc_ctx, filt_frame);

                AVPacket *enc_pkt = av_packet_alloc();
                while (avcodec_receive_packet(enc_ctx, enc_pkt) == 0) {
                    av_packet_rescale_ts(enc_pkt,
                                        enc_ctx->time_base,
                                        out_stream->time_base);
                    enc_pkt->stream_index = out_stream->index;
                    av_interleaved_write_frame(out_fmt_ctx, enc_pkt);
                }
                av_packet_free(&enc_pkt);

                av_frame_unref(filt_frame);
                frame_count++;

                if (frame_count % 50 == 0)
                    printf("  Filtered + encoded %d frames\n", frame_count);
            }

            av_frame_unref(dec_frame);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 6. Flush decoder → filter graph → encoder                           */
    /* ------------------------------------------------------------------ */
    avcodec_send_packet(dec_ctx, NULL);
    while (avcodec_receive_frame(dec_ctx, dec_frame) == 0) {
        dec_frame->pts = dec_frame->best_effort_timestamp;
        if (av_buffersrc_add_frame_flags(fg.src_ctx, dec_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
            break;
        while (av_buffersink_get_frame(fg.sink_ctx, filt_frame) >= 0) {
            filt_frame->pts = pts_out++;
            filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
            avcodec_send_frame(enc_ctx, filt_frame);
            AVPacket *enc_pkt = av_packet_alloc();
            while (avcodec_receive_packet(enc_ctx, enc_pkt) == 0) {
                av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
                enc_pkt->stream_index = out_stream->index;
                av_interleaved_write_frame(out_fmt_ctx, enc_pkt);
            }
            av_packet_free(&enc_pkt);
            av_frame_unref(filt_frame);
            frame_count++;
        }
        av_frame_unref(dec_frame);
    }

    /* Flush encoder */
    avcodec_send_frame(enc_ctx, NULL);
    {
        AVPacket *enc_pkt = av_packet_alloc();
        while (avcodec_receive_packet(enc_ctx, enc_pkt) == 0) {
            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, out_stream->time_base);
            enc_pkt->stream_index = out_stream->index;
            av_interleaved_write_frame(out_fmt_ctx, enc_pkt);
        }
        av_packet_free(&enc_pkt);
    }

    av_write_trailer(out_fmt_ctx);
    printf("Done. %d frames → %s\n", frame_count, out_path);
    ret = 0;

cleanup:
    avfilter_graph_free(&fg.graph);
    av_frame_free(&dec_frame);
    av_frame_free(&filt_frame);
    av_packet_free(&pkt);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&in_fmt_ctx);
    avcodec_free_context(&enc_ctx);
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&out_fmt_ctx->pb);
    avformat_free_context(out_fmt_ctx);
    return ret < 0 ? 1 : 0;
}
