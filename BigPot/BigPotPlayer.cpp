#include "BigPotPlayer.h"


BigPotPlayer::BigPotPlayer()
{
	UI = new BigPotUI;
	config = new BigPotConfig;
}


BigPotPlayer::~BigPotPlayer()
{
	delete UI;
	delete config;
	//delete media;
}

int BigPotPlayer::beginWithFile(const string &filename)
{
	if (engine->init()) return -1;
	
	config->init();
	config->getString(sys_encode, "sys_encode");
	cur_volume = BP_AUDIO_MIX_MAXVOLUME / 2;
	config->getInteger(cur_volume, "volume");
	UI->init();

	//�״�������ק���ļ�Ҳ��Ϊ��ͬһ��
	drop_filename = filename;
	auto play_filename = filename;
	run = true;
	bool first = true;

	while (run)
	{
		media = nullptr;
		media = new BigPotMedia;

		//����ǿ���̨����ͨ�������������ansi
		//����Ǵ��ڳ���ͨ�������������utf-8
		//����ͨ����ק����Ķ���utf-8
		//������Ӧ�Դ��ڳ���Ϊ��
		play_filename = drop_filename;  //����������utf8
		
		engine->setWindowTitle(play_filename);
		
		//���ļ�, ��Ҫ����ת��
		auto open_filename = BigPotConv::conv(play_filename, BP_encode, sys_encode); //�����Ҫansi
		media->openFile(open_filename);
		

		//���ڳߴ磬ʱ��
		media->videoStream->getSize(w, h);
		engine->setWindowSize(w, h);
		media->audioStream->setVolume(cur_volume);
		//�״δ��ļ����ھ���		
		if (first) engine->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);

		//��ȡ��¼�е��ļ�ʱ�䲢��ת
		cur_time = getFileTime(play_filename);
		media->seekTime(cur_time);

		//��ѭ��
		engine->createMainTexture(w, h);
		this->eventLoop();
		engine->destroyMainTexture();

		//�����ý���ļ��ͼ�¼ʱ��
		if (media->isMedia())
			setFileTime(cur_time, play_filename);

		delete media;
		first = false;
	}
	config->setString(sys_encode, "sys_encode");
	config->setInteger(cur_volume, "volume");
	config->write();
	engine->destroy();
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
	bool havevideo = media->videoStream->exist();
	bool havemedia = media->audioStream->exist() || havevideo;
	int totalTime = media->getTotalTime();
	printf("Total time is %1.3fs or %dmin%ds\n", totalTime / 1000.0, totalTime / 60000, totalTime % 60000 / 1000);

	int maxDelay = 0;
	while (loop && engine->pollEvent(e) >= 0)
	{
		media->decodeFrame();
		engine->getMouseState(x, y);
		if (ui_alpha > 0) 
			ui_alpha--;
		if (h - y < 50 || (w - x) < 200 && y < 150)
			ui_alpha = 128;
		switch (e.type)
		{
		case BP_MOUSEMOTION:
			break;
		case BP_MOUSEBUTTONUP:
			if (e.button.button == BP_BUTTON_LEFT)
			{
				if (h - e.button.y < 50)
				{
					double pos = 1.0 * e.button.x / w;
					media->seekPos(pos);
				}

				if (e.button.y < 50 && e.button.x > w - 100)
				{
					pause = !pause;
					media->audioStream->setPause(pause);
				}
			}
			ui_alpha = 128;
			break;
		case BP_MOUSEWHEEL:
		{
			if (e.wheel.y > 0)
			{
				media->audioStream->changeVolume(v);
			}
			else if (e.wheel.y < 0)
			{
				media->audioStream->changeVolume(-v);
			}
			ui_alpha = 128;
			break;
		}
		case BP_KEYDOWN:
		{
			switch (e.key.keysym.sym)
			{
			case BPK_LEFT:
				media->seekTime(media->getTime() - t, -1);
				break;
			case BPK_RIGHT:
				media->seekTime(media->getTime() + t);
				break;
			case BPK_UP:
				media->audioStream->changeVolume(v);
				break;
			case BPK_DOWN:
				media->audioStream->changeVolume(-v);
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
				media->audioStream->setPause(pause);
				break;
			case BPK_RETURN:
				engine->toggleFullscreen();
				break;
			case BPK_ESCAPE:
				loop = false;
				run = false;
				break;
			}
			ui_alpha = 128;
			break;
		}
		case BP_QUIT:
			loop = false;
			run = false;
			break;
		case BP_WINDOWEVENT:
			if (e.window.event == BP_WINDOWEVENT_RESIZED)
			{
				w = e.window.data1;
				h = e.window.data2;
			}
			else if (e.window.event == BP_WINDOWEVENT_LEAVE)
			{
				ui_alpha = 0;
			}
			break;
		case BP_DROPFILE:
			loop = false;
			drop_filename = e.drop.file;
			engine->free(e.drop.file);
		default:
			break;
		}
		e.type = BP_FIRSTEVENT;
		//media->audioStream->setAnotherTime(media->getVideoTime());
		//if (!media->showVideoFrame(i*100))
		int audioTime = media->getTime();  //ע������Ϊ��Ƶʱ�䣬����Ƶ������ʹ����Ƶʱ��
		if (havemedia && !pause)
		{
			int videostate = media->videoStream->showTexture(audioTime);
			//����֡��
			if (videostate == 0)
			{
				UI->drawUI(ui_alpha, audioTime, totalTime, media->audioStream->changeVolume(0));
				engine->renderPresent();
				//���¾���Ϊ����ʾ��Ϣ������ȥ��
#ifdef _DEBUG
				int videoTime = (media->videoStream->getTimedts());
				int delay = -videoTime + audioTime;
				maxDelay = max(maxDelay, abs(delay));
				if (i % 1000 == 0)
				{
					maxDelay = 0;
				}
				printf("\rvolume %d, audio %4.3f, video %4.3f, diff %d / %d\t",
					media->audioStream->changeVolume(0), audioTime / 1e3, videoTime / 1e3, delay, i);
#endif
			}
			else if ((videostate == -1 || videostate == 2) && i % 50 == 0)
			{
				engine->renderCopy();
				UI->drawUI(ui_alpha, audioTime, totalTime, media->audioStream->changeVolume(0));
				engine->renderPresent();
			}
		}
		i++;
		engine->delay(1);
	}
	cur_time = media->getTime();
	cur_volume = media->audioStream->getVolume();
	engine->renderClear();
	engine->renderPresent();
	
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

int BigPotPlayer::getFileTime(const string& filename)
{
	if (filename == "")
		return 0;
	int time;
	config->getRecord(time, filename.c_str());
	return time;
}

int BigPotPlayer::setFileTime(int time, const string& filename)
{
	if (filename == "")
		return 0;
	config->setRecord(time, filename.c_str());
	return time;
}
