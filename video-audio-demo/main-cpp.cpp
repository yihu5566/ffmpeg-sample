#include <iostream>
#include <libavformat/avformat.h>

using namespace std;

int main(int argc, char **argv) {
    char in_filename[] = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren-30s.mp4";
    char in_filename_audio[] = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/dump.mp4";
    char *out_filename;
    AVOutputFormat *ofmt = NULL;
    //抽取视频数据
    //抽取音频收据
    //合并数据并输出新的mp4文件

    cout << "Hello, World!" << std::endl;
    return 0;
}
