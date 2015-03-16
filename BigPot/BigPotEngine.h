#pragma once

extern "C"
{
#ifdef _MSC_VER
#include "sdl203/SDL.h"
#include "sdl203/SDL_image.h"
#include "sdl203/sdl_ttf.h"
#else
#include "sdl204/SDL.h"
#include "sdl204/SDL_image.h"
#include "sdl204/sdl_ttf.h"
#endif
}

#include <algorithm>
#include <functional>
#include <vector>
#include <string>

using namespace std;

//�����ǵײ㲿�֣���SDL�ĺ�������װ��һ��
//��������ײ㣬��Ҫ����ʵ�������ȫ�����ܣ������¶���ȫ������������
#define BP_AUDIO_DEVICE_FORMAT AUDIO_S16
#define BP_AUDIO_MIX_MAXVOLUME SDL_MIX_MAXVOLUME

typedef function<void(uint8_t*, int)> AudioCallback;
typedef SDL_Renderer BP_Renderer;
typedef SDL_Window BP_Window;
typedef SDL_Texture BP_Texture;
typedef SDL_Rect BP_Rect;

typedef enum { BP_ALIGN_LEFT, BP_ALIGN_MIDDLE, BP_ALIGN_RIGHT } BP_Align;

#define BP_WINDOWPOS_CENTERED SDL_WINDOWPOS_CENTERED

#define RMASK (0xff0000)
#define GMASK (0xff00)
#define BMASK (0xff)
#define AMASK (0xff000000)

//���������������ļ���δʹ��
typedef SDL_AudioSpec BP_AudioSpec;
//����ֱ��ʹ��SDL���¼��ṹ����������ײ�������ʵ��һ����ͬ��
typedef SDL_Event BP_Event;

class BigPotEngine
{
private:
	BigPotEngine();
	virtual ~BigPotEngine();
private:
	static BigPotEngine _control;
	BigPotEngine* _this;
public:
	static BigPotEngine* getInstance(){ return &_control; };
	//ͼ�����
private:
	BP_Window* _win = nullptr;
	BP_Renderer* _ren = nullptr;
	BP_Texture* _tex = nullptr, *_tex2 = nullptr, *_logo = nullptr;
	BP_AudioSpec _want, _spec;
	BP_Rect _rect;
	BP_Texture* testTexture(BP_Texture* tex) { return tex ? tex : this->_tex; };
	bool _full_screen = false;
	bool _keep_ratio = true;

	int _start_w = 320, _start_h = 150; //320, 150
	int _win_w, _win_h;
public:
	int init();
	__declspec(deprecated)	
		void getWindowSize(int &w, int &h) { SDL_GetWindowSize(_win, &w, &h); }
	int getWindowsWidth()
	{
		int w;
		SDL_GetWindowSize(_win, &w, nullptr);
		return w;
	}
	int getWindowsHeight()
	{
		int h;
		SDL_GetWindowSize(_win, nullptr, &h);
		return h;
	}
	void setWindowSize(int w, int h) 
	{ 
		if (w == 0 || h == 0) return;
		_win_w = w; _win_h = h;
		SDL_SetWindowSize(_win, w, h); 
		setPresentPosition();
	}
	void setWindowPosition(int x, int y)
	{
		SDL_SetWindowPosition(_win, x, y);
	}
	void setWindowTitle(const string &str){ SDL_SetWindowTitle(_win, str.c_str()); }
	BP_Renderer* getRenderer(){ return _ren; }
		
	void createMainTexture(int w, int h)
	{
		_tex = createYUVTexture(w, h); 
		//_tex2 = createRGBATexture(w, h);
		setPresentPosition();
	}

	void setPresentPosition();  //������ͼ��λ��
	__declspec(deprecated)
		void getPresentSize(int& w, int& h) { w = _rect.w; h = _rect.h; }
	int getPresentWidth() { return _rect.w; }
	int getPresentHeight() { return _rect.h; }

	void destroyMainTexture() { destroyTexture(_tex); }
	
	void destroyTexture(BP_Texture* t) { SDL_DestroyTexture(t); }
	
	BP_Texture* createYUVTexture(int w, int h)
	{
		return SDL_CreateTexture(_ren, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, w, h);
	};
	void updateYUVTexture(BP_Texture* t, uint8_t* data0, int size0, uint8_t* data1, int size1, uint8_t* data2, int size2)
	{
		SDL_UpdateYUVTexture(testTexture(t), nullptr, data0, size0, data1, size1, data2, size2);
	}

	BP_Texture* createRGBATexture(int w, int h)
	{
		return SDL_CreateTexture(_ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	};
	void updateRGBATexture(BP_Texture* t, uint8_t* buffer, int pitch)
	{
		SDL_UpdateTexture(testTexture(t), nullptr, buffer, pitch);
	}

	void renderCopy(BP_Texture* t = nullptr) { SDL_RenderCopy(_ren, testTexture(t), nullptr, &_rect); }
	void showLogo() { SDL_RenderCopy(_ren, _logo, nullptr, nullptr); }
	void renderPresent() { SDL_RenderPresent(_ren); renderClear(); };
	void renderClear() { SDL_RenderClear(_ren); }
	void setTextureAlphaMod(BP_Texture* t, uint8_t alpha) { SDL_SetTextureAlphaMod(t, alpha); };

	void createWindow();
	void createRenderer();
	void renderCopy(BP_Texture* t, int x, int y, int w = 0, int h = 0, int inPresent = 0);
	void destroy()
	{
		SDL_DestroyTexture(_tex);
		SDL_DestroyRenderer(_ren);
		SDL_DestroyWindow(_win);
	}
	void toggleFullscreen();
	BP_Texture* loadImage(const string& filename)
	{
		return IMG_LoadTexture(_ren, filename.c_str());
	}
	bool setKeepRatio(bool b);
	BP_Texture* transBitmapToTexture(const uint8_t* src, uint32_t color, int w, int h, int stride);
	//�������
private:
	SDL_AudioDeviceID _device;
	AudioCallback _callback = nullptr;
public:
	void pauseAudio(int pause) { SDL_PauseAudioDevice(_device, pause); };
	void closeAudio(){ SDL_CloseAudioDevice(_device); };
	int getMaxVolume() { return BP_AUDIO_MIX_MAXVOLUME; };
	void mixAudio(Uint8 * dst, const Uint8 * src, Uint32 len, int volume) 
	{
		SDL_MixAudioFormat(dst, src, BP_AUDIO_DEVICE_FORMAT, len, volume);
	};

	int openAudio(int& freq, int& channels, int& size, int minsize, AudioCallback f);
	static void mixAudioCallback(void* userdata, Uint8* stream, int len);
	void setAudioCallback(AudioCallback cb = nullptr){ _callback = cb; };
	//�¼����
private:
	SDL_Event _e;
	int _time;
public:
	void delay(const int t) { SDL_Delay(t); }
	uint32_t getTicks(){ return SDL_GetTicks(); }
	uint32_t tic() { return _time = SDL_GetTicks(); }
	void toc() 	{ printf("%d\n", SDL_GetTicks() - _time); }
	void getMouseState(int &x, int& y){ SDL_GetMouseState(&x, &y); };
	int pollEvent(BP_Event& e) { return SDL_PollEvent(&e); };
	void free(void* mem){ SDL_free(mem); }
	//UI���
private:
	BP_Texture* _square;
public:
	BP_Texture* createSquareTexture();
	BP_Texture* createTextTexture(const string &fontname, const string &text, int size);
	void drawText(const string &fontname, const string &text, int size, int x, int y, uint8_t alpha, int align);
	void drawSubtitle(const string &fontname, const string &text, int size, int x, int y, uint8_t alpha, int align);
	//void split(std::string& s, std::string& delim, std::vector< std::string >* ret);
	vector<string> splitString(const string& s, const string& delim);
};

//����ֱ���հ�SDL
//�����ײ����Լ�����һ��
//������Ϲ����
typedef enum
{
	BP_FIRSTEVENT = SDL_FIRSTEVENT,
	//���رհ�ť
	BP_QUIT = SDL_QUIT,
	//window
	BP_WINDOWEVENT = SDL_WINDOWEVENT,
	BP_SYSWMEVENT = SDL_SYSWMEVENT,
	//����
	BP_KEYDOWN = SDL_KEYDOWN,
	BP_KEYUP = SDL_KEYUP,
	BP_TEXTEDITING = SDL_TEXTEDITING,
	BP_TEXTINPUT = SDL_TEXTINPUT,
	//���
	BP_MOUSEMOTION = SDL_MOUSEMOTION,
	BP_MOUSEBUTTONDOWN = SDL_MOUSEBUTTONDOWN,
	BP_MOUSEBUTTONUP = SDL_MOUSEBUTTONUP,
	BP_MOUSEWHEEL = SDL_MOUSEWHEEL,
	//������
	BP_CLIPBOARDUPDATE = SDL_CLIPBOARDUPDATE,
	//�Ϸ��ļ�
	BP_DROPFILE = SDL_DROPFILE,
	//��Ⱦ�ı�
	BP_RENDER_TARGETS_RESET = SDL_RENDER_TARGETS_RESET,

	BP_LASTEVENT = SDL_LASTEVENT
} BP_EventType;

typedef enum
{
	BP_WINDOWEVENT_NONE = SDL_WINDOWEVENT_NONE,           /**< Never used */
	BP_WINDOWEVENT_SHOWN = SDL_WINDOWEVENT_SHOWN,
	BP_WINDOWEVENT_HIDDEN = SDL_WINDOWEVENT_HIDDEN,
	BP_WINDOWEVENT_EXPOSED = SDL_WINDOWEVENT_EXPOSED,

	BP_WINDOWEVENT_MOVED = SDL_WINDOWEVENT_MOVED,

	BP_WINDOWEVENT_RESIZED = SDL_WINDOWEVENT_RESIZED,
	BP_WINDOWEVENT_SIZE_CHANGED = SDL_WINDOWEVENT_SIZE_CHANGED,
	BP_WINDOWEVENT_MINIMIZED = SDL_WINDOWEVENT_MINIMIZED,
	BP_WINDOWEVENT_MAXIMIZED = SDL_WINDOWEVENT_MAXIMIZED,
	BP_WINDOWEVENT_RESTORED = SDL_WINDOWEVENT_RESTORED,

	BP_WINDOWEVENT_ENTER = SDL_WINDOWEVENT_ENTER,
	BP_WINDOWEVENT_LEAVE = SDL_WINDOWEVENT_LEAVE,
	BP_WINDOWEVENT_FOCUS_GAINED = SDL_WINDOWEVENT_FOCUS_GAINED,
	BP_WINDOWEVENT_FOCUS_LOST = SDL_WINDOWEVENT_FOCUS_LOST,
	BP_WINDOWEVENT_CLOSE = SDL_WINDOWEVENT_CLOSE
} BP_WindowEventID;

typedef enum
{
	BPK_LEFT = SDLK_LEFT,
	BPK_RIGHT = SDLK_RIGHT,
	BPK_UP = SDLK_UP,
	BPK_DOWN = SDLK_DOWN,
	BPK_SPACE = SDLK_SPACE,
	BPK_ESCAPE = SDLK_ESCAPE,
	BPK_RETURN = SDLK_RETURN
} BP_KeyBoard;

typedef enum
{
	BP_BUTTON_LEFT= SDL_BUTTON_LEFT,
	BP_BUTTON_MIDDLE= SDL_BUTTON_MIDDLE,
	BP_BUTTON_RIGHT =SDL_BUTTON_RIGHT
} BP_Button;

#ifdef __MINGW32__
class mutex
{
private:
	SDL_mutex* _mutex;
public:
	mutex(){ _mutex = SDL_CreateMutex(); }
	~mutex(){ SDL_DestroyMutex(_mutex); }
	int lock(){ return SDL_LockMutex(_mutex); }
	int unlock(){ return SDL_UnlockMutex(_mutex); }
};
#endif
