#ifndef MUSIC_H
#define MUSIC_H

#include <string>

class Music {
public:
    virtual ~Music() = default;
    virtual bool Play(const char* text, const char* songName, const char* singerName, const char* tagName) = 0;
    virtual void Quit() = 0;
    virtual bool Next() = 0;
    virtual bool Previous() = 0;
    virtual bool Pause() = 0;
    virtual bool Resume() = 0;
    virtual bool IsPlaying() const = 0;
    virtual std::string GetSongList() const = 0;
};

#endif // MUSIC_H