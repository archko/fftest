#include <iostream>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

#include "SDL2/SDL.h"

//  audio sample rates map from FFMPEG to SDL (only non planar)
static std::map<int, int> AUDIO_FORMAT_MAP = {
        {AV_SAMPLE_FMT_U8,  AUDIO_U8},
        {AV_SAMPLE_FMT_S16, AUDIO_S16SYS},
        {AV_SAMPLE_FMT_S32, AUDIO_S32SYS},
        {AV_SAMPLE_FMT_FLT, AUDIO_F32SYS}
};

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio    //48000 * (32/8)

static unsigned int audioLen = 0;
static unsigned char *audioChunk = NULL;
static unsigned char *audioPos = NULL;

static void fill_audio(void *udata, Uint8 *stream, int len) {
    SDL_memset(stream, 0, len);

    if (audioLen == 0)
        return;

    len = (len > audioLen ? audioLen : len);

    SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);

    audioPos += len;
    audioLen -= len;
}

static int playAudio() {
    const char *filename = "/Users/archko/Movie/健身气功八段锦-clip.mp4";

    // 1  输入流
    AVFormatContext *pFormatCtx = NULL;
    const AVCodec *pCodec = NULL;
    int ret;

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }
    //寻找视频流与音频流 ffmpeg
    int audioStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStream == -1) {
        printf("can not find audio stream!");
        return -1;
    }
    int videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream == -1) {
        printf("can not open a video stream");
        return -1;
    }

    //寻找解码器 ffmpeg
    AVCodecParameters *pCodecParameters = pFormatCtx->streams[audioStream]->codecpar;
    pCodec = avcodec_find_decoder(pFormatCtx->streams[audioStream]->codecpar->codec_id);
    if (pCodec == nullptr) {
        printf("can not find a codec");
        return -1;
    }
    //加载解码器参数 ffmpeg
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) != 0) {
        printf("can not copy codec context");
        return -1;
    }

    //启动解码器 ffmpeg
    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        printf("can not open the decoder!");
        return -1;
    }

    if (ret != 0) {
        av_log(NULL, AV_LOG_ERROR, "swr_alloc_set_opts2 fail.\n");
        return false;
    }

    // 重采样contex
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;   //声音格式  SDL仅支持部分音频格式
    int out_sample_rate = /*48000; */ pCodecCtx->sample_rate;  //采样率
    int out_channels =    /*1;  */    pCodecCtx->ch_layout.nb_channels;     //通道数
    int out_nb_samples = /*1024;  */  pCodecCtx->frame_size;
    int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);   //输出buff
    unsigned char *outBuff = (unsigned char *) av_malloc(MAX_AUDIO_FRAME_SIZE * out_channels);
    AVChannelLayout out_chn_layout;
    out_chn_layout.nb_channels = 2;
    struct SwrContext *au_convert_ctx = swr_alloc(); //初始化重采样结构体
    au_convert_ctx = swr_alloc(); //初始化重采样结构体
    swr_alloc_set_opts2(&au_convert_ctx,
                        &out_chn_layout,                    /*out*/
                        out_sample_fmt,                    /*out*/
                        out_sample_rate,                   /*out*/
                        &pCodecCtx->ch_layout,           /*in*/
                        pCodecCtx->sample_fmt,               /*in*/
                        pCodecCtx->sample_rate,              /*in*/
                        0,
                        NULL);

    swr_init(au_convert_ctx);


    ///   SDL 
    if (SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log("init audio subsysytem failed.");
        return 0;
    }

    SDL_AudioSpec wantSpec;
    wantSpec.freq = out_sample_rate;
    // 和SwrContext的音频重采样参数保持一致
    wantSpec.format = AUDIO_FORMAT_MAP[out_sample_fmt];
    wantSpec.channels = out_channels;
    wantSpec.silence = 0;
    wantSpec.samples = out_nb_samples;
    wantSpec.callback = fill_audio;
    wantSpec.userdata = pCodecCtx;

    if (SDL_OpenAudio(&wantSpec, NULL) < 0) {
        printf("can not open SDL!\n");
        return -1;
    }

    SDL_PauseAudio(0);

    //3 解码
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    while (1) {
        if ((ret = av_read_frame(pFormatCtx, &packet)) < 0)
            break;

        if (packet.stream_index == audioStream) {
            ret = avcodec_send_packet(pCodecCtx, &packet); // 送一帧到解码器

            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, frame);  // 从解码器取得解码后的数据

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                    goto end;
                }

                ret = swr_convert(au_convert_ctx, &outBuff, MAX_AUDIO_FRAME_SIZE, (const uint8_t **) frame->data,
                                  frame->nb_samples);
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while converting\n");
                    goto end;
                }

                //static FILE *outFile = fopen("xxx.pcm", "wb");
                //fwrite(outBuff, 1, out_buffer_size, outFile);

                while (audioLen > 0) {
                    SDL_Delay(1);
                }

                audioChunk = (unsigned char *) outBuff;
                audioPos = audioChunk;
                audioLen = out_buffer_size;

                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }

    // 音频不需要 flush ? 

    end:
    SDL_Quit();

    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_frame_free(&frame);

    swr_free(&au_convert_ctx);
}
