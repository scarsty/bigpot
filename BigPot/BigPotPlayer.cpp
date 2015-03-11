#include "BigPotPlayer.h"


BigPotPlayer::BigPotPlayer()
{
	_UI = new BigPotUI;
	_config = new BigPotConfig;
	_subtitle = new BigPotSubtitle;
	_w = 320;
	_h = 150;
}

BigPotPlayer::~BigPotPlayer()
{
	delete _UI;
	delete _config;
	delete _subtitle;
	//delete media;
}

int BigPotPlayer::beginWithFile(const string &filename)
{
	if (engine_->init()) return -1;
	
	_config->init();
	_sys_encode = _config->getString("sys_encode", "cp936");
	_cur_volume = _config->getInteger("volume", BP_AUDIO_MIX_MAXVOLUME / 2);
	_UI->init();

	//�״�������ק���ļ�Ҳ��Ϊ��ͬһ��
	_drop_filename = filename;
	auto play_filename = filename;
	_run = true;
	bool first = true;

	_subtitle->init();

	while (_run)
	{
		_media = nullptr;
		_media = new BigPotMedia;

		//����ǿ���̨����ͨ�������������ansi
		//����Ǵ��ڳ���ͨ�������������utf-8
		//����ͨ����ק����Ķ���utf-8
		//������Ӧ�Դ��ڳ���Ϊ��
		play_filename = _drop_filename;  //����������utf8
		
		engine_->setWindowTitle(play_filename);
		
		//���ļ�, ��Ҫ����ת��
		auto open_filename = BigPotConv::conv(play_filename, _BP_encode, _sys_encode); //�����Ҫansi
		_media->openFile(open_filename);
		_subtitle->tryOpenSubtitle(open_filename);

		//���ڳߴ磬ʱ��
		_media->getVideoStream()->getSize(_w, _h);
		engine_->setWindowSize(_w, _h);
		_media->getAudioStream()->setVolume(_cur_volume);
		//��ͼ������Ļ
		_subtitle->setFrameSize(_w, _h);

		//�״δ��ļ����ھ���		
		if (first) engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);

		//��ȡ��¼�е��ļ�ʱ�䲢��ת
		_cur_time = 0;
		_cur_time = getRecordFileTime(play_filename);
		if (_cur_time > 0) _media->seekTime(_cur_time , -1);

		//��ѭ��
		engine_->createMainTexture(_w, _h);
		this->eventLoop();
		engine_->destroyMainTexture();

		//�����ý���ļ��ͼ�¼ʱ��
		if (_media->isMedia())
			setRecordFileTime(_cur_time, play_filename);
		//�ر���Ļ
		_subtitle->closeSubtitle();

		delete _media;
		first = false;
	}
	_config->setString(_sys_encode, "sys_encode");
	_config->setInteger(_cur_volume, "volume");
	_config->write();
	_subtitle->destroy();

	engine_->destroy();

	return 0;
}

int BigPotPlayer::eventLoop()
{
	BP_Event e; 

	bool loop = true, pause = false;
	int ui_alpha = 128;
	int finished, i = 0, x, y;
	int t = 5000;
	int v = 4;
	bool havevideo = _media->getVideoStream()->exist();
	bool havemedia = _media->getAudioStream()->exist() || havevideo;
	int totalTime = _media->getTotalTime();
	string open_filename;
	printf("Total time is %1.3fs or %dmin%ds\n", totalTime / 1000.0, totalTime / 60000, totalTime % 60000 / 1000);


	int maxDelay = 0;
	while (loop && engine_->pollEvent(e) >= 0)
	{
		_media->decodeFrame();
		engine_->getMouseState(x, y);
		if (ui_alpha > 0) 
			ui_alpha--;
		if (_h - y < 50 || (_w - x) < 200 && y < 150)
			ui_alpha = 128;
		switch (e.type)
		{
		case BP_MOUSEMOTION:
			break;
		case BP_MOUSEBUTTONUP:
			if (e.button.button == BP_BUTTON_LEFT)
			{
				if (_h - e.button.y < 50)
				{
					double pos = 1.0 * e.button.x / _w;
					_media->seekPos(pos);
				}

				if (e.button.y < 50 && e.button.x > _w - 100)
				{
					pause = !pause;
					_media->getAudioStream()->setPause(pause);
				}
			}
			ui_alpha = 128;
			break;
		case BP_MOUSEWHEEL:
		{
			if (e.wheel.y > 0)
			{
				_media->getAudioStream()->changeVolume(v);
			}
			else if (e.wheel.y < 0)
			{
				_media->getAudioStream()->changeVolume(-v);
			}
			ui_alpha = 128;
			break;
		}
		case BP_KEYDOWN:
		{
			switch (e.key.keysym.sym)
			{
			case BPK_LEFT:
				_media->seekTime(_media->getTime() - t, -1);
				break;
			case BPK_RIGHT:
				_media->seekTime(_media->getTime() + t);
				break;
			case BPK_UP:
				_media->getAudioStream()->changeVolume(v);
				break;
			case BPK_DOWN:
				_media->getAudioStream()->changeVolume(-v);
				break;
			}
			ui_alpha = 128;
			break;
		}
		case BP_KEYUP:
		{
			switch (e.key.keysym.sym)
			{
			case BPK_SPACE:
				pause = !pause;
				_media->getAudioStream()->setPause(pause);
				break;
			case BPK_RETURN:
				engine_->toggleFullscreen();
				break;
			case BPK_ESCAPE:
				loop = false;
				_run = false;
				break;
			}
			ui_alpha = 128;
			break;
		}
		case BP_QUIT:
			loop = false;
			_run = false;
			break;
		case BP_WINDOWEVENT:
			if (e.window.event == BP_WINDOWEVENT_RESIZED)
			{
				//��Ҫ������ʾ����Ļ��λ��
				_w = e.window.data1;
				_h = e.window.data2;
				engine_->setPresentPosition();
				int w, h;
				engine_->getPresentSize(w, h);
				_subtitle->setFrameSize(w, h);
			}
			else if (e.window.event == BP_WINDOWEVENT_LEAVE)
			{
				ui_alpha = 0;
			}
			break;
		case BP_DROPFILE:
			//���ļ������ȼ���ǲ�����Ļ��������Ļ����ý���ļ�����ʧ�ܻ��
			open_filename = BigPotConv::conv(e.drop.file, _BP_encode, _sys_encode);
			_subtitle->closeSubtitle();
			if (!_subtitle->openSubtitle(open_filename))
			{
				_drop_filename = e.drop.file;
				loop = false;
			}	
			engine_->free(e.drop.file);
			break;
		default:
			break;
		}
		e.type = BP_FIRSTEVENT;
		//media->audioStream->setAnotherTime(media->getVideoTime());
		//if (!media->showVideoFrame(i*100))
		int audioTime = _media->getTime();  //ע������Ϊ��Ƶʱ�䣬����Ƶ������ʹ����Ƶʱ��
		if (!pause)
		{
			int videostate = _media->getVideoStream()->showTexture(audioTime);
			//����֡��
			bool show = false;
			//����Ƶ��ʾ�ɹ��������о�̬��Ƶ������ֻ����Ƶ����ˢ��
			if (videostate == 0)
			{
				show = true;
				//���¾���Ϊ����ʾ��Ϣ������ȥ��
#ifdef _DEBUG
				int videoTime = (_media->getVideoStream()->getTimedts());
				int delay = -videoTime + audioTime;
				maxDelay = max(maxDelay, abs(delay));
				if (i % 1000 == 0)
				{
					maxDelay = 0;
				}
				printf("\rvolume %d, audio %4.3f, video %4.3f, diff %d / %d\t",
					_media->getAudioStream()->changeVolume(0), audioTime / 1e3, videoTime / 1e3, delay, i);
#endif
			}
			else if ((videostate == -1 || videostate == 2) && i % 50 == 0)
			{
				show = true;
				if (havevideo)
					engine_->renderCopy();
				else
					engine_->showLogo();
			}
			if (show)
			{
				if (_subtitle->exist())
					_subtitle->show(audioTime);
				_UI->drawUI(ui_alpha, audioTime, totalTime, _media->getAudioStream()->changeVolume(0));
				engine_->renderPresent();
			}
		}
		i++;
		engine_->delay(1);
		if (audioTime > totalTime)
			_media->seekTime(0);
	}
	_cur_time = _media->getTime();
	_cur_volume = _media->getAudioStream()->getVolume();
	engine_->renderClear();
	engine_->renderPresent();
	
	return 0;
}

int BigPotPlayer::drawTex2()
{
	/*SDL_SetRenderTarget(ren, tex2);

	SDL_Texture * img = IMG_LoadTexture(ren, "logo.png");	

	SDL_Rect r;
	SDL_RenderCopy(ren, img, nullptr, nullptr);
	SDL_DestroyTexture(img);

	SDL_SetRenderTarget(ren, nullptr);
	*/
	return 0;
}

int BigPotPlayer::showTex2()
{
	//SDL_RenderCopy(ren, tex2, nullptr, nullptr);
	return 0;
}

std::string BigPotPlayer::getSysString(const string& str)
{
	return "";
}

int BigPotPlayer::getRecordFileTime(const string& filename)
{
	if (filename == "")
		return 0;
	return _config->getRecord(filename.c_str());
}

int BigPotPlayer::setRecordFileTime(int time, const string& filename)
{
	if (filename == "")
		return 0;
	_config->setRecord(time, filename.c_str());
	return time;
}
