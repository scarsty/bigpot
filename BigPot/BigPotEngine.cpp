﻿#include "BigPotEngine.h"

BigPotEngine BigPotEngine::_engine;

BigPotEngine::BigPotEngine()
{
	_this = &_engine;
}


BigPotEngine::~BigPotEngine()
{
	//destroy();
}

void BigPotEngine::renderCopy(BP_Texture* t, int x, int y, int w, int h, int inPresent)
{
	if (inPresent == 1)
	{
		x += _rect.x;
		y += _rect.y;
	}
	SDL_Rect r = { x, y, w, h };
	SDL_RenderCopy(_ren, t, nullptr, &r);
}

int BigPotEngine::openAudio(int& freq, int& channels, int& size, int minsize, AudioCallback f)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	
	printf("\naudio freq/channels: stream %d/%d, ", freq, channels);
	if (channels <= 2) channels = 2;
	want.freq = freq;
	want.format = BP_AUDIO_DEVICE_FORMAT;
	want.channels = channels;
	want.samples = size;
	want.callback = mixAudioCallback;
	//want.userdata = this;
	want.silence = 0;

	_callback = f;
	//if (useMap())
	{
		want.samples = max(size, minsize);
	}

	_device = 0;
	while (_device == 0)
	{
		_device = SDL_OpenAudioDevice(NULL, 0, &want, &_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
		want.channels--;
	}
	printf("device %d/%d\n", _spec.freq, _spec.channels);

	if (_device)
	{
		SDL_PauseAudioDevice(_device, 0);
	}
	else
	{
		printf("failed to open audio: %s\n", SDL_GetError());
	}

	freq = _spec.freq;
	channels = _spec.channels;

	return 0;
}

void BigPotEngine::mixAudioCallback(void* userdata, Uint8* stream, int len)
{
	SDL_memset(stream, 0, len);
	if (_engine._callback)
	{
		_engine._callback(stream, len);
	}
}

BP_Texture* BigPotEngine::createSquareTexture(int size)
{
	int d = size;
	auto square_s = SDL_CreateRGBSurface(0, d, d, 32, RMASK, GMASK, BMASK, AMASK);
	SDL_FillRect(square_s, nullptr, 0xffffffff);
	/*SDL_Rect r = { 0, 0, 1, 1 };
	auto &x = r.x;
	auto &y = r.y;
	for (x = 0; x < d; x++)
	for (y = 0; y < d; y++)
	{
	if ((x - d / 2)*(x - d / 2) + (y - d / 2)*(y - d / 2) < (d / 2) * (d / 2))
	{
	SDL_FillRect(ball_s, &r, 0xffffffff);
	}
	}
	*/
	_square = SDL_CreateTextureFromSurface(_ren, square_s);
	SDL_SetTextureBlendMode(_square, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(_square, 128);
	SDL_FreeSurface(square_s);
	return _square;
}

//注意：当字符串为空时，也会返回一个空字符串  
vector<string> BigPotEngine::splitString(const string& s, const string& delim)
{
	vector<string> ret;
	size_t last = 0;
	size_t index = s.find_first_of(delim, last);
	while (index != std::string::npos)
	{
		ret.push_back(s.substr(last, index - last));
		last = index + 1;
		index = s.find_first_of(delim, last);
	}
	if (index - last > 0)
	{
		ret.push_back(s.substr(last, index - last));
	}
	return ret;
}

void BigPotEngine::drawSubtitle(const string &fontname, const string &text, int size, int x, int y, uint8_t alpha, int align)
{
	if (alpha == 0)
		return;
	auto font = TTF_OpenFont(fontname.c_str(), size);
	if (!font) return;
	SDL_Color color = { 255, 255, 255, 255 };
	SDL_Color colorb = { 0, 0, 0, 255 };
	auto ret = splitString(text, "\n");
	for (int i = 0; i < ret.size(); i++){
		if (ret[i] == "")
			continue;
		TTF_SetFontOutline(font, 2);
		auto text_sb = TTF_RenderUTF8_Blended(font, ret[i].c_str(), colorb);
		TTF_SetFontOutline(font, 0);
		auto text_s = TTF_RenderUTF8_Blended(font, ret[i].c_str(), color);
		//SDL_SetTextureAlphaMod(text_t, alpha);
		SDL_Rect rectb = { 2, 2, 0, 0 };
		SDL_BlitSurface(text_s, NULL, text_sb, &rectb);

		auto text_t = SDL_CreateTextureFromSurface(_ren, text_sb);

		SDL_FreeSurface(text_s);
		SDL_FreeSurface(text_sb);

		SDL_Rect rect;
		SDL_QueryTexture(text_t, nullptr, nullptr, &rect.w, &rect.h);
		rect.y = y + i*(size + 2);

		switch (align)
		{
		case BP_ALIGN_LEFT:
			rect.x = x;
			break;
		case BP_ALIGN_RIGHT:
			rect.x = x - rect.w;
			break;
		case BP_ALIGN_MIDDLE:
			rect.x = x - rect.w / 2;
			break;
		}

		SDL_RenderCopy(_ren, text_t, nullptr, &rect);
		SDL_DestroyTexture(text_t);
	}
	TTF_CloseFont(font);
}

BP_Texture* BigPotEngine::createTextTexture(const string &fontname, const string &text, int size)
{
	auto font = TTF_OpenFont(fontname.c_str(), size);
	if (!font) return nullptr;
	SDL_Color c = { 255, 255, 255, 128 };
	auto text_s = TTF_RenderUTF8_Blended(font, text.c_str(), c);
	auto text_t = SDL_CreateTextureFromSurface(_ren, text_s);
	SDL_FreeSurface(text_s);
	TTF_CloseFont(font);
	return text_t;
}

void BigPotEngine::drawText(const string &fontname, const string &text, int size, int x, int y, uint8_t alpha, int align)
{
	if (alpha == 0)
		return;
	auto text_t = createTextTexture(fontname, text, size);
	if (!text_t) return;
	SDL_SetTextureAlphaMod(text_t, alpha);
	SDL_Rect rect;
	SDL_QueryTexture(text_t, nullptr, nullptr, &rect.w, &rect.h);
	rect.y = y;
	switch (align)
	{
	case BP_ALIGN_LEFT:
		rect.x = x;
		break;
	case BP_ALIGN_RIGHT:
		rect.x = x - rect.w;
		break;
	case BP_ALIGN_MIDDLE:
		rect.x = x - rect.w / 2;
		break;
	}
	SDL_RenderCopy(_ren, text_t, nullptr, &rect);
	SDL_DestroyTexture(text_t);
}

int BigPotEngine::init()
{
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		return -1;
	}
	_win = SDL_CreateWindow("BigPotPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            _start_w, _start_h, SDL_WINDOW_RESIZABLE);
    SDL_ShowWindow(_win);
    SDL_RaiseWindow(_win);
	_ren = SDL_CreateRenderer(_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE/*| SDL_RENDERER_PRESENTVSYNC*/);
    
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	
	_rect = { 0, 0, _start_w, _start_h };
	_logo = loadImage("logo.png");
	showLogo();
	renderPresent();
	TTF_Init();
    
    SDL_Rect r;
    SDL_GetDisplayBounds(0, &r);
    _max_w = r.w;
    _max_h = r.h;
    
    printf("maximium width and height are: %d, %d\n", _max_w, _max_h);
    
	return 0;
}

void BigPotEngine::toggleFullscreen()
{
	_full_screen = !_full_screen;
	if (_full_screen)
		SDL_SetWindowFullscreen(_win, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		SDL_SetWindowFullscreen(_win, 0);
	SDL_RenderClear(_ren);	
}

bool BigPotEngine::setKeepRatio(bool b)
{
	return _keep_ratio = b;
}

void BigPotEngine::setPresentPosition()
{
	if (!_tex)
		return;
	int w_dst = 0, h_dst = 0;
	int w_src = 0, h_src = 0;
	getWindowSize(w_dst, h_dst);
	SDL_QueryTexture(_tex, nullptr, nullptr, &w_src, &h_src);
	if (_keep_ratio)
	{
		if (w_src == 0 || h_src == 0) return;
		double w_ratio = 1.0*w_dst / w_src;
		double h_ratio = 1.0*h_dst / h_src;
		double ratio = min(w_ratio, h_ratio);
		if (w_ratio > h_ratio)
		{
			//宽度大，左右留空
			_rect.x = (w_dst - w_src * ratio) / 2;
			_rect.y = 0;
			_rect.w = w_src * ratio;
			_rect.h = h_dst;
		}
		else
		{
			//高度大，上下留空
			_rect.x = 0;
			_rect.y = (h_dst - h_src * ratio) / 2;
			_rect.w = w_dst;
			_rect.h = h_src * ratio;
		}
	}
	else
	{
		_rect.x = 0;
		_rect.y = 0;
		_rect.w = w_dst;
		_rect.h = h_dst;
	}
}

BP_Texture* BigPotEngine::transBitmapToTexture(const uint8_t* src, uint32_t color, int w, int h, int stride)
{
	auto s = SDL_CreateRGBSurface(0, w, h, 32, 0xff000000, 0xff0000, 0xff00, 0xff);
	SDL_FillRect(s, nullptr, color);
	auto p = (uint8_t*)s->pixels;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			p[4 * (y*w + x)] = src[y*stride + x];
		}
	}
	auto t = SDL_CreateTextureFromSurface(_ren, s);
	SDL_FreeSurface(s);
	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(t, 192);
	return t;
}

int BigPotEngine::showMessage(const string &content)
{
    const SDL_MessageBoxButtonData buttons[] =
    {
        { /* .flags, .buttonid, .text */        0, 0, "no" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "yes" },
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "cancel" },
    };
    const SDL_MessageBoxColorScheme colorScheme =
    {
        { /* .colors (.r, .g, .b) */
            /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
            { 255,   0,   0 },
            /* [SDL_MESSAGEBOX_COLOR_TEXT] */
            {   0, 255,   0 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
            { 255, 255,   0 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
            {   0,   0, 255 },
            /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
            { 255,   0, 255 }
        }
    };
    const SDL_MessageBoxData messageboxdata =
    {
        SDL_MESSAGEBOX_INFORMATION, /* .flags */
        NULL, /* .window */
        "BigPot Player", /* .title */
        content.c_str(), /* .message */
        SDL_arraysize(buttons), /* .numbuttons */
        buttons, /* .buttons */
        &colorScheme /* .colorScheme */
    };
    int buttonid;
    SDL_ShowMessageBox(&messageboxdata, &buttonid);
    return buttonid;
}


