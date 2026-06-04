//
//  IMusicPlayer.h
//
#ifndef _I_MUSIC_PLAYER_H_
#define _I_MUSIC_PLAYER_H_

#include <cstdint>
#include <memory>
#include <functional>
#include <string>

/*------------------------- 错误码定义 -------------------------*/
/**
 * 成功
 */
#define EMP_RET_OK 0
/**
 * 错误
 */
#define EMP_RET_ERR -1
/**
 * 音乐流播放结束
 */
#define EMP_RET_EOS -2
/**
 * 取消播放
 */
#define EMP_RET_CANCEL -3
/**
 * 暂停播放
 */
#define EMP_RET_PAUSE -4 
/**
 * 内存错误
 */
#define EMP_RET_MEM_ERR -5
/**
 * 网络连接错误
 */
#define EMP_RET_NETWORK_ERR -6
/**
 * HTTP请求错误
 */
#define EMP_RET_HTTP_ERR -7
/**
 * 音频解码错误
 */
#define EMP_RET_DECODE_ERR -8

class Http;
class IMusicPlayer {
public:
    /**
     * 获取单例
     */
    static IMusicPlayer* GetInstance();
    virtual ~IMusicPlayer() = default;
    /**
     * 初始化
     * @param config_json 配置参数(json字符串)
     */
    virtual void Init(const char* config_json) = 0;
    /**
     * 搜索歌曲
     * @param text 搜索内容(必填)
     * @param songName 歌曲名称(可选)
     * @param singerName 歌手名称(可选)
     * @param tagName 标签名称(可选)
     * @return (<0)错误码；(>=0)搜索到的歌曲总数
     */
    virtual int Search(const char* text, const char* songName, const char* singerName, const char* tagName) = 0;
    /**
     * 播放歌曲
     * @param index 歌曲索引(可选，默认0)
     * @return 错误码
     */
    virtual int Play(int index = 0) = 0;
    /**
     * 停止播放
     * @param timeout_ms 超时时长，单位ms
     */
    virtual void Quit(int32_t timeout_ms = 0) = 0;
    /**
     * 播放下一首
     * @return 错误码
     */
    virtual int Next() = 0;
    /**
     * 播放上一首
     * @return 错误码
     */
    virtual int Previous() = 0;
    /**
     * 暂停播放
     * @return 错误码
     */
    virtual int Pause() = 0;
    /**
     * 继续播放
     * @return 错误码
     */
    virtual int Resume() = 0;
     /**
     * 是否正在播放
     */
    virtual bool IsPlaying() const = 0;
    /**
     * 是否可用(歌曲列表是否为空?)
     */
    virtual bool IsAvailable() const = 0;
    /**
     * 获取错误码
     * @return 错误码
     */
    virtual int GetError() const = 0;
    /**
     * 获取错误信息
     * @return 错误信息
     */
    virtual const char* GetMessage() const = 0;
    /**
     * 获取歌曲名称
     * @return 歌曲名称
     */
    virtual const char* GetSongName() const = 0;
    /**
     * 获取歌手名称
     * @return 歌手名称
     */
    virtual const char* GetSingerName() const = 0;
    /**
     * 获取歌曲列表（JSON 字符串）
     * @return 队列快照 JSON；无列表时返回空对象 "{}"
     */
    virtual std::string GetSongList() const = 0;
    /**
     * 设置播放状态为停止(仅设置标志，不等待；区别于Quit()，后者会等待任务结束)
     */
    void ToggleQuit() { quit_ = true; }
    /**
     * 是否暂停
     * @return 是否暂停
     */
    bool IsPaused() const { return paused_; }
    /**
     * 设置音频输出(采样率、通道数)
     * @param sample_rate 采样率
     * @param channels 通道数
     */
    void SetAudioSink(int sample_rate, int channels) { audio_sink_sample_rate_ = sample_rate; audio_sink_channels_ = channels; }
    /**
     * 设置播放开始回调(可选)
     * @param songName 歌曲名称
     * @param singerName 歌手名称
     */
    void SetOnStarted(std::function<void(const char* songName, const char* singerName)> on_started) { on_started_ = on_started; }
    /**
     * 设置播放完成回调(必选)
     * @param code 错误码
     * @param message 错误信息
     */
    void SetOnFinished(std::function<void(int code, const char* message)> on_finished) { on_finished_ = on_finished; }
    /**
     * 设置PCM数据准备好回调(必选)
     * @param pcm PCM数据
     * @param length PCM数据长度
     */
    void SetOnPcmReady(std::function<void(char* pcm, int length)> on_pcm_ready) { on_pcm_ready_ = on_pcm_ready; }
    /**
     * 设置封面数据准备好回调(可选)
     * @param data 封面数据
     * @param datalen 封面数据长度
     */
    void SetOnCoverImageReady(std::function<void(char* data, int datalen)> on_cover_image_ready) { on_cover_image_ready_ = on_cover_image_ready; }
    /**
     * 设置播放进度回调(可选)
     * @param current_ms 当前播放时间
     * @param total_ms 总时间
     * @param lyric_previous 上一句歌词
     * @param lyric_current 当前歌词
     * @param lyric_next 下一句歌词
     */
    void SetOnProgress(std::function<void(int current_ms, int total_ms, const char* lyric_previous, const char* lyric_current, const char* lyric_next)> on_progress) { on_progress_ = on_progress; }
protected:
    bool quit_ = false;
    bool paused_ = false;
    int err_code_ = 0;
    const char* err_message_ = nullptr;
    int audio_sink_sample_rate_ = 0, audio_sink_channels_ = 0;
    std::function<void(const char* songName, const char* singerName)> on_started_;
    std::function<void(int code, const char* message)> on_finished_;
    std::function<void(char* pcm, int length)> on_pcm_ready_;
    std::function<void(char* data, int datalen)> on_cover_image_ready_;
    std::function<void(int current_ms, int total_ms, const char* lyric_previous, const char* lyric_current, const char* lyric_next)> on_progress_;
};

#endif // _I_MUSIC_PLAYER_H_