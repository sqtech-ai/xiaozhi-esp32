#include <esp_log.h>
#include <cstring>
#include <vector>
#include "board.h"
#include "display.h"
#include "audio_codec.h"
#include "lvgl_display.h"
#include "esp32_music.h"
#include "IMusicPlayer.h"
#include "application.h"


#define TAG "Esp32Music"

std::unique_ptr<LvglImage> JpegToImage(char* data, int datalen, int target_width, int target_height);

Esp32Music::Esp32Music(): music_player_(IMusicPlayer::GetInstance()) {
    auto& app = Application::GetInstance();
    auto display = dynamic_cast<LvglDisplay*>(Board::GetInstance().GetDisplay());
    auto codec = Board::GetInstance().GetAudioCodec();
    app.GetDeviceStateMachine().AddStateChangeListener([this](DeviceState prev, DeviceState curr) {
        ESP_LOGI(TAG, "DeviceStateChange: prev=%d curr=%d", prev, curr);
        if (music_player_->IsPlaying()) {
            if (curr == kDeviceStateConnecting || curr == kDeviceStateSpeaking) {
                ToggleQuit();
            }
        }
    });
    music_player_->SetOnStarted([display](const char* songName, const char* singerName) {
        std::string title = songName;
        if(singerName && *singerName) {
            title += " - " + std::string(singerName);
        }
        ESP_LOGI(TAG, "OnPlaybackStarted: %s", title.c_str());
        display->SetMusicTitle(title.c_str());
    });
    music_player_->SetOnFinished([this, display](int code, const char* message) {
        ESP_LOGI(TAG, "OnPlaybackFinished:%d, %s", code, message);
        display->SetPreviewImage(nullptr);
        display->SetMusicTitle(nullptr);
        display->SetMusicProgress(0, 0, nullptr);
        Application::GetInstance().Schedule([this, code]() {
            if(code == EMP_RET_EOS) {
                Next();
            } else {
                ToggleQuit();
                ExitMusicPlaybackMode();//退出播放模式
            }
        });
    });
    music_player_->SetOnProgress([display](int current_ms, int total_ms, const char* lyric_previous, const char* lyric_current, const char* lyric_next) {
        if (current_ms < 0 || total_ms <= 0) {
            return;
        }
        display->SetMusicProgress(current_ms, total_ms, lyric_current);
    });
    music_player_->SetOnPcmReady([codec](char* pcm, int length) {
        if (pcm == nullptr || length <= 0 || (length % (int)sizeof(int16_t)) != 0) {
            return;
        }
        const size_t samples = static_cast<size_t>(length) / sizeof(int16_t);
        std::vector<int16_t> pcm_data(samples);
        memcpy(pcm_data.data(), pcm, static_cast<size_t>(length));
        codec->OutputData(pcm_data);
    });
    music_player_->SetOnCoverImageReady([display](char* data, int datalen) {
        if (data == nullptr || datalen <= 0) {
            return;
        }
        auto image = JpegToImage(data, datalen, display->width(), display->height());
        if (image != nullptr) {
            display->SetPreviewImage(std::move(image));
        }
    });
    music_player_->Init("{\"audioResamplerFast\":true,\"reportOnQuit\":false,\"playTaskStackSize\":4500}");
    music_player_->SetAudioSink(codec->output_sample_rate(), codec->output_channels());
}

Esp32Music::~Esp32Music() {

}

bool Esp32Music::Play(const char* text, const char* songName, const char* singerName, const char* tagName) {
    int result = music_player_->Search(text, songName, singerName, tagName);
    if(result != EMP_RET_OK) {
        ESP_LOGI(TAG, "Search failed: %d, %s", result, music_player_->GetMessage());
        return false;
    }
    ESP_LOGI(TAG, "[%s:%d]stack=%d", __func__, __LINE__, uxTaskGetStackHighWaterMark(nullptr));
    if((result = music_player_->Play()) == EMP_RET_OK) {
        EnterMusicPlaybackMode();//进入播放模式
    } else {
        ESP_LOGI(TAG, "Play:%d, %s", music_player_->GetError(), music_player_->GetMessage());
        ExitMusicPlaybackMode();
    }
    return result == EMP_RET_OK;
}

void Esp32Music::Quit() {
    music_player_->Quit();
    ExitMusicPlaybackMode();
}

void Esp32Music::ToggleQuit() {
    music_player_->ToggleQuit();
}

bool Esp32Music::Next() {
    if(music_player_->IsAvailable() && !music_player_->IsPlaying()) {
        int result = -1;
        if((result = music_player_->Next()) == EMP_RET_OK) {
            EnterMusicPlaybackMode();//进入播放模式
        } else {
            ESP_LOGI(TAG, "Next:%d, %s", music_player_->GetError(), music_player_->GetMessage());
            ExitMusicPlaybackMode();
        }
        return result == EMP_RET_OK;
    }
    ESP_LOGI(TAG, "Next: not can play");
    return false;
}

bool Esp32Music::Previous() {
    if(music_player_->IsAvailable() && !music_player_->IsPlaying()) {
        int result = -1;
        if((result = music_player_->Previous()) == EMP_RET_OK) {
            EnterMusicPlaybackMode();//进入播放模式
        } else {
            ESP_LOGI(TAG, "Previous:%d, %s", music_player_->GetError(), music_player_->GetMessage());
            ExitMusicPlaybackMode();
        }
        return result == EMP_RET_OK;
    }
    ESP_LOGI(TAG, "Previous: not can play");
    return false;
}

bool Esp32Music::IsPlaying() const {
    return music_player_->IsPlaying();
}

std::string Esp32Music::GetSongList() const {
    return music_player_->GetSongList();
}

void Esp32Music::EnterMusicPlaybackMode() {
    if (is_playing_mode) {
        return;
    }
    is_playing_mode = true;
    auto& app = Application::GetInstance();
    auto& audio_service = app.GetAudioService();
    app.AbortSpeaking(kAbortReasonNone);//停止说话
    app.StopListening();//停止监听
    app.CloseAudioChannelIfOpened();//关闭和云端的对话通道
    audio_service.ToggleMusicPlaying(true);//开启音乐播放
    audio_service.EnableVoiceProcessing(false);//关闭语音处理管线(上行语音)
}

void Esp32Music::ExitMusicPlaybackMode() {
    if (!is_playing_mode) {
        return;
    }
    is_playing_mode = false;
    auto& app = Application::GetInstance();
    auto& audio_service = app.GetAudioService();
    audio_service.ToggleMusicPlaying(false);//关闭音乐播放
    audio_service.EnableVoiceProcessing(true);//开启语音处理管线(上行语音)
}
