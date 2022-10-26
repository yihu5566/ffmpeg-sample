#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"

int main() {

    AVFormatContext *fmt_ctx = NULL;
    char *filename = "/Users/dongfang/CLionProjects/ffmpeg_demo/juren-30s.mp4";
    char *filename1 = "/Users/dongfang/CLionProjects/ffmpeg_demo/juren-30s.flv";
    int err;
    int type = 5;
    int ret = 0;
    fmt_ctx = avformat_alloc_context();
    if (!fmt_ctx) {
//        printf("error code %d \n", AVERROR(ENOMEM));
        av_log(NULL, AV_LOG_ERROR, "error code %d \n", AVERROR(ENOMEM));
        return ENOMEM;
    }
    if ((err = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        printf("can not open file %d \n", err);
        return err;
    }
    ///编码相关
    if(type==5){

    }

    ///解码相关
    if (type == 4) {
        AVCodecContext *avctx = avcodec_alloc_context3(NULL);
        ret = avcodec_parameters_to_context(avctx, fmt_ctx->streams[0]->codecpar);
        if (ret < 0) {
            printf("error code %d \n", ret);
            return ret;
        }
        const AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
        if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
            printf("open codec faile %d \n", ret);
            return ret;
        }
        AVPacket *pkt = av_packet_alloc();
        AVFrame *frame = av_frame_alloc();
        int frame_num = 0;
        int read_end = 0;

        for (;;) {
            if (1 == read_end) {
                break;
            }
            ret = av_read_frame(fmt_ctx, pkt);
            //跳过不处理音频包
            if (1 == pkt->stream_index) {
                av_packet_unref(pkt);
                continue;
            }

            if (AVERROR_EOF == ret) {
                //读取完文件，这时候 pkt 的 data 跟 size 应该是 null
                avcodec_send_packet(avctx, pkt);
                //省略处理读取错误的情况

                return ret;
            } else if (ret == 0) {
                //视频帧信息
                retry:
                if (avcodec_send_packet(avctx, pkt) == AVERROR(EAGAIN)) {
                    printf("Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    //这里可以考虑休眠 0.1 秒，返回 EAGAIN 通常是 ffmpeg 的内部 api 有bug
                    goto retry;
                } else {
                    //释放pkt里面的编码数据
                    av_packet_unref(pkt);
                    //循环不断从解码器读数据，直到没有数据可读。
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
                            //解码出一帧 YUV 数据，打印一些信息。

                            printf("decode success ----\n");
                            printf("width : %d , height : %d \n", frame->width, frame->height);
                            printf("pts : %lld , duration : %lld \n", frame->pts, frame->duration);
                            printf("format : %d \n", frame->format);
                            printf("key_frame : %d \n", frame->key_frame);
                            printf("AVPictureType : %d \n", frame->pict_type);
                            int num = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, 1920, 1080, 1);
                            printf("num : %d , \n", num);
                            //保存首帧为图片
                            //打印 yuv
                            printf(" Y size : %d \n", frame->linesize[0]);
                            printf(" U size : %d \n", frame->linesize[1]);
                            printf(" V size : %d \n", frame->linesize[2]);

                            //修改 yuv
                            for (int tt = 0; tt < 5; tt++) {
                                frame->data[0][tt] = 0xFF;
                            }
                            //拼接文件名
                            char yuv_pic_name[200] = "./yuv420p_";
                            char frame_num_str[10];
//                            itoa(frame_num, frame_num_str, 10);
                            sprintf(frame_num_str,"%d",frame_num);
                            strcat(yuv_pic_name, frame_num_str);
                            //写入文件
                            FILE *fp = NULL;
                            fp = fopen(yuv_pic_name, "w+");
                            fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, fp);
                            fwrite(frame->data[1], 1, frame->linesize[1] * frame->height / 2, fp);
                            fwrite(frame->data[2], 1, frame->linesize[2] * frame->height / 2, fp);
                            fclose(fp);

                            frame_num++;
                            if (frame_num > 10) {
                                return 99;
                            }


                        } else {
                            printf("other fail \n");
                            return ret;
                        }
                    }
                }

            }

        }

        av_frame_free(&frame);
        av_packet_free(&pkt);

        //关闭解码器。
        avcodec_close(avctx);
    }

//    读取av—packet
    if (type == 1) {
        if ((err = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
            printf("error code %d \n", err);

        } else {
//            读取视频文件参数
//            printf("open success \n");
//            printf("filename - %s \n", fmt_ctx->filename);
//            printf("duration - %I64d \n", fmt_ctx->duration);
//            printf("nb_streams - %u \n", fmt_ctx->nb_streams);
//            for (int i = 0; i < fmt_ctx->nb_streams; i++) {
//                printf("stream codec_type - %d \n", fmt_ctx->streams[i]->codecpar->codec_type);
//            }
//            printf("iformat name - %s \n", fmt_ctx->iformat->name);
//            printf("iformat long name - %s \n", fmt_ctx->iformat->long_name);

            AVPacket *pkt = av_packet_alloc();
            int ret = 0;
            ret = av_read_frame(fmt_ctx, pkt);
            if (ret < 0) {
                printf("read fail \n");
            } else {
                printf("open success \n");
                for (int i = 0; i < fmt_ctx->nb_streams; i++) {
                    // 0视频流 1音频流
                    printf("stream codec_type - %d \n", fmt_ctx->streams[i]->codecpar->codec_type);
                }
                printf("stream_index - %d \n", pkt->stream_index);
                printf("duration - %lld ,time_base : %d%d\n", pkt->duration, fmt_ctx->streams[1]->time_base.num,
                       fmt_ctx->streams[0]->time_base.den);
                printf("size - %d \n", pkt->size);
                printf("pos  - %lld \n", pkt->pos);
                printf("data:  %x,%x,%x,%x,%x,%x,%x \n", pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3],
                       pkt->data[4], pkt->data[5],
                       pkt->data[6]);
            }
            av_packet_free(&pkt);

        }
    }

    if (2 == type) {
        //设置探测大小
        AVDictionary *format_opts = NULL;
        av_dict_set(&format_opts, "probesize", "32", 0);

        if ((err = avformat_open_input(&fmt_ctx, filename1, NULL, &format_opts)) < 0) {
            printf("error code %d \n", err);
        } else {
            avformat_find_stream_info(fmt_ctx, NULL);
            printf("open success \n");
            printf("filename - %s \n", fmt_ctx->url);
            printf("duration - %lld \n", fmt_ctx->duration);
            printf("nb_streams - %u \n", fmt_ctx->nb_streams);
            for (int i = 0; i < fmt_ctx->nb_streams; i++) {
                printf("stream codec_type - %d \n", fmt_ctx->streams[i]->codecpar->codec_type);
            }
            printf("iformat name - %s \n", fmt_ctx->iformat->name);
            printf("iformat long name - %s \n", fmt_ctx->iformat->long_name);
        }
        av_dict_free(&format_opts);
    }

    if (type == 3) {

        AVDictionary *format_opts = NULL;
        AVDictionaryEntry *t;
        av_dict_set(&format_opts, "formatprobesize", "10485760", AV_DICT_MATCH_CASE);
        av_dict_set(&format_opts, "export_all", "1", AV_DICT_MATCH_CASE);
        av_dict_set(&format_opts, "export_666", "1", AV_DICT_MATCH_CASE);

        //获取字典里的第一个属性
        if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
            av_log(NULL, AV_LOG_INFO, "option key %s,value %s \n", t->key, t->value);
        }
        if ((err = avformat_open_input(&fmt_ctx, filename, NULL, &format_opts)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "error code %d \n", AVERROR(ENOMEM));
        } else {
            av_log(NULL, AV_LOG_ERROR, "open success  \n");
            av_log(NULL, AV_LOG_ERROR, "open duration: %lld \n", fmt_ctx->duration);
        }

        if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
            av_log(NULL, AV_LOG_INFO, "option key %s,value %s \n", t->key, t->value);
        }
    }
    printf("Hello, World!\n");
    return 0;
}
