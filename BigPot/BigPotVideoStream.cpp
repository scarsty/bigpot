#include "BigPotVideoStream.h"


BigPotVideoStream::BigPotVideoStream()
{
	//��Ƶ������, �㹻��ʱ���������������֡����˸
	maxSize = 0;
}


BigPotVideoStream::~BigPotVideoStream()
{
}

int BigPotVideoStream::showTexture(int time)
{
	if (streamIndex < 0)
		return 0;
	auto f = getCurrentFrameData();
	int time_c = f.time;
	if (haveDecoded() && time >= time_c)
	{
		auto tex = (BP_Texture*)f.data;
		engine->renderCopy(tex);
		timeShown = time_c;
		ticksShown = engine->getTicks();
		dropDecoded();
		return 0;
	}
	return -1;	
}

void BigPotVideoStream::freeData(void* p)
{
	engine->destroyTexture((BP_Texture*)p);
}

BigPotMediaStream::FrameData BigPotVideoStream::convert(void* p /*= nullptr*/)
{
	auto &f = frame;
	auto tex = (BP_Texture*)data;
	if (useMap())
	{
		tex = engine->createYUVTexture(codecCtx->width, codecCtx->height);
	}
	engine->updateYUVTexture(tex, f->data[0], f->linesize[0], f->data[1], f->linesize[1], f->data[2], f->linesize[2]);
	return{ timedts, f->linesize[0], tex };
}

int BigPotVideoStream::dropTexture()
{
	return 0;
}
