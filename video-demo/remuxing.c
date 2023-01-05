/*
 * Copyright (c) 2013 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat/libavcodec demuxing and muxing API example.
 *
 * Remux streams from one container format to another.
 * @example remuxing.c
 */
#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>


int64_t max(int64_t i, int64_t pts);

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag) {
    AVRational * time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

int main(int argc, char **argv) {
    //输出
    AVOutputFormat *ofmt = NULL;
    //下文
    AVFormatContext *in_fmt1 = NULL, *in_fmt2 = NULL, *ou_fmt = NULL;
    const char *in_filename, *in_filename_audio, *out_filename;
    int ret, i;

    int video1_in_index, audio1_in_index;
    int video2_in_index, audio2_in_index;
    int video_ou_index, audio_ou_index;
    int64_t next_video_pts, next_audio_pts;

    //视频
    in_filename = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/dump.mp4";
    //音频
    in_filename_audio = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren-30s.mp4";
    //合成
    out_filename = "/Users/dongfang/CLionProjects/ffmpeg-sample/mp4_file/juren.mp4";

    in_fmt1 = avformat_alloc_context();
    in_fmt2 = avformat_alloc_context();
    ou_fmt = avformat_alloc_context();

    //打开输入视频
    if ((ret = avformat_open_input(&in_fmt1, in_filename, NULL, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }

    if ((ret = avformat_find_stream_info(in_fmt1, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }


    for (i = 0; i < in_fmt1->nb_streams; i++) {
        AVStream *stream = in_fmt1->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video1_in_index == -1) {
            video1_in_index = i;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio1_in_index == -1) {
            audio1_in_index = i;
        }
    }


    //打开输入音频
    if ((ret = avformat_open_input(&in_fmt2, in_filename_audio, NULL, 0)) < 0) {
        fprintf(stderr, "Could not open input audio file '%s'", in_filename_audio);
        goto end;
    }
    if ((ret = avformat_find_stream_info(in_fmt2, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input audio stream information");
        goto end;
    }

    // 将源文件2音视频索引信息找出来
    for (i = 0; i < in_fmt2->nb_streams; i++) {
        AVStream *stream = in_fmt2->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video2_in_index == -1) {
            video2_in_index = i;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio2_in_index == -1) {
            audio2_in_index = i;
        }
    }

    //打开输出流
    avformat_alloc_output_context2(&ou_fmt, NULL, NULL, out_filename);
    if (!ou_fmt) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }


    ofmt = ou_fmt->oformat;

    // 为封装的文件添加视频流信息;由于假设两个文件的视频流具有相同的编码方式，这里就是简单的流拷贝
    for (i = 0; i < 1; i++) {
        if (video1_in_index != -1) {
            AVStream *stream = avformat_new_stream(ou_fmt, NULL);
            video_ou_index = stream->index;

            // 由于是流拷贝方式
            AVStream *srcstream = in_fmt1->streams[video1_in_index];
            if (avcodec_parameters_copy(stream->codecpar, srcstream->codecpar) < 0) {
                printf("avcodec_parameters_copy fail");
                break;
            }

            // codec_id和code_tag共同决定了一种编码方式在容器里面的码流格式，所以如果源文件与目的文件的码流格式不一致，那么就需要将目的文件
            // 的code_tag 设置为0，当调用avformat_write_header()函数后会自动将两者保持一致
            unsigned int src_tag = srcstream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmt->oformat->codec_tag, src_tag) != stream->codecpar->codec_tag) {
                stream->codecpar->codec_tag = 0;
            }

            break;
        }
        if (video2_in_index != -1) { // 只要任何一个文件有视频流都创建视频流
            AVStream *stream = avformat_new_stream(ou_fmt, NULL);
            video_ou_index = stream->index;

            // 由于是流拷贝方式
            AVStream *srcstream = in_fmt2->streams[video2_in_index];
            if (avcodec_parameters_copy(stream->codecpar, srcstream->codecpar) < 0) {
                printf("avcodec_parameters_copy 2 fail");
                break;
            }

            unsigned int src_tag = srcstream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmt->oformat->codec_tag, src_tag) != stream->codecpar->codec_tag) {
                stream->codecpar->codec_tag = 0;
            }

        }
    }

    // 为封装的文件添加流信息;由于假设两个文件的视频流具有相同的编码方式，这里就是简单的流拷贝
    for (i = 0; i < 1; i++) {

        if (audio1_in_index != -1) {
            AVStream *dststream = avformat_new_stream(ou_fmt, NULL);
            printf("读取次数 %d   ---%d\n  ", in_fmt1->nb_streams, audio1_in_index);
            AVStream *srcstream = in_fmt1->streams[audio1_in_index];

            if (avcodec_parameters_copy(dststream->codecpar, srcstream->codecpar) < 0) {
                printf("avcodec_parameters_copy 1 fial\n");
                break;
            }
            printf("读取次数 %d   ---%d\n  ", in_fmt1->nb_streams, in_fmt2->nb_streams);
            audio_ou_index = dststream->index;

            break;
        }

        if (audio2_in_index != -1) {
            AVStream *dststream = avformat_new_stream(ou_fmt, NULL);
            AVStream *srcstream = in_fmt2->streams[audio1_in_index];
            if (avcodec_parameters_copy(dststream->codecpar, srcstream->codecpar) < 0) {
                printf("avcodec_parameters_copy 1 fial");
                break;
            }
            audio_ou_index = dststream->index;
        }
    }


    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ou_fmt->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ou_fmt, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    AVPacket *in_pkt1 = av_packet_alloc();
    while (av_read_frame(in_fmt1, in_pkt1) >= 0) {

        // 视频流
        if (in_pkt1->stream_index == video1_in_index) {

            AVStream *srcstream = in_fmt1->streams[in_pkt1->stream_index];
            AVStream *dststream = ou_fmt->streams[video_ou_index];
            // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
            next_video_pts = max(in_pkt1->pts + in_pkt1->duration, next_video_pts);
            av_packet_rescale_ts(in_pkt1, srcstream->time_base, dststream->time_base);
            // 更正目标流的索引
            in_pkt1->stream_index = video_ou_index;

            printf("pkt1 video %d(%6s) du %d(%s)", in_pkt1->pts, av_ts2timestr(in_pkt1->pts, &dststream->time_base),
                   in_pkt1->duration, av_ts2timestr(in_pkt1->duration, &dststream->time_base));
        }

        // 音频流
        if (in_pkt1->stream_index == audio1_in_index) {
            AVStream *srcstream = in_fmt1->streams[in_pkt1->stream_index];
            AVStream *dststream = ou_fmt->streams[audio_ou_index];
            next_audio_pts = max(in_pkt1->pts + in_pkt1->duration, next_audio_pts);
            // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
            av_packet_rescale_ts(in_pkt1, srcstream->time_base, dststream->time_base);
            // 更正目标流的索引
            in_pkt1->stream_index = audio_ou_index;
            printf("pkt1 audio %d(%6s) du%d(%s)", in_pkt1->pts, av_ts2timestr(in_pkt1->pts, &dststream->time_base),
                   in_pkt1->duration, av_ts2timestr(in_pkt1->duration, &dststream->time_base));
        }

        // 向封装器中写入数据
        if ((ret = av_write_frame(ou_fmt, in_pkt1)) < 0) {
            printf("av_write_frame fail1 %s", av_err2str(ret));
            break;
        }
    }

    // 进行流拷贝；源文件1
    while (1) {
        AVPacket *in_pkt2 = av_packet_alloc();
        if (av_read_frame(in_fmt2, in_pkt2) < 0) break;
        /** 遇到问题：写入数据是提示"[mp4 @ 0x10100ba00] Application provided invalid, non monotonically increasing dts to muxer in stream 1: 4046848 >= 0"
         *  分析原因：因为是两个源文件进行合并，对于每一个源文件来说，它的第一个AVPacket的pts都是0开始的
         *  解决方案：所以第二个源文件的pts,dts,duration就应该加上前面源文件的duration最大值
         */
        // 视频流
        if (in_pkt2->stream_index == video2_in_index) {

            AVStream *srcstream = in_fmt2->streams[in_pkt2->stream_index];
            AVStream *dststream = ou_fmt->streams[video_ou_index];
            if (next_video_pts > 0) {
                AVStream *srcstream2 = in_fmt1->streams[video1_in_index];
                in_pkt2->pts = av_rescale_q(in_pkt2->pts, srcstream->time_base, srcstream2->time_base) + next_video_pts;
                in_pkt2->dts = av_rescale_q(in_pkt2->dts, srcstream->time_base, srcstream2->time_base) + next_video_pts;
                in_pkt2->duration = av_rescale_q(in_pkt2->duration, srcstream->time_base, srcstream2->time_base);
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream2->time_base, dststream->time_base);
            } else {
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream->time_base, dststream->time_base);
            }
            printf("pkt2 video %lld(%s)", in_pkt2->pts, av_ts2timestr(in_pkt2->pts, &dststream->time_base));
            // 更正目标流的索引
            in_pkt2->stream_index = video_ou_index;
        }

        // 音频流
        if (in_pkt2->stream_index == audio2_in_index) {
            if (in_pkt2->pts == AV_NOPTS_VALUE) {
                in_pkt2->pts = in_pkt2->dts;
            }
            if (in_pkt2->dts == AV_NOPTS_VALUE) {
                in_pkt2->dts = in_pkt2->pts;
            }
            AVStream *srcstream = in_fmt2->streams[in_pkt2->stream_index];
            AVStream *dststream = ou_fmt->streams[audio_ou_index];
            if (next_audio_pts > 0) {
                AVStream *srcstream2 = in_fmt1->streams[audio1_in_index];
                in_pkt2->pts = av_rescale_q(in_pkt2->pts, srcstream->time_base, srcstream2->time_base) + next_audio_pts;
                in_pkt2->dts = av_rescale_q(in_pkt2->dts, srcstream->time_base, srcstream2->time_base) + next_audio_pts;
                in_pkt2->duration = av_rescale_q(in_pkt2->duration, srcstream->time_base, srcstream2->time_base);
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream2->time_base, dststream->time_base);
            } else {
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream->time_base, dststream->time_base);
            }
            printf("pkt2 audio %d(%s)", in_pkt2->pts, av_ts2timestr(in_pkt2->pts, &dststream->time_base));
            // 更正目标流的索引
            in_pkt2->stream_index = audio_ou_index;
        }

        // 向封装器中写入数据
        if (av_write_frame(ou_fmt, in_pkt2) < 0) {
            printf("av_write_frame 2 fail");
            break;
        }

        av_packet_unref(in_pkt2);
    }


    av_write_trailer(ou_fmt);
    end:

    avformat_close_input(&in_fmt1);
    avformat_close_input(&in_fmt2);

    /* close output */
    if (ou_fmt && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ou_fmt->pb);
    avformat_free_context(ou_fmt);


    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}

int64_t max(int64_t i, int64_t pts) {
    return i > pts;
}
