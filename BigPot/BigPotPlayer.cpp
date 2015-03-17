#include "BigPotPlayer.h"


BigPotPlayer::BigPotPlayer()
{
	_UI = new BigPotUI;
	//_config = new BigPotConfig;
	//_subtitle = new BigPotSubtitle;
	_subtitle_factory = new BigPotSubtitleFactory;
	//config_->init();
	_w = 320;
	_h = 150;
}

BigPotPlayer::~BigPotPlayer()
{
	delete _UI;
	//delete _config;
	delete _subtitle_factory;
	//delete _subtitle;
	//delete media;
}

int BigPotPlayer::beginWithFile(const string &filename)
{
	if (init() != 0) return -1;


	//�״�������ק���ļ�Ҳ��Ϊ��ͬһ��
	_drop_filename = filename;
	auto play_filename = filename;
	_run = true;
	bool first = true;

	//_subtitle->init();

	while (_run)
	{
		openMedia(play_filename);
		//�״δ��ļ����ھ���
		if (first) engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);

		this->eventLoop();

		closeMedia(play_filename);
		play_filename = _drop_filename;

		first = false;
	}
	destroy();
	return 0;
}

int BigPotPlayer::eventLoop()
{
	BP_Event e; 

	bool loop = true, pause = false, seeking = false;
	int ui_alpha = 128;
	int finished, i = 0, x, y;
	int seek_step = 5000;
	int volume_step = 4;
	bool havevideo = _media->getVideoStream()->exist();
	bool havemedia = _media->getAudioStream()->exist() || havevideo;
	int totalTime = _media->getTotalTime();
	string open_filename;
	printf("Total time is %1.3fs or %dmin%ds\n", totalTime / 1000.0, totalTime / 60000, totalTime % 60000 / 1000);

	int maxDelay = 0; //ͳ��ʹ��
	int prev_show_time = 0;  //��һ����ʾ��ʱ��

	while (loop && engine_->pollEvent(e) >= 0)
	{
		seeking = false;

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
					seeking = true;
				}

				if (e.button.y < 50 && e.button.x > _w - 100)
				{
					pause = !pause;
					_media->setPause(pause);
				}
			}
			ui_alpha = 128;
			break;
		case BP_MOUSEWHEEL:
		{
			if (e.wheel.y > 0)
			{
				_media->getAudioStream()->changeVolume(volume_step);
			}
			else if (e.wheel.y < 0)
			{
				_media->getAudioStream()->changeVolume(-volume_step);
			}
			ui_alpha = 128;
			break;
		}
		case BP_KEYDOWN:
		{
			switch (e.key.keysym.sym)
			{
			case BPK_LEFT:
				_media->seekTime(_media->getTime() - seek_step, -1);
				seeking = true;
				break;
			case BPK_RIGHT:
				_media->seekTime(_media->getTime() + seek_step);
				seeking = true;
				break;
			case BPK_UP:
				_media->getAudioStream()->changeVolume(volume_step);
				break;
			case BPK_DOWN:
				_media->getAudioStream()->changeVolume(-volume_step);
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
				_media->setPause(pause);
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
				_subtitle->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());
			}
			else if (e.window.event == BP_WINDOWEVENT_LEAVE)
			{
				ui_alpha = 0;
			}
			break;
		case BP_DROPFILE:
			//���ļ������ȼ���ǲ�����Ļ��������Ļ����ý���ļ�����ʧ�ܻ��
			//����ý���ļ�������Ļ�򿪻�ǳ�������������Ļ�ļ�����չ��
			open_filename = BigPotConv::conv(e.drop.file, _BP_encode, _sys_encode);
			//����ǲ�����Ļ����������
			if (_subtitle_factory->isSubtitle(open_filename))
			{
				_subtitle_factory->destroySubtitle(_subtitle);
				_subtitle = _subtitle_factory->createSubtitle(open_filename);
				_subtitle->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());
			}
			else
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
		//if (!loop) break;

		//��ÿ��ѭ��������Ԥ��ѹ
		_media->decodeFrame();

		//��������ƵΪ��׼��ʾ��Ƶ
		int audioTime = _media->getTime();  //ע������Ϊ��Ƶʱ�䣬����Ƶ������ʹ����Ƶʱ��
		int time_s = audioTime;
		if (pause)
		{
			time_s = 0; //pauseʱ��ˢ����Ƶʱ���ᣬ������������ʾ��ֹͼ������
		}
		int videostate = _media->getVideoStream()
			->showTexture(time_s); 

		//���ݽ���Ƶ�Ľ���ж��Ƿ���ʾ
		bool show = false;
		//����Ƶ��ʾ�ɹ��������о�̬��Ƶ������ֻ����Ƶ����ˢ��
		if (videostate == 0)
		{
			show = true;
			//���¾���Ϊ����ʾ��Ϣ������ȥ��
#ifdef _DEBUG
			int videoTime = (_media->getVideoStream()->getTimedts());
			int delay = -videoTime + audioTime;
			printf("\rvolume %d, audio %4.3f, video %4.3f, diff %d / %d\t",
				_media->getAudioStream()->changeVolume(0), audioTime / 1e3, videoTime / 1e3, delay, i);
#endif
		}
		//��ֹʱ������Ƶʱ����Ƶ�ѷ���ʱ40������ʾһ��
		//����Ƶδ��ͣ��δ��ʱ�䲻�������ж�
		else if ((pause || videostate == -1 || videostate ==2)
			&& engine_->getTicks() - prev_show_time > 40)
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
			prev_show_time = engine_->getTicks();
		}
		i++;
		engine_->delay(1);
		if (audioTime >= totalTime)
			_media->seekTime(0);
	}
	engine_->renderClear();
	engine_->renderPresent();
	
	return 0;
}

int BigPotPlayer::init()
{
	if (engine_->init()) return -1;

	config_->init(_filepath);
	_sys_encode = config_->getString("sys_encode", "cp936");
	_cur_volume = config_->getInteger("volume", BP_AUDIO_MIX_MAXVOLUME / 2);
	_UI->init();
	return 0;
}

void BigPotPlayer::destroy()
{
	config_->setString(_sys_encode, "sys_encode");
	config_->setInteger(_cur_volume, "volume");
	config_->write();
	_UI->destory();
	engine_->destroy();
}


//����Ϊutf8����
void BigPotPlayer::openMedia(const string& filename)
{
	_media = nullptr;
	_media = new BigPotMedia;

	//����ǿ���̨����ͨ�������������ansi
	//����Ǵ��ڳ���ͨ�������������utf-8
	//����ͨ����ק����Ķ���utf-8
	//������Ӧ�Դ��ڳ���Ϊ��

	engine_->setWindowTitle(filename);

	//���ļ�, ��Ҫ����ת��
	auto open_filename = BigPotConv::conv(filename, _BP_encode, _sys_encode); //�����Ҫansi
	_media->openFile(open_filename);

	//���ڳߴ磬ʱ��
	_w = _media->getVideoStream()->getWidth();
	_h = _media->getVideoStream()->getHeight();
	engine_->setWindowSize(_w, _h);
	engine_->createMainTexture(_w, _h);
	//���»�ȡ�ߴ磬�п�����֮ǰ��ͬ
	_w = engine_->getWindowsWidth();
	_h = engine_->getWindowsHeight();	
	
	//����
	_media->getAudioStream()->setVolume(_cur_volume);
	
	//��ͼ������Ļ
	auto open_subfilename = _subtitle_factory->lookForSubtitle(open_filename);
	_subtitle = _subtitle_factory->createSubtitle(open_subfilename);
	_subtitle->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());

	//��ȡ��¼�е��ļ�ʱ�䲢��ת
	if (_media->isMedia())
	{
		_cur_time = 0;
		_cur_time = config_->getRecord(filename.c_str());
		if (_cur_time > 0) _media->seekTime(_cur_time, -1);
	}
}

void BigPotPlayer::closeMedia(const string& filename)
{
	engine_->destroyMainTexture();

	//��¼����ʱ��
	_cur_time = _media->getTime();
	_cur_volume = _media->getAudioStream()->getVolume();

	//�ر���Ļ
	_subtitle_factory->destroySubtitle(_subtitle);
	//_subtitle->closeSubtitle();
	
	//�����ý���ļ��ͼ�¼ʱ��
	if (_media->isMedia())
		config_->setRecord(_cur_time, filename.c_str());

	delete _media;
}



