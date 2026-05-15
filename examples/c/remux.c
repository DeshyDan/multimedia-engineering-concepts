/*
 * remux.c — Remux (stream copy) between containers using libavformat
 * ==================================================================
 * Opens an input container, copies all streams (video, audio, subtitles)
 * without re-encoding, and writes them into a new output container.
 *
 * Equivalent CLI: ffmpeg -i input.mkv -c copy output.mp4
 *
 * Key APIs used:
 *   avformat_open_input()     — open source container
 *   avformat_alloc_output_context2() — create output container
 *   avformat_new_stream()     — add a stream to the output
 *   avcodec_parameters_copy() — copy codec parameters between streams
 *   av_interleaved_write_frame() — write a remuxed packet
 *
 * References:
 *   API:  https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/remuxing.c
 *   Wiki: https://trac.ffmpeg.org/wiki/Using%20libavformat%20and%20libavcodec
 *
 * Build:
 *   gcc remux.c -o remux $(pkg-config --cflags --libs libavformat libavcodec libavutil)
 *
 * Usage:
 *   ./remux input.mkv output.mp4
 */

#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>    /* av_rescale_q() */
#include <libavutil/timestamp.h>      /* av_ts2str(), av_ts2timestr() */

/* Helper: log a packet's PTS/DTS/duration before writing */
/* Uncomment calls to log_packet() below to enable packet logging */
__attribute__((unused))
static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt,
                        const char *tag)
{
    AVRational *tb = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("%-3s pts:%-10s pts_time:%-10s dts:%-10s dts_time:%-10s "
           "duration:%-8s duration_time:%-8s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts),        av_ts2timestr(pkt->pts,        tb),
           av_ts2str(pkt->dts),        av_ts2timestr(pkt->dts,        tb),
           av_ts2str(pkt->duration),   av_ts2timestr(pkt->duration,   tb),
           pkt->stream_index);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        fprintf(stderr, "Example: %s input.mkv output.mp4\n", argv[0]);
        return 1;
    }

    const char *in_path  = argv[1];
    const char *out_path = argv[2];

    AVFormatContext *in_ctx  = NULL;
    AVFormatContext *out_ctx = NULL;
    AVPacket        *pkt     = NULL;
    int              ret     = 0;

    /* ------------------------------------------------------------------ */
    /* 1. Open the input container                                          */
    /* ------------------------------------------------------------------ */
    ret = avformat_open_input(&in_ctx, in_path, NULL, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open input '%s': %s\n", in_path, errbuf);
        return 1;
    }

    ret = avformat_find_stream_info(in_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        goto cleanup;
    }

    /* Print a summary of the input to stderr (like ffmpeg's -v info output) */
    av_dump_format(in_ctx, 0, in_path, 0 /* is_output = false */);

    /* ------------------------------------------------------------------ */
    /* 2. Allocate the output format context                               */
    /* ------------------------------------------------------------------ */
    /*
     * avformat_alloc_output_context2() guesses the output format from the
     * file extension. We pass NULL for the explicit format name to let
     * FFmpeg decide (e.g., ".mp4" → mp4 muxer, ".mkv" → matroska muxer).
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#gad2742b8a7e76ac4adb7e5bd3c4c7dcc9
     */
    ret = avformat_alloc_output_context2(&out_ctx, NULL, NULL, out_path);
    if (ret < 0 || !out_ctx) {
        fprintf(stderr, "Could not create output context for '%s'\n", out_path);
        ret = ret < 0 ? ret : AVERROR(ENOMEM);
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 3. Map each input stream to an output stream                        */
    /* ------------------------------------------------------------------ */
    /*
     * stream_mapping[i] = output stream index for input stream i.
     * Set to -1 for streams we want to discard (e.g. data/attachment streams).
     */
    int *stream_mapping = av_calloc(in_ctx->nb_streams, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    int out_stream_idx = 0;
    for (unsigned int i = 0; i < in_ctx->nb_streams; i++) {
        AVStream          *in_stream  = in_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        /* Only copy audio, video, and subtitle streams */
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO  &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO  &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = out_stream_idx++;

        /* Add a new stream to the output container */
        AVStream *out_stream = avformat_new_stream(out_ctx, NULL);
        if (!out_stream) {
            fprintf(stderr, "Failed to allocate output stream\n");
            ret = AVERROR(ENOMEM);
            goto cleanup;
        }

        /*
         * Copy the codec parameters from the input stream to the output stream.
         * This is sufficient for remuxing — no decoder or encoder is needed.
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe
         */
        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            fprintf(stderr, "Failed to copy codec parameters\n");
            goto cleanup;
        }

        /*
         * Reset the codec_tag to 0 so the muxer can choose the correct tag
         * for the output container. Leaving the input tag may cause problems
         * when changing containers (e.g., MKV → MP4).
         */
        out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(out_ctx, 0, out_path, 1 /* is_output = true */);

    /* ------------------------------------------------------------------ */
    /* 4. Open the output file (if it's a file, not a network stream)      */
    /* ------------------------------------------------------------------ */
    /*
     * AVIO_FLAG_WRITE opens the URL for writing.
     * The AVFMT_NOFILE flag means the muxer manages its own I/O
     * (e.g., HLS muxer writes multiple files). Most muxers do NOT have
     * this flag, so we open the file here.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html
     */
    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_ctx->pb, out_path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Could not open output file '%s': %s\n", out_path, errbuf);
            goto cleanup;
        }
    }

    /* ------------------------------------------------------------------ */
    /* 5. Write the container header                                        */
    /* ------------------------------------------------------------------ */
    ret = avformat_write_header(out_ctx, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error writing header: %s\n", errbuf);
        goto cleanup;
    }

    /* ------------------------------------------------------------------ */
    /* 6. Read packets from input and write them to output                 */
    /* ------------------------------------------------------------------ */
    pkt = av_packet_alloc();
    if (!pkt) {
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    while ((ret = av_read_frame(in_ctx, pkt)) >= 0) {
        int in_idx  = pkt->stream_index;
        int out_idx = stream_mapping[in_idx];

        /* Discard streams we don't want to copy */
        if (out_idx < 0) {
            av_packet_unref(pkt);
            continue;
        }

        AVStream *in_stream  = in_ctx->streams[in_idx];
        AVStream *out_stream = out_ctx->streams[out_idx];

        /* (optional) log the packet timestamps before rescaling */
        /* log_packet(in_ctx, pkt, "in"); */

        /*
         * Rescale timestamps from the input stream's timebase to the output
         * stream's timebase. This is essential because different containers
         * use different timebases (e.g., MPEG-TS uses 1/90000, MP4 may use
         * 1/12800). av_rescale_q_rnd() rounds to the nearest integer.
         *
         * AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX preserves AV_NOPTS_VALUE.
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/mathematics_8h.html#a3b1c6a4c2b5d88e8b42e4b0df6e6f3a1
         */
        pkt->pts = av_rescale_q_rnd(pkt->pts,
                                    in_stream->time_base, out_stream->time_base,
                                    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt->dts = av_rescale_q_rnd(pkt->dts,
                                    in_stream->time_base, out_stream->time_base,
                                    AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt->duration = av_rescale_q(pkt->duration,
                                     in_stream->time_base, out_stream->time_base);
        pkt->pos          = -1;          /* reset byte-position hint */
        pkt->stream_index = out_idx;

        /* log_packet(out_ctx, pkt, "out"); */

        /*
         * av_interleaved_write_frame() buffers packets internally to ensure
         * correct interleaving order (required by most muxers). For streams
         * where ordering is already correct, av_write_frame() can be used
         * directly (no internal buffering).
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493f160e1086abb3d0dec
         */
        ret = av_interleaved_write_frame(out_ctx, pkt);
        if (ret < 0) {
            char errbuf[128];
            av_strerror(ret, errbuf, sizeof(errbuf));
            fprintf(stderr, "Error writing packet: %s\n", errbuf);
            goto cleanup;
        }
    }

    /* ------------------------------------------------------------------ */
    /* 7. Write the container trailer (finalises indexes, moov atom, etc.) */
    /* ------------------------------------------------------------------ */
    /*
     * av_write_trailer() must be called after all packets are written.
     * For MP4, this writes the moov atom which makes the file seekable.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13
     */
    av_write_trailer(out_ctx);
    printf("Remux complete: %s → %s\n", in_path, out_path);

cleanup:
    av_packet_free(&pkt);
    av_freep(&stream_mapping);
    avformat_close_input(&in_ctx);
    if (out_ctx && !(out_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&out_ctx->pb);
    avformat_free_context(out_ctx);
    return ret < 0 ? 1 : 0;
}
