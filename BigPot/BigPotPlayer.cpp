#include "BigPotPlayer.h"


BigPotPlayer::BigPotPlayer()
{
	UI = new BigPotUI;
	init();
}


BigPotPlayer::~BigPotPlayer()
{
	delete UI;
	//delete media;
}

int BigPotPlayer::init()
{
	return 0;
}

int BigPotPlayer::playFile(const string &filename)
{
	this->filename = filename;
	media = new BigPotMedia;
	media->openFile(filename);

	control->getWindowSize(w, h);
	media->videoStream->getSize(w, h);

	control->setWindowSize(w, h);
	control->createMainTexture(w, h);
	control->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);
	auto s = BigPotConv::cp936toutf8(filename);
	control->setWindowTitle(s);

	this->eventLoop();
	return 0;
}

int BigPotPlayer::eventLoop()
{
	BP_Event e; 
	bool loop = true, pause = false;
	int drawUI = 128;
	int finished, i=0, x, y;
	int t = 5000;
	int v = 4;

	int totalTime = media->getTotalTime();
	printf("Total time is %1.3fs or %dmin%ds\n", totalTime / 1000.0, totalTime / 60000, totalTime % 60000 / 1000);

	int maxDelay = 0;
	while (loop && control->pollEvent(e) >= 0)
	{
		media->decodeFrame();
		control->getMouseState(x, y);
		if (drawUI > 0) 
			drawUI--;
		if (h - y < 50 || (w - x) < 200 && y < 150)
			drawUI = 128;
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
			drawUI = 128;
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
			drawUI = 128;
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
			drawUI = 128;
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
			}
			drawUI = 128;
			break;
		}
		case BP_QUIT:
			loop = false;
			break;
		case BP_WINDOWEVENT:
			if (e.window.event == BP_WINDOWEVENT_RESIZED)
			{
				w = e.window.data1;
				h = e.window.data2;
			}
			else if (e.window.event == BP_WINDOWEVENT_LEAVE)
			{
				drawUI = 0;
			}
			break;
		default:
			break;
		}
		e.type = BP_FIRSTEVENT;
		//media->audioStream->setAnotherTime(media->getVideoTime());
		//if (!media->showVideoFrame(i*100))
		int audioTime = media->getTime();  //ע������Ϊ��Ƶʱ�䣬����Ƶ������ʹ����Ƶʱ������2΢��
		if (!pause&&!media->videoStream->showTexture(audioTime))
		{
			UI->drawUI(drawUI, audioTime, totalTime, media->audioStream->changeVolume(0));
			control->renderPresent();

			//���¾���Ϊ����ʾ��Ϣ������ȫ��ȥ��
			int videoTime = (media->videoStream->getTimedts());
			int delay = -videoTime + audioTime;
			maxDelay = max(maxDelay, abs(delay));
			if (i % 1000 == 0)
			{
				maxDelay = 0;
			}
			printf("\rvolume %d, audio %4.3f, video %4.3f, diff %d / %d\t",
			media->audioStream->changeVolume(0), audioTime / 1e3, videoTime / 1e3, delay, i);	
		}
		i++;
		control->delay(1);
	}
	return 0;
}

int BigPotPlayer::createScreenTexture()
{
	/*if (tex)
		SDL_DestroyTexture(tex);
	int w, h;
	SDL_GetWindowSize(win, &w, &h);
	//tex = SDL_CreateTexture(ren,);*/
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
