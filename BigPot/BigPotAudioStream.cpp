#include "BigPotAudioStream.h"

BigPotAudioStream::BigPotAudioStream()
{
	_volume = engine_->getMaxVolume() / 2;
	//Ԥ�������
	//����֪����Ƶ������������Ӧ��Ϊ0��һ������¶�������Ϊ0
	maxSize_ = 100;
	//��������С4M����
	if (useMap()) 
		data_ = av_mallocz(_scream_size);
	_resample_buffer =  (decltype(_resample_buffer))av_mallocz(_convert_size);
}


BigPotAudioStream::~BigPotAudioStream()
{
	if (useMap())
		av_free(data_);
	if (_resample_buffer)
		av_free(_resample_buffer);
	engine_->setAudioCallback(nullptr);
	closeAudioDevice();
}

void BigPotAudioStream::openAudioDevice()
{
	if (stream_index_ < 0)
		return;
	_freq = codecCtx_->sample_rate;
	_channels = codecCtx_->channels;
	engine_->openAudio(_freq, _channels, codecCtx_->frame_size,
		2048, bind(&BigPotAudioStream::mixAudioData, this, placeholders::_1, placeholders::_2));
}

int BigPotAudioStream::closeAudioDevice()
{
	engine_->closeAudio();
	return 0;
}

void BigPotAudioStream::mixAudioData(Uint8* stream, int len)
{
	if (!useMap())
	{
		engine_->mixAudio(stream, (uint8_t*)_resample_buffer, len, _volume);
		dropDecoded();
		return;
	}

	if (_data_write <= _data_read)
		return;
	//SDL_LockMutex(t->mutex_cpp);
	auto data1 = (uint8_t*)data_;
	int pos = _data_read % _scream_size;
	int rest = _scream_size - pos;
	//һ�λ������Σ���֤��������С�㹻
	if (len <= rest)
	{
		engine_->mixAudio(stream, data1 + pos, len, _volume);
	}
	else
	{
		engine_->mixAudio(stream, data1 + pos, rest, _volume);
		engine_->mixAudio(stream + rest, data1, len - rest, _volume);
	}
	//auto readp = data1 + pos;
	//int i = t->_map.size();
	while (haveDecoded())
	{
		auto f = getCurrentFrameData();
		if (!f.data || f.time < 0)
		{
			dropDecoded();
			break;
		}
		if (_data_read >= f.info)
		{
			//printf("drop %I64d\n", t->dataRead - f.info);
			dropDecoded();
			if (_data_read == f.info)
			{
				time_shown_ = f.time;
				ticks_shown_ = engine_->getTicks();
				break;
			}
			else
			{
				//��ȡ��һ��
				auto f1 = getCurrentFrameData();
				if (f1.info > _data_read)
				{
					time_shown_ = f.time + (f1.time - f.time) * (_data_read - f.info) / (f1.info - f.info);
					ticks_shown_ = engine_->getTicks();
				}
			}
		}
		else
		{
			break;
		}
		
	}
	_data_read += len;
	//SDL_UnlockMutex(t->mutex_cpp);
}

BigPotMediaStream::FrameData BigPotAudioStream::convert(void* p /*= nullptr*/)
{
	data_length_ = BigPotResample::convert(codecCtx_, frame_, 
		BP_AUDIO_RESAMPLE_FORMAT, _freq, _channels, _resample_buffer);
	if (useMap())
	{
		//����д��λ��
		//printf("%I64d,%I64d, %d\n", dataWrite, dataRead, _map.size());
		int pos = _data_write % _scream_size;
		int rest = _scream_size - pos;
		//����һ��д�룬����������д�룬�����Ǹ��������������ǻ�����������Ч��Ҳ��������
		if (data_length_ <= rest)
		{
			memcpy((uint8_t*)data_ + pos, _resample_buffer, data_length_);
		}
		else
		{
			memcpy((uint8_t*)data_ + pos, _resample_buffer, rest);
			memcpy((uint8_t*)data_, _resample_buffer, data_length_ - rest);
		}
		FrameData f = { time_dts_, _data_write, data_ };
		_data_write += data_length_;
		//���ص���ָ��λ��
		return f;
	}
	else
	{
		memcpy(data_, _resample_buffer, data_length_);
		return{ time_dts_, data_length_, data_ };
	}
}

void BigPotAudioStream::freeData(void* p)
{
	//av_free(p);
}

int BigPotAudioStream::setVolume(int v)
{
	v = max(v, 0);
	v = min(v, engine_->getMaxVolume());
	//printf("\rvolume is %d\t\t\t\t\t", v);
	return _volume = v;
}

int BigPotAudioStream::changeVolume(int v)
{
	if (v == 0)
		return _volume;
	return setVolume(_volume + v);
}

bool BigPotAudioStream::needDecode2()
{
	//return true;
	return _data_write - _data_read < _scream_size / 2;
}

void BigPotAudioStream::resetDecodeState()
{
	_data_write = _data_read = 0;
	//memset(data, 0, screamSize);
}

bool BigPotAudioStream::setPause(bool pause)
{
	engine_->pauseAudio(pause);
	return pause;
}

