#ifndef ESP32_MUSIC_H
#define ESP32_MUSIC_H

#include "music.h"

class IMusicPlayer;
class Esp32Music : public Music {
public:
    Esp32Music();
    virtual ~Esp32Music();

    bool Play(const char* text, const char* songName, const char* singerName, const char* tagName) override;
    void Quit() override;
    bool Next() override;
    bool Pause() override;
    bool Resume() override;
    bool Previous() override;
    bool IsPlaying() const override;
    std::string GetSongList() const override;
protected:
    void EnterMusicPlaybackMode();
    void ExitMusicPlaybackMode();
private:
    bool is_playing_mode = false;
    IMusicPlayer* music_player_ = nullptr;
};

#endif // ESP32_MUSIC_H
