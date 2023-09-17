//
//  decode_video.c
//  FFTest
//
//  Created by archko on 2023/9/14.
//

/**
 * @file
 * video decoding with libavcodec API example
 *
 * @example decode_video.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
};
#endif
#endif

#define INBUF_SIZE 4096

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename) {
    FILE *f;
    int i;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++) {
        fwrite(buf + i * wrap, 1, xsize, f);
    }
    fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename) {
    char buf[1024];
    int ret;
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);
        // the picture is allocated by the decoder. no need to free it
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
    }
}

static int decode_video() {
    const char *filename, *outfilename;
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx = NULL;
    const AVCodec *pCodec;
    AVFrame *pFrame;
    AVPacket *pPacket;

    AVCodecParserContext *pParserContext;
    FILE *file;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    int ret;

    filename = "/Users/archko/Movie/健身气功八段锦-clip.mp4";
    outfilename = "/Users/archko/Movie/code.mp4";
    // set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams)
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) < 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    //pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec); //pCodec不传会用默认参数

    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    pParserContext = av_parser_init(pCodec->id);
    if (!pParserContext) {
        fprintf(stderr, "parser not found\n");
        exit(1);
    }

    pPacket = av_packet_alloc();
    if (!pPacket) {
        exit(1);
    }
    pFrame = av_frame_alloc();
    if (!pFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    while (!feof(file)) {
        // read raw data from the input file
        data_size = fread(inbuf, 1, INBUF_SIZE, file);
        if (!data_size) {
            break;
        }
        // use the parser to split the data into frames
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(pParserContext, pCodecCtx,
                                   &pPacket->data, &pPacket->size,
                                   data, data_size,
                                   AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data += ret;
            data_size -= ret;
            if (pPacket->size) {
                decode(pCodecCtx, pFrame, pPacket, outfilename);
            }
        }
    }
    // flush the decoder
    decode(pCodecCtx, pFrame, NULL, outfilename);
    fclose(file);
    av_parser_close(pParserContext);
    avcodec_free_context(&pCodecCtx);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    return 0;
}
