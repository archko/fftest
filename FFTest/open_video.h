#ifndef FFTEST_OPEN_VIDEO_H
#define FFTEST_OPEN_VIDEO_H
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

/**
 * 升级ffmpeg 6.0.1
 */
static int open(const char *filepath, int *videoIndex, AVFormatContext **pFormatCtx, AVCodecContext **pCodecCtx) {
    int i;
    const AVCodec *pCodec;

    if (avformat_open_input(pFormatCtx, filepath, NULL, NULL) < 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(*pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    *videoIndex = -1;
    AVFormatContext *formatCtx = *pFormatCtx;
    for (i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            *videoIndex = i;
            break;
        }
    }
    if (*videoIndex == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    pCodec = avcodec_find_decoder(formatCtx->streams[*videoIndex]->codecpar->codec_id);
    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return -1;
    }
    *pCodecCtx = avcodec_alloc_context3(pCodec); //pCodec不传会用默认参数
    avcodec_parameters_to_context(*pCodecCtx, formatCtx->streams[*videoIndex]->codecpar);

    if (avcodec_open2(*pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    return 0;
}

#endif