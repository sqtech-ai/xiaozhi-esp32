//
//  EspBoard.cpp
//
#include <memory>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include "board.h"
#include "IOTBoard.h"

#define TAG "EspBoard"

class BoardHttpImpl: public BoardHttp
{
public:
    BoardHttpImpl():http_(Board::GetInstance().GetNetwork()->CreateHttp(1)) {}
    virtual ~BoardHttpImpl() = default;
    void SetTimeout(int timeout_ms) override {
        http_->SetTimeout(timeout_ms);
    }
    void SetHeader(const std::string& key, const std::string& value) override {
        http_->SetHeader(key, value);
    }
    void SetContent(std::string&& content) override {
        http_->SetContent(std::move(content));
    }
    bool Open(const std::string& method, const std::string& url) override {
        return http_->Open(method, url);
    }
    void Close() override {
        http_->Close();
    }
    int Read(char* buffer, size_t buffer_size) override {
        return http_->Read(buffer, buffer_size);
    }
    int Write(const char* buffer, size_t buffer_size) override {
        return http_->Write(buffer, buffer_size);
    }
    int GetStatusCode() override {
        return http_->GetStatusCode();
    }
    std::string GetResponseHeader(const std::string& key) const override {
        return http_->GetResponseHeader(key);
    }
    size_t GetBodyLength() override {
        return http_->GetBodyLength();
    }
    std::string ReadAll() override {
        return http_->ReadAll();
    }
private:
    std::unique_ptr<Http> http_;
};

class BoardMqttImpl: public BoardMqtt
{
public:
    BoardMqttImpl():mqtt_(Board::GetInstance().GetNetwork()->CreateMqtt(1)) {}
    virtual ~BoardMqttImpl() = default;
    void SetKeepAlive(int keep_alive_seconds) override {
        mqtt_->SetKeepAlive(keep_alive_seconds);
    }
    bool Connect(const std::string broker_address, int broker_port, const std::string client_id, const std::string username, const std::string password) override {
        return mqtt_->Connect(broker_address, broker_port, client_id, username, password);
    }
    void Disconnect() override {
        mqtt_->Disconnect();
    }
    bool Publish(const std::string topic, const std::string payload, int qos = 0) override {
        return mqtt_->Publish(topic, payload, qos);
    }
    bool Subscribe(const std::string topic, int qos = 0) override {
        return mqtt_->Subscribe(topic, qos);
    }
    bool Unsubscribe(const std::string topic) override {
        return mqtt_->Unsubscribe(topic);
    }
    bool IsConnected() override {
        return mqtt_->IsConnected();
    }
    void OnConnected(std::function<void()> callback) override {
        mqtt_->OnConnected(callback);
    }
    void OnDisconnected(std::function<void()> callback) override {
        mqtt_->OnDisconnected(callback);
    }
    void OnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) override {
        mqtt_->OnMessage(callback);
    }
    void OnError(std::function<void(const std::string& error)> callback) override {
        mqtt_->OnError(callback);
    }
private:
    std::unique_ptr<Mqtt> mqtt_;
};

#if defined(HAVE_LVGL) && defined(CONFIG_IDF_TARGET_ESP32S3)
#include <jpeg_decoder.h>
std::unique_ptr<LvglImage> JpegToImage(char* data, int datalen, int target_width, int target_height)
{
    if (data == nullptr || datalen < 3) {
        return nullptr;
    }
    if (!(data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)) {
        return nullptr;
    }
    esp_err_t err = ESP_OK;
    esp_jpeg_image_cfg_t jpeg_cfg = {};
    jpeg_cfg.indata = (uint8_t*)data;
    jpeg_cfg.indata_size = datalen;
    jpeg_cfg.out_format = JPEG_IMAGE_FORMAT_RGB565;
    jpeg_cfg.out_scale = JPEG_IMAGE_SCALE_0;
    esp_jpeg_image_output_t source_info = {};
    if ((err = esp_jpeg_get_image_info(&jpeg_cfg, &source_info)) != ESP_OK) {
        ESP_LOGW(TAG, "esp_jpeg_get_image_info() = %d", err);
        return nullptr;
    }
    //缩放策略：先按1/8缩放，再按1/2缩放，再按1/4缩放，再按1/8缩放
    jpeg_cfg.out_scale = JPEG_IMAGE_SCALE_1_8;
    struct ScaleCandidate {
        esp_jpeg_image_scale_t scale;
        int divisor;
    };
    static const ScaleCandidate kCandidates[] = {
        {JPEG_IMAGE_SCALE_0, 1},
        {JPEG_IMAGE_SCALE_1_2, 2},
        {JPEG_IMAGE_SCALE_1_4, 4},
        {JPEG_IMAGE_SCALE_1_8, 8},
    };
    const size_t max_pixels = static_cast<size_t>(std::max(target_width, 1)) * static_cast<size_t>(std::max(target_height, 1)) * 3;
    for (const auto& candidate : kCandidates) {
        const size_t scaled_pixels = (static_cast<size_t>(source_info.width) / candidate.divisor) * (static_cast<size_t>(source_info.height) / candidate.divisor);
        if (scaled_pixels <= max_pixels) {
            jpeg_cfg.out_scale = candidate.scale;
            break;
        }
    }
    esp_jpeg_image_output_t output_info = {};
    if ((err = esp_jpeg_get_image_info(&jpeg_cfg, &output_info)) != ESP_OK || output_info.output_len == 0) {
        ESP_LOGW(TAG, "Failed to size JPEG music cover");
        return nullptr;
    }
    //ESP_LOGI(TAG, "source_info:%dx%d, output_info:%dx%d", source_info.width, source_info.height, output_info.width, output_info.height);
    void* decoded = heap_caps_malloc(output_info.output_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (decoded == nullptr) {
        ESP_LOGW(TAG, "heap_caps_malloc(%u) = %p", output_info.output_len, decoded);
        return nullptr;
    }
    jpeg_cfg.outbuf = static_cast<uint8_t*>(decoded);
    jpeg_cfg.outbuf_size = static_cast<uint32_t>(output_info.output_len);
    jpeg_cfg.flags.swap_color_bytes = 0;
    if ((err = esp_jpeg_decode(&jpeg_cfg, &output_info)) != ESP_OK) {
        ESP_LOGW(TAG, "esp_jpeg_decode() = %d", err);
        heap_caps_free(decoded);
        return nullptr;
    }
    return std::make_unique<LvglAllocatedImage>(decoded, output_info.output_len, output_info.width, output_info.height, output_info.width * 2, LV_COLOR_FORMAT_RGB565);
}
#endif