#pragma once

#include "BigPotMediaStream.h"
#include "BigPotResample.h"

class BigPotAudioStream : public BigPotMediaStream
{
public:
	BigPotAudioStream();
	virtual ~BigPotAudioStream();
private:
	const int _scream_size = 0x400000, _convert_size = 192000;
	int _volume;
	int _scream_length = 0;
	//uint32_t _readed = 0;
	uint8_t* _resample_buffer = nullptr;
    int64_t _data_read = 0, _data_write = 0;  //��ȡ��д���ֽ�����ʵ��λ���ɸ�ֵ��ߴ����������
	int _freq, _channels;

	void mixAudioData(uint8_t* stream, int len);
	int closeAudioDevice();
	virtual FrameData convert(void* p = nullptr);
	virtual void freeData(void* buffer);
	virtual bool needDecode2();
public:
	void openAudioDevice();
	void resetDecodeState();
	int setVolume(int v);
	int changeVolume(int v);
	int getVolume() { return _volume; };
	virtual void setPause(bool pause);
};