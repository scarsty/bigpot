#include "PotPlayer.h"
#include "Config.h"
#include "Font.h"
#include "PotSubtitleManager.h"
#include "convert.h"

#ifdef _WIN32
//#include <shlobj.h>
//#pragma comment(lib,"shfolder.lib")
#endif

PotPlayer::PotPlayer()
{
    //_subtitle = new BigPotSubtitle;
    //Config::getInstance()->init();
    width_ = 320;
    height_ = 150;
    handle_ = nullptr;
    run_path_ = "./";
}

PotPlayer::PotPlayer(char* s) : PotPlayer()
{
    run_path_ = File::getFilePath(s);
#if defined(_WIN32) && defined(_SINGLE_FILE)
    char szPath[MAX_PATH];
    //SHGetSpecialFolderPath(NULL, szPath, CSIDL_LOCAL_APPDATA, false);
    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath);
    //std::wstring ws(szPath);
    std::string str(szPath);
    run_path_ = str + "/bigpot";
    WIN32_FIND_DATAA wfd;
    if (FindFirstFileA(run_path_.c_str(), &wfd) == INVALID_HANDLE_VALUE)
    {
        CreateDirectoryA(run_path_.c_str(), NULL);
    }
#endif
}

PotPlayer::~PotPlayer()
{
    //delete _UI;
    //delete _config;
    //delete _subtitle;
    //delete media;
}

int PotPlayer::beginWithFile(std::string filename)
{
    int count = 0;
    if (init() != 0)
    {
        return -1;
    }
    engine_->resetRenderTarget();
    int start_time = engine_->getTicks();

    if (filename.empty() && Config::getInstance()->getInteger("auto_play_recent"))
    {
        filename = Config::getInstance()->getString("recent_file");
        if (!File::fileExist(PotConv::conv(filename, BP_encode_, sys_encode_)))
        {
            filename = "";
        }
    }
    //首次运行拖拽的文件也认为是同一个
    drop_filename_ = filename;

    printf("Begin with file: %s\n", filename.c_str());
    auto play_filename = drop_filename_;
    running_ = true;

    //_subtitle->init();

    while (running_)
    {
        /*if (count <= 1)
        {
        //_drop_filename = "";
        //play_filename = "";
        }*/

        openMedia(play_filename);
        bool add_cond = true;
        //printf("%d", engine_->getTicks() - start_time);
        add_cond = engine_->getTicks() - start_time < 2000;
#ifndef _WINDLL
        if (count == 0 && add_cond)
        {
            /*auto w = engine_->getMaxWindowWidth();
            auto h = engine_->getMaxWindowHeight();
            auto x = max(0, (w-_w)/2);
            auto y = max(0, (h-_h)/2);
            printf("%d,%d\n",x,y);
            engine_->setWindowPosition(x, y);*/
            //首次打开文件窗口居中
            engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);
        }
#endif
        this->eventLoop();
        closeMedia(play_filename);
        if (play_filename != "")
        {
            count++;
        }
        play_filename = drop_filename_;
    }
    destroy();
    return exit_type_;
}

int PotPlayer::eventLoop()
{
    BP_Event e;

    bool loop = true, pause = false, seeking = false;
    int finished, i = 0;
    const int seek_step = 5000;
    int volume_step = 1;
    bool havevideo = media_->getVideo()->exist();
    bool havemedia = media_->getAudio()->exist() || havevideo;
    int totalTime = media_->getTotalTime();
    std::string open_filename;
    printf("Total time is %1.3f s or %d min %1.3f s\n", totalTime / 1e3, totalTime / 60000, totalTime % 60000 / 1e3);

    int maxDelay = 0;          //统计使用
    int prev_show_time = 0;    //上一次显示的时间
    exit_type_ = 0;

    int sub_state = 0;    //0不显示，1外部字幕，2及以上内部字幕
    int internal_sub_count = media_->getStreamCount(BPMEDIA_TYPE_SUBTITLE);
    if (internal_sub_count > 0)
    {
        sub_state = 2;
    }
    if (subtitle_ && subtitle_->exist())
    {
        sub_state = 1;
    }
    int find_direct = 0;

    while (loop && engine_->pollEvent(e) >= 0)
    {
        seeking = false;
        find_direct++;    //连续24天后方向会出现bug，但是不管了
        int last_volume = media_->getAudio()->getVolume();

        auto switchSubtitle = [&]()
        {
            sub_state++;
            if (sub_state == 1)
            {
                if (subtitle_ == nullptr || !subtitle_->exist())
                {
                    sub_state++;
                }
            }
            else if (sub_state > 2)
            {
                media_->switchStream(BPMEDIA_TYPE_SUBTITLE);
                media_->getSubtitle()->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());
            }
            if (sub_state >= 2 + internal_sub_count)
            {
                sub_state = 0;
            }
            if (sub_state == 0)
            {
                UI_.setText("Hide subtitles");
            }
            else if (sub_state == 1)
            {
                UI_.setText("External subtitles");
            }
            else if (sub_state >= 2 && media_->getSubtitle()->getStreamIndex() >= 0)
            {
                UI_.setText(convert::formatString("Internal subtitles stream %d", media_->getSubtitle()->getStreamIndex()));
            }
        };

        switch (e.type)
        {
        case BP_MOUSEMOTION:
            break;
        case BP_MOUSEBUTTONUP:
            if (e.button.button == BP_BUTTON_LEFT)
            {
                double pos = UI_.inProcess();
                int button = UI_.inButton();
                if (pos >= 0)
                {
                    media_->seekPos(pos, 1, 1);
                    seeking = true;
                }
                if (button == PotUI::ButtonPause)
                {
                    pause = !pause;
                    media_->setPause(pause);
                }
                else if (button == PotUI::ButtonNext)
                {
                    find_direct = 1;
                    auto next_file = findNextFile(drop_filename_, find_direct);
                    if (next_file != "")
                    {
                        drop_filename_ = next_file;
                        loop = false;
                    }
                }
                else if (button == PotUI::ButtonFullScreen)
                {
                    engine_->toggleFullscreen();
                    setSubtitleFrameSize();
                }
                else if (button == PotUI::ButtonSubtitle)
                {
                    switchSubtitle();
                }
            }
#ifdef _WINDLL
            if (e.button.button == BP_BUTTON_RIGHT)
            {
                loop = false;
                running_ = false;
            }
#endif
            break;
        case BP_MOUSEWHEEL:
        {
            if (e.wheel.y > 0)
            {
                cur_volume_ = media_->getAudio()->changeVolume(volume_step);
            }
            else if (e.wheel.y < 0)
            {
                cur_volume_ = media_->getAudio()->changeVolume(-volume_step);
            }
            UI_.setText("v");
            break;
        }
        case BP_KEYDOWN:
        {
            switch (e.key.keysym.sym)
            {
            case BPK_LEFT:
                media_->seekTime(media_->getTime() - seek_step, -1);
                UI_.setText("");
                seeking = true;
                break;
            case BPK_RIGHT:
                media_->seekTime(media_->getTime() + seek_step, 1);
                UI_.setText("");
                seeking = true;
                break;
            case BPK_UP:
                cur_volume_ = media_->getAudio()->changeVolume(volume_step);
                UI_.setText("v");
                break;
            case BPK_DOWN:
                cur_volume_ = media_->getAudio()->changeVolume(-volume_step);
                UI_.setText("v");
                break;
            case BPK_1:
                media_->switchStream(BPMEDIA_TYPE_AUDIO);
                UI_.setText(convert::formatString("Switch audio stream to %d", media_->getAudio()->getStreamIndex()));
                break;
            case BPK_2:
                switchSubtitle();
                break;
            }
            break;
        }
        case BP_KEYUP:
        {
            switch (e.key.keysym.sym)
            {
            case BPK_ESCAPE:
                if (engine_->isFullScreen())
                {
                    engine_->toggleFullscreen();
                    setSubtitleFrameSize();
                }
                else
                {
                    loop = false;
                    running_ = false;
                }
                break;
            case BPK_BACKSPACE:
                media_->seekTime(0);
                seeking = true;
                break;
#ifndef _WINDLL
            case BPK_SPACE:
                pause = !pause;
                media_->setPause(pause);
                break;
            case BPK_RETURN:
                engine_->toggleFullscreen();
                setSubtitleFrameSize();
                break;
            case BPK_DELETE:
                Config::getInstance()->clearRecord();
                break;
            case BPK_PERIOD:
            {
                find_direct = 1;
                auto next_file = findNextFile(drop_filename_, find_direct);
                if (next_file != "")
                {
                    drop_filename_ = next_file;
                    loop = false;
                }
                break;
            }
            case BPK_COMMA:
            {
                find_direct = -60 * 1000;
                auto next_file = findNextFile(drop_filename_, find_direct);
                if (next_file != "")
                {
                    drop_filename_ = next_file;
                    loop = false;
                }
                break;
            }
            case BPK_EQUALS:
            {
                int w, h;
                engine_->getWindowSize(w, h);
                w += width_ / 4;
                h += height_ / 4;
                setWindowSize(w, h);
                engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);
                break;
            }
            case BPK_MINUS:
            {
                int w, h;
                engine_->getWindowSize(w, h);
                w -= width_ / 4;
                h -= height_ / 4;
                setWindowSize(w, h);
                engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);
                break;
            }
            case BPK_0:
                setWindowSize(media_->getVideo()->getWidth(), media_->getVideo()->getHeight());
                engine_->setWindowPosition(BP_WINDOWPOS_CENTERED, BP_WINDOWPOS_CENTERED);
                break;
#endif
            }
            break;
        }
        //#ifndef _WINDLL
        case BP_QUIT:
            //pause = true;
#ifdef _WINDLL
            engine_->delay(10);
#endif
            //media_->setPause(pause);
            loop = false;
            running_ = false;
            exit_type_ = 1;
            break;
        //#endif
        case BP_WINDOWEVENT:
            if (e.window.event == BP_WINDOWEVENT_RESIZED)
            {
                setWindowSize(e.window.data1, e.window.data2);
            }
            else if (e.window.event == BP_WINDOWEVENT_LEAVE)
            {
            }
            break;
        case BP_DROPFILE:
            //有文件拖入先检查是不是字幕，不是字幕则当作媒体文件，打开失败活该
            //若将媒体文件当成字幕打开会非常慢，故限制字幕文件的扩展名
            open_filename = PotConv::conv(e.drop.file, BP_encode_, sys_encode_);
            printf("Change file: %s\n", open_filename.c_str());
            //检查是不是字幕，如果是则打开
            if (PotSubtitleManager::isSubtitle(open_filename))
            {
                PotSubtitleManager::destroySubtitle(subtitle_);
                subtitle_ = PotSubtitleManager::createSubtitle(open_filename);
                setSubtitleFrameSize();
                if (subtitle_ && subtitle_->exist())
                {
                    sub_state = 1;
                }
            }
            else
            {
                drop_filename_ = e.drop.file;
                loop = false;
            }
            engine_->free(e.drop.file);
            break;
        default:
            break;
        }
        e.type = BP_FIRSTEVENT;

        if (!loop)
        {
            break;
        }
        //在每个循环均尝试预解压
        media_->decodeFrame();
        //尝试以音频为基准显示视频
        int audioTime = media_->getTime();    //注意优先为音频时间，若音频不存在使用视频时间
        //if (seeking)
        //{
        //    cout << audioTime << " " << media_->getAudioStream()->getTimedts() << endl;
        //}

        if (last_volume == media_->getAudio()->getVolume())
        {
            volume_step = 1;
        }
        else
        {
            volume_step = (std::min)(volume_step + 1, 5);
        }

        int time_s = audioTime;
        if (pause)
        {
            time_s = 0;    //pause时不刷新视频时间轴，而依赖后面显示静止图像的语句
        }
        int videostate = media_->getVideo()->show(time_s);

        //播放回调
        if (play_callback)
        {
            play_callback(audioTime);
        }
        //结束回调
        if (stop_callback && videostate == PotStreamVideo::NoVideoFrame)
        {
            char s[1024];
            stop_callback(&loop, s);
            if (!loop)
            {
                drop_filename_ = s;
            }
        }

        //printf("\nvideostate%d", videostate);
        //依据解视频的结果判断是否显示
        bool show = false;
        //有视频显示成功，或者有静态视频，或者只有音频，均刷新
        if (videostate == PotStreamVideo::VideoFrameShowed)
        {
            show = true;
            //以下均是为了显示信息，可以去掉
#ifdef _DEBUG
            int videoTime = (media_->getVideo()->getTimedts());
            int delay = -videoTime + audioTime;
            printf("\rvolume %d, audio %4.3f, video %4.3f, diff %1.3f in loop %d\t",
                media_->getAudio()->changeVolume(0), audioTime / 1e3, videoTime / 1e3, delay / 1e3, i);
#endif
        }
        //静止时，无视频时，视频已放完时40毫秒显示一次
        //有视频未暂停且未到时间不会进入此判断
        else if ((pause || videostate == PotStreamVideo::NoVideo || videostate == PotStreamVideo::NoVideoFrame) && engine_->getTicks() - prev_show_time > 100)
        {
            show = true;
            if (havevideo)
            {
                engine_->renderCopy();
            }
            else
            {
                engine_->showLogo();
            }
        }
        if (show)
        {
            if (sub_state == 1 && subtitle_ && subtitle_->exist())
            {
                subtitle_->show(audioTime);
            }
            if (sub_state >= 2)
            {
                media_->getSubtitle()->show(audioTime);
            }
            UI_.drawUI(audioTime, totalTime, cur_volume_, pause);
            engine_->renderPresent();
            prev_show_time = engine_->getTicks();
        }
        i++;
        engine_->delay(1);
        if (audioTime >= totalTime)
        {
            if (Config::getInstance()->getInteger("loop", 0))
            {
                find_direct = 0;
            }
            auto next_file = findNextFile(drop_filename_, find_direct);
            if (next_file != "")
            {
                drop_filename_ = next_file;
                Config::getInstance()->setRecord(drop_filename_, 0);    //若是自动跳转，则设置从头开始
                loop = false;
            }
        }
#ifdef _WINDLL
        if (videostate == PotStreamVideo::NoVideo || time_s >= totalTime)
        {
            loop = false;
            running_ = false;
        }
#endif
    }
    engine_->renderClear();
    engine_->renderPresent();

    auto s = convert::formatString("%d", i);
    //engine_->showMessage(s);
    return exit_type_;
}

int PotPlayer::init()
{
    if (engine_->init(handle_, handle_type_))
    {
        return -1;
    }
    Config::getInstance()->init(run_path_);
#ifdef _WIN32
    sys_encode_ = Config::getInstance()->getString("sys_encode", "cp936");
#else
    sys_encode_ = Config::getInstance()->getString("sys_encode", "utf-8");
#endif
    cur_volume_ = Config::getInstance()->getInteger("volume", BP_AUDIO_MIX_MAXVOLUME / 2);
    UI_.init();
    return 0;
}

void PotPlayer::destroy()
{
    UI_.destory();
    engine_->destroy();
#ifndef _WINDLL
    Config::getInstance()->write();
#endif
}

//参数为utf8编码
void PotPlayer::openMedia(const std::string& filename)
{
    media_ = nullptr;
    media_ = new PotMedia;
#ifndef _WINDLL
    //某些格式的媒体是分开为很多个文件，这类文件最好先切换工作目录
    File::changePath(File::getFilePath(filename));
#endif
    //通过参数传入的字串被SDL转为utf-8
    //打开文件, 需要进行转换
    auto open_filename = PotConv::conv(filename, BP_encode_, sys_encode_);    //windows下打开需要ansi
    if (media_->openFile(open_filename) != 0)
    {
        return;
    }

    //窗口尺寸，时间
    width_ = media_->getVideo()->getWidth();
    height_ = media_->getVideo()->getHeight();
    engine_->setRatio(media_->getVideo()->getRatioX(), media_->getVideo()->getRatioY());
    engine_->setRotation(media_->getVideo()->getRotation());
#ifndef _WINDLL
    if (engine_->isFullScreen())    // || (engine_->getWindowWidth() >= width_ && engine_->getWindowHeight() >= height_))
    {
        //此处原来是视频尺寸小于窗口则不改变，现移除此功能
    }
    else
    {
        int maxw = engine_->getMaxWindowWidth();
        int maxh = engine_->getMaxWindowHeight();
        //搞点余量？
        engine_->setWindowSize(std::min(width_, maxw - 80), std::min(height_, maxh - 80));
    }
    engine_->setWindowTitle(PotConv::conv(File::getFilenameWithoutPath(open_filename), sys_encode_, BP_encode_));
#endif
    engine_->createMainTexture(media_->getVideo()->getSDLPixFmt(), width_*2, height_*2);

    //重新获取尺寸，有可能与之前不同
    width_ = engine_->getWindowWidth();
    height_ = engine_->getWindowHeight();

    //音量
    media_->getAudio()->setVolume(cur_volume_);

    //试图载入字幕
    auto open_subfilename = PotSubtitleManager::lookForSubtitle(open_filename);
    subtitle_ = PotSubtitleManager::createSubtitle(open_subfilename);
    setSubtitleFrameSize();

#ifndef _WINDLL
    //读取记录中的文件时间并跳转
    if (media_->isMedia())
    {
        cur_time_ = 0;
        cur_time_ = Config::getInstance()->getRecord(filename.c_str());
        printf("Play from %1.3fs\n", cur_time_ / 1000.0);
        if (cur_time_ > 0 && cur_time_ < media_->getTotalTime())
        {
            media_->seekTime(cur_time_, -1);
        }
    }
#endif
}

void PotPlayer::setSubtitleFrameSize()
{
    if (subtitle_)
    {
        subtitle_->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());
    }
    media_->getSubtitle()->setFrameSize(engine_->getPresentWidth(), engine_->getPresentHeight());
}

void PotPlayer::closeMedia(const std::string& filename)
{
    //记录播放时间
    cur_time_ = media_->getTime();
    cur_volume_ = media_->getAudio()->getVolume();

    //关闭字幕
    PotSubtitleManager::destroySubtitle(subtitle_);
    //_subtitle->closeSubtitle();

    //如果是媒体文件就记录时间
#ifndef _WINDLL
    auto config = Config::getInstance();
    if (media_->isMedia() && cur_time_ < media_->getTotalTime() && cur_time_ > 0)
    {
        config->setRecord(filename, cur_time_);
    }
    else
    {
        config->removeRecord(filename);
    }
    config->setString("sys_encode", sys_encode_);
    config->setInteger("volume", cur_volume_);
    if (config->getInteger("auto_play_recent") && !drop_filename_.empty())
    {
        config->setString("recent_file", drop_filename_);
    }
    config->write();
#endif
    delete media_;
    File::changePath(run_path_);
}

std::string PotPlayer::findNextFile(const std::string& filename, int direct)
{
#ifdef _WINDLL
    return "";
#endif
    if (filename == "")
    {
        return "";
    }
    if (direct == 0)
    {
        return filename;
    }
    std::string next_file;
    auto filename1 = PotConv::conv(drop_filename_, BP_encode_, sys_encode_);
    auto path = File::getFilePath(filename1);
    filename1 = File::getFilenameWithoutPath(filename1);
    auto ext = File::getFileExt(filename1);
    auto files = File::getFilesInPath(path);
    //只查找相同扩展名
    for (auto it = files.begin(); it != files.end();)
    {
        if (File::getFileExt(*it) == ext)
        {
            it++;
        }
        else
        {
            it = files.erase(it);
        }
    }
    std::sort(files.begin(), files.end());
    if (direct > 0)
    {
        for (int i = 0; i < int(files.size()) - 1; i++)
        {
            if (files.at(i) == filename1)
            {
                next_file = files.at(i + 1);
            }
        }
    }
    else
    {
        for (int i = int(files.size()) - 1; i > 0; i--)
        {
            if (files.at(i) == filename1)
            {
                next_file = files.at(i - 1);
            }
        }
    }
    if (next_file != "")
    {
        return PotConv::conv(path + "/" + next_file, sys_encode_, BP_encode_);
    }
    else
    {
        return "";
    }
}

void PotPlayer::setWindowSize(int w, int h)
{
    engine_->setWindowSize(w, h);
    engine_->getWindowSize(width_, height_);
    engine_->setPresentPosition();
    setSubtitleFrameSize();
}
