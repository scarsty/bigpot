#include "BigPotMedia.h"

BigPotMedia::BigPotMedia()
{
	videoStream = new BigPotVideoStream();
	audioStream = new BigPotAudioStream();
}


BigPotMedia::~BigPotMedia()
{
	delete videoStream;
	delete audioStream;
}

int BigPotMedia::openFile(const string &filename)
{
	videoStream->openFile(filename, BPMEDIA_TYPE_VIDEO);
	audioStream->openFile(filename, BPMEDIA_TYPE_AUDIO);

	if (audioStream->exist())
	{
		totalTime = audioStream->getTotalTime();
		audioStream->openAudioDevice();
	}
	else
		totalTime = videoStream->getTotalTime();
	return 0;
}


int BigPotMedia::decodeFrame()
{
	videoStream->decodeFrame();
	audioStream->decodeFrame();
	//seek֮����Ƶ���������Ҫ׷����Ƶ
	if (seeking)
	{
		seeking = false;
		if (videoStream->exist() && audioStream->exist())
		{
			//��ĳһ���ӳ��������������
			videoStream->skipFrame(audioStream->getTimedts());
			audioStream->skipFrame(videoStream->getTimedts());
		}
		else
		{
			//����ǿ����ʾһ֡��Ƶ�ظ���Ƶ��ʱ���ᣬ����Ƶ�ļ�������Ҫ
			if (videoStream->exist())
				videoStream->showTexture(INT32_MAX);
		}
		//printf("\naudio %4.3f, video %4.3f\t\t", audioStream->timed / 1e3, videoStream->timed / 1e3);
	}
	return 0;
}

int BigPotMedia::getAudioTime()
{
	//printf("\t\t\t\t\t\t\r%d,%d,%d", audioStream->time, videoStream->time, audioStream->getAudioTime());
	return audioStream->getTime();
}

int BigPotMedia::seekTime(int time, int direct /*= 1*/)
{
	time = min(time, totalTime-100);
	videoStream->seek(time, direct);
	audioStream->resetDecodeState();
	audioStream->seek(time, direct);
	seeking = true;
	return 0;
}

int BigPotMedia::showVideoFrame(int time)
{
	return videoStream->showTexture(time);
}

int BigPotMedia::seekPos(double pos)
{
	//printf("\nseek %f pos, %f s\n", pos, pos * totalTime / 1e3);
	return seekTime(pos * totalTime);
}

int BigPotMedia::getVideoTime()
{
	return videoStream->getTime();
}

int BigPotMedia::getTime()
{
	if (audioStream->exist())
		return audioStream->getTime();
	else
		return videoStream->getTime();
}

