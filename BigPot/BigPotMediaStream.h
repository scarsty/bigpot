#pragma once

extern "C"
{
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavutil/avutil.h"
#include "ffmpeg/libavformat/avformat.h"
}

#include "BigPotBase.h"
#include "BigPotControl.h"
#include <algorithm>
#include <map>
#include <mutex> 

enum BigPotMediaType
{
	BPMEDIA_TYPE_VIDEO = AVMEDIA_TYPE_VIDEO,
	BPMEDIA_TYPE_AUDIO = AVMEDIA_TYPE_AUDIO,
};

class BigPotMediaStream : public BigPotBase
{
public:
	struct FrameData
	{
		int time;
		int64_t info;
		void* data;
	};

	BigPotMediaStream();
	virtual ~BigPotMediaStream();
protected:
	BigPotMediaType type;
	AVFormatContext* formatCtx;
	AVStream* stream;
	AVCodecContext *codecCtx;
	AVCodec *codec;
	AVPacket packet;
	int streamIndex = -1;
	double timePerFrame = 0, timePerPacket=0;
	int maxSize = 0;  //Ϊ0ʱ��Ԥ��һ֡, ����Ч����=1��ͬ, ����ʹ��map�͸��ӻ�����

	AVFrame *frame;
	string filename;
	mutex mutex_cpp;
	
	int ticksShown = -1;
	int timedts = 0, timepts = 0, timeShown = 0;  //��ѹʱ�䣬Ӧչʾʱ�䣬����Ѿ�չʾ��ʱ��
	int time_another = 0;
	int startTime = 0;
	int totalTime = 0;

	void* data = nullptr;	//�޻���ʱ���û�����, ����Ϊ�������Ƶ������
	uint32_t datalength = 0;
private:

	map<int, FrameData> _map;
	bool decoded = false, skip = false, ended = false;

private:
	int decodeFramePre();
	virtual FrameData convert(void * p = nullptr) 
	{
		return{0, 0, nullptr};
	}

	int dropFrameData(int key = -1);
	void setMap(int key, FrameData f);
	virtual void freeData(void* p){};
	void clearMap();	
	bool needDecode();
	virtual bool needDecode2() { return true; };
protected:
	void setDecoded(bool b);
	bool haveDecoded();
	void dropDecoded();
	bool useMap();
	FrameData getCurrentFrameData();
public:
	int openFile(const string & filename, BigPotMediaType type);
	int decodeFrame();
	int getTotalTime();

	void setSkip(bool b)
	{
		skip = b;
	}

	void resetTimeBegin()
	{
		ticksShown = -1;
	}

	int seek(int time, int direct = 1);
	void setFrameTime();
	int getTime();
	int setAnotherTime(int time);
	int skipFrame(int time);
	void getSize(int &w, int&h);
	int getTimedts()
	{
		return timedts;
	}
	int getTimeShown()
	{
		return timeShown;
	}
	bool exist(){ return streamIndex >= 0; }
	void resetTimeAxis();
};


