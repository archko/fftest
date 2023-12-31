////
////  simplest_ffmpeg_picture_encoder.cpp
////  FFTest
////
////  Created by archko on 2023/9/14.
////
//
//#include <stdio.h>
//
//#define __STDC_CONSTANT_MACROS
//
//#if defined(__cplusplus)
//extern "C"
//{
//#endif
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#if defined(__cplusplus)
//}
//#endif
//
//int main(int argc, char* argv[])
//{
//    AVFormatContext *fmt_ctx = NULL;
//    AVIOContext *avio_ctx = NULL;
//    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
//    size_t buffer_size, avio_ctx_buffer_size = 4096;
//    char *input_filename = NULL;
//    int ret = 0;
//    struct buffer_data bd = { 0 };
//    if (argc != 2) {
//        fprintf(stderr, "usage: %s input_file\n"
//                "API example program to show how to read from a custom buffer "
//                "accessed through AVIOContext.\n", argv[0]);
//        return 1;
//    }
//    
//    input_filename = argv[1];
//    /* slurp file content into buffer */
//    ret = av_file_map(input_filename, &buffer, &buffer_size, 0, NULL);
//    if (ret < 0)
//        goto end;
//    /* fill opaque structure used by the AVIOContext read callback */
//    bd.ptr  = buffer;
//    bd.size = buffer_size;
//    if (!(fmt_ctx = avformat_alloc_context())) {
//        ret = AVERROR(ENOMEM);
//        goto end;
//    }
//    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
//    if (!avio_ctx_buffer) {
//        ret = AVERROR(ENOMEM);
//        goto end;
//    }
//    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size,
//                                  0, &bd, &read_packet, NULL, NULL);
//    if (!avio_ctx) {
//        ret = AVERROR(ENOMEM);
//        goto end;
//    }
//    
//    fmt_ctx->pb = avio_ctx;
//    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
//    if (ret < 0) {
//        fprintf(stderr, "Could not open input\n");
//        goto end;
//    }
//    ret = avformat_find_stream_info(fmt_ctx, NULL);
//    if (ret < 0) {
//        fprintf(stderr, "Could not find stream information\n");
//        goto end;
//    }
//    av_dump_format(fmt_ctx, 0, input_filename, 0);
//end:
//    avformat_close_input(&fmt_ctx);
//    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
//    if (avio_ctx) {
//        av_freep(&avio_ctx->buffer);
//        av_freep(&avio_ctx);
//    }
//    av_file_unmap(buffer, buffer_size);
//    if (ret < 0) {
//        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
//        return 1;
//    }
//    return 0;
//}
