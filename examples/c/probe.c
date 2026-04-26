/*
 * probe.c — Inspect a media file using libavformat and libavcodec
 * ================================================================
 * Demonstrates how to open a media file, read its format metadata,
 * and iterate over all streams to print codec, resolution, sample rate,
 * and timing information.
 *
 * Equivalent CLI: ffprobe -v quiet -show_format -show_streams input.mp4
 *
 * References:
 *   API:  https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
 *   Src:  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/demuxing_decoding.c
 *   Wiki: https://trac.ffmpeg.org/wiki/Using%20libavformat%20and%20libavcodec
 *
 * Build:
 *   gcc probe.c -o probe $(pkg-config --cflags --libs libavformat libavcodec libavutil)
 *
 * Usage:
 *   ./probe input.mp4
 */

#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/pixdesc.h>      /* av_get_pix_fmt_name() */
#include <libavutil/pixfmt.h>       /* av_color_space_name(), av_color_primaries_name(), av_color_transfer_name() */

/* Print all metadata key/value tags from a dictionary */
static void print_tags(AVDictionary *tags, const char *indent)
{
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(tags, "", tag, AV_DICT_IGNORE_SUFFIX)))
        printf("%s  %-20s = %s\n", indent, tag->key, tag->value);
}

/* Evaluate a rational fps fraction (e.g., 30000/1001 → 29.97) */
static double fps_to_double(AVRational r)
{
    if (r.den == 0)
        return 0.0;
    return (double)r.num / (double)r.den;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    AVFormatContext *fmt_ctx = NULL;
    int ret;

    /*
     * avformat_open_input() reads the container header and fills fmt_ctx.
     * Pass NULL for AVInputFormat to auto-detect the container format.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
     */
    ret = avformat_open_input(&fmt_ctx, input_path, NULL, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not open '%s': %s\n", input_path, errbuf);
        return 1;
    }

    /*
     * avformat_find_stream_info() reads packets from the file to populate
     * stream information that may not be available in the header alone.
     * This is necessary for formats like MPEG-TS or raw streams.
     *
     * Ref: https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
     */
    ret = avformat_find_stream_info(fmt_ctx, NULL);
    if (ret < 0) {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Could not find stream info: %s\n", errbuf);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    /* ------------------------------------------------------------------ */
    /* Format / Container information                                       */
    /* ------------------------------------------------------------------ */
    printf("=== Format ===\n");
    printf("  File:       %s\n", fmt_ctx->url);
    printf("  Container:  %s (%s)\n",
           fmt_ctx->iformat->name,
           fmt_ctx->iformat->long_name ? fmt_ctx->iformat->long_name : "");

    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        int64_t dur_sec  = fmt_ctx->duration / AV_TIME_BASE;
        int64_t dur_ms   = (fmt_ctx->duration % AV_TIME_BASE) / 1000;
        printf("  Duration:   %lld:%02lld:%02lld.%03lld\n",
               (long long)(dur_sec / 3600),
               (long long)((dur_sec % 3600) / 60),
               (long long)(dur_sec % 60),
               (long long)dur_ms);
    }

    if (fmt_ctx->bit_rate > 0)
        printf("  Bit rate:   %lld kbps\n", (long long)(fmt_ctx->bit_rate / 1000));

    printf("  Streams:    %u\n", fmt_ctx->nb_streams);

    if (fmt_ctx->metadata) {
        printf("  Metadata:\n");
        print_tags(fmt_ctx->metadata, "  ");
    }

    /* ------------------------------------------------------------------ */
    /* Stream information                                                   */
    /* ------------------------------------------------------------------ */
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream         *stream  = fmt_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        /*
         * Find the codec descriptor for display purposes (no need to open it).
         * avcodec_find_decoder() returns NULL for codecs not compiled in.
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga6258c831b3e4e4462b54da1b1bdd4a7a
         */
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);

        printf("\n=== Stream #%u ===\n", i);
        printf("  Codec:      %s (%s)\n",
               codec ? codec->name      : "unknown",
               codec ? codec->long_name : "");
        printf("  Type:       %s\n", av_get_media_type_string(codecpar->codec_type));

        /*
         * Timebase: the unit used by PTS/DTS values in this stream.
         * Timestamps in AVPacket are in stream timebase units.
         *
         * Ref: https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6
         */
        printf("  Timebase:   %d/%d\n", stream->time_base.num, stream->time_base.den);

        if (stream->duration != AV_NOPTS_VALUE) {
            double dur_sec = (double)stream->duration * av_q2d(stream->time_base);
            printf("  Duration:   %.3f s\n", dur_sec);
        }

        if (codecpar->bit_rate > 0)
            printf("  Bit rate:   %lld kbps\n", (long long)(codecpar->bit_rate / 1000));

        switch (codecpar->codec_type) {

        case AVMEDIA_TYPE_VIDEO:
            /*
             * Video stream properties.
             * r_frame_rate is the 'real base framerate' — best guess at fps.
             * avg_frame_rate is the average fps measured from the container.
             *
             * Ref: https://ffmpeg.org/doxygen/trunk/structAVStream.html
             * Pixel format ref: https://ffmpeg.org/doxygen/trunk/pixfmt_8h.html
             */
            printf("  Resolution: %dx%d\n", codecpar->width, codecpar->height);
            printf("  FPS:        %.3f (real: %.3f)\n",
                   fps_to_double(stream->avg_frame_rate),
                   fps_to_double(stream->r_frame_rate));
            printf("  Pix fmt:    %s\n",
                   av_get_pix_fmt_name((enum AVPixelFormat)codecpar->format));
            printf("  Profile:    %s\n",
                   avcodec_profile_name(codecpar->codec_id, codecpar->profile)
                       ? avcodec_profile_name(codecpar->codec_id, codecpar->profile)
                       : "unknown");
            printf("  Level:      %d\n", codecpar->level);

            if (codecpar->color_space != AVCOL_SPC_UNSPECIFIED)
                printf("  Color spc:  %s\n",
                       av_color_space_name(codecpar->color_space));
            if (codecpar->color_primaries != AVCOL_PRI_UNSPECIFIED)
                printf("  Primaries:  %s\n",
                       av_color_primaries_name(codecpar->color_primaries));
            if (codecpar->color_trc != AVCOL_TRC_UNSPECIFIED)
                printf("  Transfer:   %s\n",
                       av_color_transfer_name(codecpar->color_trc));
            break;

        case AVMEDIA_TYPE_AUDIO:
            /*
             * Audio stream properties.
             *
             * Ref: https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
             * Sample fmt: https://ffmpeg.org/doxygen/trunk/samplefmt_8h.html
             */
            printf("  Sample rate:%d Hz\n", codecpar->sample_rate);
            printf("  Channels:   %d\n", codecpar->ch_layout.nb_channels);
            printf("  Sample fmt: %s\n",
                   av_get_sample_fmt_name((enum AVSampleFormat)codecpar->format));
            break;

        case AVMEDIA_TYPE_SUBTITLE:
            printf("  (subtitle stream)\n");
            break;

        default:
            printf("  (other stream type: %d)\n", codecpar->codec_type);
        }

        if (stream->metadata) {
            printf("  Metadata:\n");
            print_tags(stream->metadata, "  ");
        }
    }

    /* Always free resources */
    avformat_close_input(&fmt_ctx);
    return 0;
}
