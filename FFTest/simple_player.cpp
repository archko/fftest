/**
 * 最简单的基于FFmpeg的视频播放器SU(SDL升级版)
 * Simplest FFmpeg Player (SDL Update)
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序实现了视频文件的解码和显示（支持HEVC，H.264，MPEG2等）。
 * 是最简单的FFmpeg视频解码方面的教程。
 * 通过学习本例子可以了解FFmpeg的解码流程。
 * 本版本中使用SDL消息机制刷新视频画面。
 * This software is a simplest video player based on FFmpeg.
 * Suitable for beginner of FFmpeg.
 *
 * Version:1.2
 *
 * 备注:
 * 标准版在播放视频的时候，画面显示使用延时40ms的方式。这么做有两个后果：
 * （1）SDL弹出的窗口无法移动，一直显示是忙碌状态
 * （2）画面显示并不是严格的40ms一帧，因为还没有考虑解码的时间。
 * SU（SDL Update）版在视频解码的过程中，不再使用延时40ms的方式，而是创建了
 * 一个线程，每隔40ms发送一个自定义的消息，告知主函数进行解码显示。这样做之后：
 * （1）SDL弹出的窗口可以移动了
 * （2）画面显示是严格的40ms一帧
 * Remark:
 * Standard Version use's SDL_Delay() to control video's frame rate, it has 2
 * disadvantages:
 * (1)SDL's Screen can't be moved and always "Busy".
 * (2)Frame rate can't be accurate because it doesn't consider the time consumed
 * by avcodec_decode_video2()
 * SU（SDL Update）Version solved 2 problems above. It create a thread to send SDL
 * Event every 40ms to tell the main loop to decode and show video frames.
 */

#include <stdio.h>

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
};
#else
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

/**
 * 升级ffmpeg 6.0.1
 */
static int play() {
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    const AVCodec *pCodec;
    AVFrame *pFrame;
    AVPacket *pPacket;
    struct SwsContext *pSwsContext;
    //SDL
    int ret, got_picture;
    int screen_w = 0, screen_h = 0;
    SDL_Window *pWindow;
    SDL_Renderer *pRenderer;
    //SDL_Overlay *bmp;
    SDL_Texture *pTexture;
    SDL_Rect rect;
    SDL_Event event;

    const char *filepath = "/Users/archko/Movie/健身气功八段锦-clip.mp4";
    //avformat_network_init();
    //pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
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
    ret = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    //uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    //------------SDL----------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    screen_w = pCodecCtx->width;
    screen_h = pCodecCtx->height;
    //screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);//废弃了,换SDL_CreateWindow
    pWindow = SDL_CreateWindow(
            "MEDIA PLAYER",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            pCodecCtx->width,
            pCodecCtx->height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!pWindow) {
        printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
        return -1;
    }

    pRenderer = SDL_CreateRenderer(pWindow, -1, 0);
    if (!pRenderer) {
        printf("SDL: could not create renderer - exiting\n");
        exit(1);
    }
    //bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);//废弃了,换SDL_CreateTexture
    pTexture = SDL_CreateTexture(
            pRenderer,
            SDL_PIXELFORMAT_YV12,
            //SDL_PIXELFORMAT_IYUV,//两个格式都行
            SDL_TEXTUREACCESS_STREAMING,
            pCodecCtx->width,
            pCodecCtx->height
    );
    if (!pTexture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    rect.x = 0;
    rect.y = 0;
    rect.w = screen_w;
    rect.h = screen_h;

    pPacket = av_packet_alloc();
    //(AVPacket *) av_malloc(sizeof(AVPacket));//c的写法
    pFrame = av_frame_alloc();

    printf("---------------Video Information------------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);

    // initialize SWS context for software scaling
    pSwsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                 pCodecCtx->pix_fmt,
                                 pCodecCtx->width, pCodecCtx->height,
                                 AV_PIX_FMT_YUV420P,
                                 SWS_BICUBIC,
                                 NULL, NULL, NULL);

    //Event Loop
    while (av_read_frame(pFormatCtx, pPacket) >= 0) {
        if (pPacket->stream_index == videoindex) {
            //ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            //avcodec_decode_video2分解为两个函数
            ret = avcodec_send_packet(pCodecCtx, pPacket);
            if (ret < 0) {
                printf("Decode Error.\n");
                return -1;
            }
            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                //SDL 刷新纹理,可能是多帧,所以用while,如果不循环,画面是黑的
                SDL_UpdateYUVTexture(pTexture, NULL,
                                     pFrame->data[0], pFrame->linesize[0],
                                     pFrame->data[1], pFrame->linesize[1],
                                     pFrame->data[2], pFrame->linesize[2]);
                //以下这段可以不要
                //rect.x = 0;
                //rect.y = 0;
                //rect.w = pCodecCtx->width;
                //rect.h = pCodecCtx->height;

                SDL_RenderClear(pRenderer);//SDL 清空渲染器内容
                SDL_RenderCopy(pRenderer, pTexture, NULL, &rect);//SDL 将纹理复制到渲染器
                SDL_RenderPresent(pRenderer);//SDL 渲染
                SDL_Delay(40);  //没有这句,会是快速播放,这是简单地让播放看似正常
            }
            //下面的渲染过时了
            /*if (got_picture) {
                SDL_LockYUVOverlay(bmp);
                pFrameYUV->data[0] = bmp->pixels[0];
                pFrameYUV->data[1] = bmp->pixels[2];
                pFrameYUV->data[2] = bmp->pixels[1];
                pFrameYUV->linesize[0] = bmp->pitches[0];
                pFrameYUV->linesize[1] = bmp->pitches[2];
                pFrameYUV->linesize[2] = bmp->pitches[1];
                sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0,
                          pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                SDL_UnlockYUVOverlay(bmp);

                SDL_DisplayYUVOverlay(bmp, &rect);
            }*/
        }
        av_packet_unref(pPacket);
        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                SDL_DestroyTexture(pTexture);
                SDL_DestroyRenderer(pRenderer);
                SDL_DestroyWindow(pWindow);
                SDL_Quit();
                exit(0);
            default:
                break;
        }
    }

    sws_freeContext(pSwsContext);

    //--------------
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    av_packet_free_side_data(pPacket);
    av_frame_free(&pFrame);

    SDL_DestroyTexture(pTexture);
    SDL_DestroyRenderer(pRenderer);
    SDL_DestroyWindow(pWindow);
    SDL_Quit();

    return 0;
}