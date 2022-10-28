#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/bprint.h"
#include "libavutil/pixdesc.h"

int main() {
    int ret = 0;
    int err;
    char *filename = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren-30s.mp4";
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        printf("error code %d \n", AVERROR(ENOMEM));
        return ENOMEM;
    }
    if ((err = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("can not open file %d \n", err);
        return err;
    }
    //打开音频解码器
    AVCodecContext *avctx = avcodec_alloc_context3(NULL);
    ret = avcodec_parameters_to_context(avctx, fmt_ctx->streams[1]->codecpar);
    if (ret < 0) {
        printf(" error code %d \n", ret);
    }
    const AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        printf("open codec fail \n");
        return ret;
    }
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext *mainsrc_ctx, *resultsink_ctx = NULL;

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *result_frame = av_frame_alloc();
    //音频格式相关
    const char *origin_sample_fmt = NULL;
    const char *origin_sample_rate = NULL;
    const char *origin_channel_layout = NULL;
    const char *result_sample_fmt = NULL;
    const char *result_sample_rate = NULL;
    const char *result_channel_layout = NULL;
    int read_end = 0;
    int frame_num = 0;
    for (;;) {
        if (1 == read_end) {
            break;
        }
        ret = av_read_frame(fmt_ctx, pkt);
        if (0 == pkt->stream_index) {
            av_packet_unref(pkt);
            continue;
        }
        if (AVERROR_EOF == ret) {
            avcodec_send_packet(avctx, NULL);
        } else {
            if (0 != ret) {
                printf("read error code %d \n", ret);
                return ENOMEM;
            } else {
                retry:
                if (avcodec_send_packet(avctx, pkt) == AVERROR(EAGAIN)) {
                    printf("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    //这里可以考虑休眠 0.1 秒，返回 EAGAIN 通常是 ffmpeg 的内部 api 有bug
                    goto retry;
                }
                av_packet_unref(pkt);
            }
        }

        //循环读取数据
        for (;;) {
            //读取 AVFrame
            ret = avcodec_receive_frame(avctx, frame);
            /* 释放 frame 里面的YUV数据，
             * 由于 avcodec_receive_frame 函数里面会调用 av_frame_unref，所以下面的代码可以注释。
             * 所以我们不需要 手动 unref 这个 AVFrame
             * */
            //av_frame_unref(frame);

            if (AVERROR(EAGAIN) == ret) {
                //提示 EAGAIN 代表 解码器 需要 更多的 AVPacket
                //跳出 第一层 for，让 解码器拿到更多的 AVPacket
                break;
            } else if (AVERROR_EOF == ret) {
                /* 提示 AVERROR_EOF 代表之前已经往 解码器发送了一个 data 跟 size 都是 NULL 的 AVPacket
                 * 发送 NULL 的 AVPacket 是提示解码器把所有的缓存帧全都刷出来。
                 * 通常只有在 读完输入文件才会发送 NULL 的 AVPacket，或者需要用现有的解码器解码另一个的视频流才会这么干。
                 * */

                //跳出 第二层 for，文件已经解码完毕。
                read_end = 1;
                break;
            } else if (ret >= 0) {
                //这两个变量在本文里没有用的，只是要传进去。
                AVFilterInOut *inputs, *outputs;
                origin_sample_fmt = av_get_sample_fmt_name(frame->format);
                printf("origin frame is %d,%s | %d | %d ,pts:%d, nb_samples:%d, duration:%d \n",
                       frame->format, origin_sample_fmt, frame->sample_rate, (int) frame->channel_layout,
                       (int) frame->pts, (int) frame->pkt_duration);

                if (NULL == filter_graph) {
                    //初始化滤镜
                    filter_graph = avfilter_graph_alloc();
                    if (!filter_graph) {
                        printf("Error: allocate filter graph failed\n");
                        return -1;
                    }

                    // 因为 filter 的输入是 AVFrame ，所以 filter 的时间基就是 AVFrame 的时间基
                    AVRational tb = fmt_ctx->streams[0]->time_base;

                    AVBPrint args;
                    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
                    av_bprintf(&args,
                               "abuffer=sample_rate=%d:sample_fmt=%s:channel_layout=%d:time_base=%d/%d[main];"
                               "[main]aformat=sample_rates=%d:sample_fmts=%s:channel_layouts=%d[result];"
                               "[result]abuffersink",
                               frame->sample_rate, origin_sample_fmt, (int) frame->channel_layout, tb.num, tb.den,
                               44100, "s64", AV_CH_FRONT_RIGHT);

                    //解析滤镜字符串。
                    ret = avfilter_graph_parse2(filter_graph, args.str, &inputs, &outputs);
                    if (ret < 0) {
                        printf("Cannot configure graph\n");
                        return ret;
                    }

                    //正式打开滤镜
                    ret = avfilter_graph_config(filter_graph, NULL);
                    if (ret < 0) {
                        printf("Cannot configure graph\n");
                        return ret;
                    }

                    //根据 名字 找到 AVFilterContext
                    mainsrc_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_abuffer_0");
                    resultsink_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_abuffersink_2");
                }

                ret = av_buffersrc_add_frame_flags(mainsrc_ctx, frame, AV_BUFFERSRC_FLAG_PUSH);
                if (ret < 0) {
                    printf("Error: av_buffersrc_add_frame failed\n");
                    return ret;
                }

                ret = av_buffersink_get_frame_flags(resultsink_ctx, result_frame, AV_BUFFERSRC_FLAG_PUSH);
                if (ret >= 0) {
                    result_sample_fmt = av_get_sample_fmt_name(result_frame->format);
                    printf("result frame is %d,%s | %d | %d ,pts:%d, nb_samples:%d , duration:%d \n",
                           result_frame->format, result_sample_fmt, result_frame->sample_rate,
                           (int) result_frame->channel_layout,
                           (int) result_frame->pts, result_frame->nb_samples, (int) result_frame->pkt_duration);
                }

                frame_num++;
                //只处理 10 帧音频
                if (frame_num > 10) {
                    return 666;
                }

            } else {
                printf("other fail \n");
                return ret;
            }
        }
    }

    av_frame_free(&frame);
    av_frame_free(&result_frame);
    av_packet_free(&pkt);

    avcodec_close(avctx);
    avformat_free_context(fmt_ctx);
    printf("done \n");

//释放滤镜
    avfilter_graph_free(&filter_graph);
    return 0;
}



