#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"

int read_packet(AVFormatContext *fmt_ctx) {
    AVPacket *pkt = av_packet_alloc();
    int ret = 0;
    ret = av_read_frame(fmt_ctx, pkt);
    if (ret < 0) {
        printf("read fail \n");
        return ret;
    } else {
        printf("stream_index : %d,pts:%lld,pos:%lld \n", pkt->stream_index, pkt->pts, pkt->pos);
        printf("data : %x %x %x %x %x %x %x \n",
               pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3], pkt->data[4],
               pkt->data[5], pkt->data[6]);
    }

    av_packet_unref(pkt);
    av_packet_free(&pkt);
}

int main() {

    AVFormatContext *fmt_ctx = NULL;
    int err, ret;
    char *filename = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren-30s.mp4";
    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        printf("error code %d /n", err);
        return ENOMEM;
    }

    //读文件
    if ((err = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("error code %d \n", err);
        return err;
    }

    read_packet(fmt_ctx);
    //按时间 seek往前跳到5秒的位置
    int64_t seek_target = 5 * AV_TIME_BASE;
    int64_t seek_min = INT64_MIN;
    int64_t seek_max = INT64_MAX;
    int flags = 0;

    ret = avformat_seek_file(fmt_ctx, -1, seek_min, seek_target, seek_max, flags);

    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s:error while seeking \n", fmt_ctx->url);
    }

    read_packet(fmt_ctx);
    read_packet(fmt_ctx);
    read_packet(fmt_ctx);

    return 0;
}
