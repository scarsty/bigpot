#include "PotMedia.h"
#include "Config.h"
#include "File.h"

PotMedia::PotMedia()
{
    avformat_network_init();
    stream_video_ = &blank_video_;
    stream_audio_ = &blank_audio_;
    stream_subtitle_ = &blank_subtitle_;
    format_ctx_video_ = avformat_alloc_context();
    format_ctx_audio_ = avformat_alloc_context();
    format_ctx_subtitle_ = avformat_alloc_context();
}

PotMedia::~PotMedia()
{
    for (auto st : streams_)
    {
        delete st;
    }
    avformat_close_input(&format_ctx_video_);
    avformat_close_input(&format_ctx_audio_);
    avformat_close_input(&format_ctx_subtitle_);
}

int PotMedia::openFile(const std::string& filename)
{
    if (!File::fileExist(filename))
    {
        //return -1;
    }

    auto format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, filename.c_str(), nullptr, nullptr) == 0)
    {
        avformat_find_stream_info(format_ctx, nullptr);

        avformat_open_input(&format_ctx_video_, filename.c_str(), nullptr, nullptr);
        avformat_open_input(&format_ctx_audio_, filename.c_str(), nullptr, nullptr);
        avformat_open_input(&format_ctx_subtitle_, filename.c_str(), nullptr, nullptr);

        avformat_find_stream_info(format_ctx_video_, nullptr);
        avformat_find_stream_info(format_ctx_audio_, nullptr);
        avformat_find_stream_info(format_ctx_subtitle_, nullptr);

        streams_.resize(format_ctx->nb_streams);
        for (int i = 0; i < format_ctx->nb_streams; i++)
        {
            switch (format_ctx->streams[i]->codecpar->codec_type)
            {
            case BPMEDIA_TYPE_VIDEO:
            {
                auto st = new PotStreamVideo();
                st->setFormatCtx(format_ctx_video_);
                streams_[i] = st;
                if (!stream_video_->exist())
                {
                    stream_video_ = st;
                }
                break;
            }
            case BPMEDIA_TYPE_AUDIO:
            {
                auto st = new PotStreamAudio();
                st->setFormatCtx(format_ctx_audio_);
                streams_[i] = st;
                if (!stream_audio_->exist())
                {
                    stream_audio_ = st;
                }
                break;
            }
            case BPMEDIA_TYPE_SUBTITLE:
            {
                auto st = new PotStreamSubtitle();
                st->setFormatCtx(format_ctx_subtitle_);
                streams_[i] = st;
                if (!stream_subtitle_->exist())
                {
                    stream_subtitle_ = st;
                }
                break;
            }
            }
            if (streams_[i])
            {
                streams_[i]->setStreamIndex(i);
                streams_[i]->openFile(filename);
            }
            //stream_index_vector_.push_back(i);
            //if (stream_index_vector_.size() == 1)
            //{
            //    //printf("finded media stream: %d\n", type);
            //    stream_ = format_ctx_->streams[i];
            //    codec_ctx_ = stream_->codec;
            //    if (stream_->r_frame_rate.den)
            //    {
            //        time_per_frame_ = 1e3 / av_q2d(stream_->r_frame_rate);
            //    }
            //    time_base_packet_ = 1e3 * av_q2d(stream_->time_base);
            //    total_time_ = format_ctx_->duration * 1e3 / AV_TIME_BASE;
            //    start_time_ = format_ctx_->start_time * 1e3 / AV_TIME_BASE;
            //    codec_ = avcodec_find_decoder(codec_ctx_->codec_id);
            //    avcodec_open2(codec_ctx_, codec_, nullptr);
            //}
            //}
        }
    }
    avformat_close_input(&format_ctx);

    if (stream_audio_->exist())
    {
        total_time_ = stream_audio_->getTotalTime();
        stream_audio_->openAudioDevice();
    }
    else
    {
        total_time_ = stream_video_->getTotalTime();
    }
    return 0;
}

int PotMedia::decodeFrame()
{
    bool need_reset = seeking_;
    //int se= engine_->getTicks();
    stream_video_->tryDecodeFrame(need_reset);
    if (stream_video_->isStopping())
    {
        return 0;
    }
    stream_audio_->tryDecodeFrame(need_reset);
    stream_subtitle_->tryDecodeFrame(need_reset);
    //int m = _audioStream->getTimedts();
    //int n = _videoStream->getTimedts();

    //同步
    if (seeking_)
    {
        seeking_ = false;
        //seek之后，音频可能落后，需要追赶音频
        if (stream_video_->exist() && stream_audio_->exist())
        {
            //一定时间以上才跳帧
            //查看延迟情况
            int v_dts = stream_video_->getTimedts();
            int a_dts = stream_audio_->getTimedts();
            int max_dts = std::max(v_dts, a_dts);
            int min_dts = std::min(v_dts, a_dts);
            printf("seeking diff v%d-a%d=%d\n", v_dts, a_dts, v_dts - a_dts);
            //一定时间以上才跳帧
            if (max_dts - min_dts > 100)
            {
                int sv = stream_video_->skipFrame(max_dts);
                int sa = stream_audio_->skipFrame(max_dts);
                printf("drop %d audio frames, %d video frames\n", sa, sv);
                /*v_dts = _videoStream->getTimedts();
                a_dts = _audioStream->getTimedts();
                printf("seeking end diff v%d-a%d=%d\n", v_dts, a_dts, v_dts - a_dts);*/
            }
        }
        //cout << "se"<<engine_->getTicks()-se << " "<<endl;
    }
    return 0;
}

int PotMedia::getAudioTime()
{
    //printf("\t\t\t\t\t\t\r%d,%d,%d", audioStream->time, videoStream->time, audioStream->getAudioTime());
    return stream_audio_->getTime();
}

int PotMedia::seekTime(int time, int direct /*= 1*/, int reset /*= 0*/)
{
    time = std::min(time, total_time_ - 100);
    stream_video_->seek(time, direct, reset);
    stream_audio_->seek(time, direct, reset);
    stream_subtitle_->seek(time - 5000, direct, reset);

    seeking_ = true;
    stream_audio_->resetDecodeState();
    return 0;
}

int PotMedia::showVideoFrame(int time)
{
    return stream_video_->show(time);
}

int PotMedia::seekPos(double pos, int direct /*= 1*/, int reset /*= 0*/)
{
    //printf("\nseek %f pos, %f s\n", pos, pos * totalTime / 1e3);
    return seekTime(pos * total_time_, direct, reset);
}

int PotMedia::getVideoTime()
{
    return stream_video_->getTime();
}

int PotMedia::getTime()
{
    if (stream_audio_->exist())
    {
        return stream_audio_->getTime();
    }
    else
    {
        return stream_video_->getTime();
    }
}

void PotMedia::destroy()
{
}

bool PotMedia::isMedia()
{
    return stream_audio_->exist() || stream_video_->exist();
}

void PotMedia::setPause(bool pause)
{
    stream_audio_->setPause(pause);
    stream_video_->setPause(pause);
    //stream_subtitle_->setPause(pause);
}

void PotMedia::switchStream(PotMediaType mt)
{
    if (getStreamCount(mt) <= 1)
    {
        return;
    }
    int current_index = -1;
    for (int i = 0; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt
            && (streams_[i] == stream_video_ || streams_[i] == stream_audio_ || streams_[i] == stream_subtitle_))
        {
            current_index = i;
            break;
        }
    }
    if (current_index == -1)
    {
        return;
    }
    PotStream* st = streams_[current_index];
    for (int i = current_index + 1; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt)
        {
            st = streams_[i];
            break;
        }
    }
    if (st == streams_[current_index])
    {
        for (int i = 0; i < current_index; i++)
        {
            if (streams_[i] && streams_[i]->getType() == mt)
            {
                st = streams_[i];
                break;
            }
        }
    }

    switch (mt)
    {
    case BPMEDIA_TYPE_VIDEO:
    {
        stream_video_ = (PotStreamVideo*)st;
        break;
    }
    case BPMEDIA_TYPE_AUDIO:
    {
        //注意此处效果不佳
        //若假设所有音频使用同一解码器，则切换的效果会较好
        int t = getTime();
        stream_audio_->closeAudioDevice();
        //stream_audio_->clearMap();
        stream_audio_ = (PotStreamAudio*)st;
        stream_audio_->openAudioDevice();
        stream_audio_->seek(t);
        //stream_audio_->setStreamIndex(st->getStreamIndex());
        break;
    }
    case BPMEDIA_TYPE_SUBTITLE:
    {
        stream_subtitle_ = (PotStreamSubtitle*)st;
        stream_subtitle_->seek(getTime() - 5000);
        break;
    }
    }
}

int PotMedia::getStreamCount(PotMediaType mt)
{
    int count = 0;
    for (int i = 0; i < streams_.size(); i++)
    {
        if (streams_[i] && streams_[i]->getType() == mt && streams_[i]->exist())
        {
            count++;
        }
    }
    return count;
}
