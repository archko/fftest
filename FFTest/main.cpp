//
//  main.cpp
//  FFTest
//
//  Created by archko on 2023/9/14.
//

#if defined(__cplusplus)
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#if defined(__cplusplus)
}
#endif

#include <iostream>
#include "test_copy.h"
#include "simple_player.cpp"

using namespace std;

int main(int argc, const char *argv[]) {
    std::cout << "Hello, World!\n";
    //打印FFmpeg 的配置信息（编译配置项）
    //cout << "avcodec_configuration : " << avcodec_configuration() << endl;

    //test_copy();
    play();

    return 0;
}
