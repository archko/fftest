//
// Created by archko on 2023/9/16.
//

#ifndef FFTEST_TEST_COPY_H
#define FFTEST_TEST_COPY_H

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

/**
 * 最简单的基于FFMPEG的封装格式转换器（无编解码）
 * https://blog.csdn.net/leixiaohua1020/article/details/25422685
 */
static int test_copy() {
    const char *in_filename = "/Users/archko/Movie/健身气功八段锦-clip.mp4";
    const char *out_filename = "/Users/archko/Movie/code.mp4";
    const AVOutputFormat *ofmt = NULL;
    //输入对应一个AVFormatContext，输出对应一个AVFormatContext
    //（Input AVFormatContext and Output AVFormatContext）
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;

    // 输入（Input）
    ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0);
    if (ret < 0) {
        printf("Could not open input file.");
        return 0;
    }
    ret = avformat_find_stream_info(ifmt_ctx, 0);
    if (ret < 0) {
        printf("Failed to retrieve input stream information");
        return 0;
    }
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    //输出（Output）
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return 0;
    }
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        //根据输入流创建输出流（Create output AVStream according to input AVStream）
        AVStream *in_stream = ifmt_ctx->streams[i];
        const AVCodec *codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, codec);
        if (!out_stream) {
            printf("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            return 0;
        }
        //复制AVCodecContext的设置（Copy the settings of AVCodecContext）
        AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
        ret = avcodec_parameters_to_context(codec_ctx, in_stream->codecpar);
        if (ret < 0) {
            printf("Failed to copy context from input to output stream codec context\n");
            return 0;
        }

        ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
        if (ret < 0) {
            printf("Failed to copy codec context to out_stream codecpar context\n");
            return 0;
        }

        out_stream->codecpar->codec_tag = 0;
        //leixiaohua1020 在这是有这段操作的,6.0不需要这样,然后正常复制视频
        //if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        //    out_stream->codecpar->codec_tag |= AV_CODEC_FLAG_GLOBAL_HEADER;
        //}
    }
    //输出一下格式------------------
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    //打开输出文件（Open output file）
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file '%s'", out_filename);
            return 0;
        }
    }
    //写文件头（Write file header）
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output file\n");
        return 0;
    }
    int frame_index = 0;
    while (1) {
        AVStream *in_stream, *out_stream;
        //获取一个AVPacket（Get an AVPacket）
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        /* copy packet */
        //转换PTS/DTS（Convert PTS/DTS）
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base,
                                   (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        //写入（Write）
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            printf("Error muxing packet\n");
            break;
        }
        //printf("Write %8d frames to output file\n", frame_index);
        av_packet_unref(&pkt);
        frame_index++;
    }
    //写文件尾（Write file trailer）
    av_write_trailer(ofmt_ctx);
    end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
        avio_close(ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    printf("Copy success.\n");
    return 0;
};


#endif //FFTEST_TEST_COPY_H
