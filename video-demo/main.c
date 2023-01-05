#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
/**
 * 需求：一段视频，一段音频，将原视频中的音频替换为外接音频
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    const char *in_filename, *out_filename;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

//    if (argc < 3) {
//        printf("usage: %s input output\n"
//               "API example program to remux a media file with libavformat and libavcodec.\n"
//               "The output format is guessed according to the file extension.\n"
//               "\n", argv[0]);
//        return 1;
//    }


    return 0;
}
