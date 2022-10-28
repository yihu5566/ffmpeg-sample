#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/bprint.h"

int save_yuv_to_file(AVFrame *frame, int num);

AVFrame *get_frame_from_jpeg_or_png_file(const char *filename, AVRational *logo_tb, AVRational *logo_fr);

int main() {
    int ret = 0;
    int err;
    char *filename = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren-30s.mp4";
    char *filename_logo = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/time.jpeg";
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
        printf("error code %d \n", AVERROR(ENOMEM));
        return ENOMEM;
    }
    if ((err = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("can not open file %d \n", err);
        return err;
    }
    AVCodecContext *avctx = avcodec_alloc_context3(NULL);
    ret = avcodec_parameters_to_context(avctx, fmt_ctx->streams[0]->codecpar);
    if (ret < 0) {
        printf(" error code %d \n", ret);
    }
    const AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        printf("open codec fail \n");
        return ret;
    }
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext *mainsrc_ctx, *resultsink_ctx, *logo_ctx, *scale_ctx, *overlay_ctx = NULL;

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *result_frame = av_frame_alloc();


    AVRational logo_tb = {0};
    AVRational logo_fr = {0};

    AVFrame *logo_frame = get_frame_from_jpeg_or_png_file(filename_logo, &logo_tb, &logo_fr);

    if (logo_frame == NULL) {
        printf("logo not exist \n");
        return 99;
    }
    int64_t logo_next_pts = 0, main_next_pts = 0;
    int read_end = 0;
    int frame_num = 0;
    for (;;) {
        if (1 == read_end) {
            break;
        }
        ret = av_read_frame(fmt_ctx, pkt);
        if (1 == pkt->stream_index) {
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

            //循环读取数据
            for (;;) {
                ret = avcodec_receive_frame(avctx, frame);
                if (AVERROR(EAGAIN) == ret) {
                    break;
                } else if (AVERROR_EOF == ret) {
                    read_end = 1;
                    break;
                } else if (ret >= 0) {
                    //这两个变量在本文里没有用的，只是要传进去。
                    AVFilterInOut *inputs, *outputs;
                    if (NULL == filter_graph) {
                        //初始化滤镜
                        filter_graph = avfilter_graph_alloc();
                        if (!filter_graph) {
                            printf("error:avfilter_graph_alloc fail \n");
                            return -1;
                        }
                        //因为filter的输入是AVFrame，所以filter的时间基就是AVFrame的时间基。
                        AVRational tb = fmt_ctx->streams[0]->time_base;
                        AVRational fr = av_guess_frame_rate(fmt_ctx, fmt_ctx->streams[0], NULL);
                        AVRational sar = frame->sample_aspect_ratio;
                        AVRational logo_sar = logo_frame->sample_aspect_ratio;

                        AVBPrint args;
                        av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);

                        av_bprintf(&args,
                                   "buffer=video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:frame_rate=%d/%d[main];"
                                   "buffer=video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:frame_rate=%d/%d[logo];"
                                   "[main][logo]overlay=x=10:y=10[result];"
                                   "[result]format=yuv420p[result_2];"
                                   "[result_2]buffersink",
                                   frame->width, frame->height, frame->format, tb.num, tb.den, sar.num, sar.den, fr.num,
                                   fr.den,
                                   logo_frame->width, logo_frame->height, logo_frame->format, logo_tb.num, logo_tb.den,
                                   logo_sar.num, logo_sar.den, logo_fr.num, logo_fr.den);

                        ret = avfilter_graph_parse2(filter_graph, args.str, &inputs, &outputs);
                        if (ret < 0) {
                            printf("Cannot configure graph\n");
                            return ret;
                        }
                        //正式打开
                        ret = avfilter_graph_config(filter_graph, NULL);
                        if (ret < 0) {
                            printf("Cannot configure graph \n");
                            return ret;
                        }

                        //更具名字找到滤镜
                        mainsrc_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_buffer_0");
                        logo_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_buffer_1");
                        resultsink_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_buffersink_4");
                        ret = av_buffersrc_add_frame_flags(logo_ctx, logo_frame, AV_BUFFERSRC_FLAG_PUSH);
                        if (ret < 0) {
                            printf("Error: av_buffersrc_add_frame failed 1\n");
                            return ret;
                        }
                        //因为 logo 只有一帧，发完，就需要关闭 buffer 滤镜
                        logo_next_pts = logo_frame->pts + logo_frame->pkt_duration;
                        ret = av_buffersrc_close(logo_ctx, logo_next_pts, AV_BUFFERSRC_FLAG_PUSH);
                    }

                    if (frame_num <= 10) {
                        //保存文件
                        printf("save_yuv_to_file success\n");
                        ret = av_buffersrc_add_frame_flags(mainsrc_ctx, frame, AV_BUFFERSRC_FLAG_PUSH);
                        if (ret < 0) {
                            printf("Error: av_buffersrc_add_frame failed 2\n");
                            return ret;
                        }
                    }
                    //输出
                    ret = av_buffersink_get_frame_flags(resultsink_ctx, result_frame, AV_BUFFERSINK_FLAG_NO_REQUEST);

                    if (ret == AVERROR_EOF) {
                        printf("no more avframe output \n");
                    } else if (ret == AVERROR(EAGAIN)) {
                        printf("need more avframe input \n");
                    } else if (ret >= 0) {
                        //保存进去文件。
                        printf("save_yuv_to_file success %d \n", frame_num);
                        save_yuv_to_file(result_frame, frame_num);
                    }
                    logo_next_pts = frame->pts + frame->pkt_duration;
                    frame_num++;
                    if (frame_num > 10) {
                        ret = av_buffersrc_close(mainsrc_ctx, logo_next_pts, AV_BUFFERSRC_FLAG_PUSH);
                    }
                } else {
                    printf("other fail \n");
                    return ret;
                }
            }
        }

    }

    av_frame_free(&frame);
    av_frame_free(&result_frame);
    av_packet_free(&pkt);

    avcodec_close(avctx);
    avformat_free_context(fmt_ctx);
    //释放滤镜
    avfilter_graph_free(&filter_graph);
    return 0;
}

int save_yuv_to_file(AVFrame *frame, int num) {
    //拼接文件名
    char yuv_pic_name[200] = {0};
    sprintf(yuv_pic_name, "./yuv420p_%d.yuv", num);

    //写入文件
    FILE *fp = NULL;
    fp = fopen(yuv_pic_name, "wb+");
    fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, fp);
    fwrite(frame->data[1], 1, frame->linesize[1] * frame->height / 2, fp);
    fwrite(frame->data[2], 1, frame->linesize[2] * frame->height / 2, fp);
    fclose(fp);
    return 0;
}


//图片获取帧数据
AVFrame *get_frame_from_jpeg_or_png_file(const char *filename, AVRational *logo_tb, AVRational *logo_fr) {
    int ret = 0;
    AVDictionary *format_opts = NULL;
    av_dict_set(&format_opts, "probesize", "500000", 0);
    AVFormatContext *format_ctx = NULL;
    if ((ret = avformat_open_input(&format_ctx, filename, NULL, &format_opts)) < 0) {
        printf("Error: avformat_open_input failed \n");
        return NULL;
    }
    avformat_find_stream_info(format_ctx, NULL);
    *logo_tb = format_ctx->streams[0]->time_base;
    *logo_fr = av_guess_frame_rate(format_ctx, format_ctx->streams[0], NULL);

    //打开解码器
    AVCodecContext *av_codec_ctx = avcodec_alloc_context3(NULL);
    ret = avcodec_parameters_to_context(av_codec_ctx, format_ctx->streams[0]->codecpar);
    if (ret < 0) {
        printf("error code %d \n", ret);
        avformat_close_input(&format_ctx);
        return NULL;
    }
    AVDictionary *codec_opts = NULL;
    av_dict_set(&codec_opts, "sub_text_format", "ass", 0);
    const AVCodec *codec = avcodec_find_decoder(av_codec_ctx->codec_id);
    if (!codec) {
        printf("codec not support %d \n", ret);
        return NULL;
    }

    if ((ret = avcodec_open2(av_codec_ctx, codec, NULL)) < 0) {
        printf("open codec faile %d \n", ret);
        return NULL;
    }

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    ret = av_read_frame(format_ctx, &pkt);
    ret = avcodec_send_packet(av_codec_ctx, &pkt);
    AVFrame *frame_2 = av_frame_alloc();
    avcodec_receive_frame(av_codec_ctx, frame_2);
    av_dict_free(&format_opts);
    av_dict_free(&codec_opts);
    return frame_2;
}
