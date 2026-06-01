# IMusicPlayer 对接接口说明

本文档依据源码头文件 [`IMusicPlayer.h`](../IMusicPlayer.h) 整理，供第三方在集成 **IOTSdk 在线音乐播放能力** 时对照使用。

默认实现类为 [`EspMusicPlayer`](../EspMusicPlayer.h)（`IMusicPlayer::GetInstance()` 返回单例）。搜索与补全能力依赖已配置的 [`IOTSdk`](./IOTSdk.md)（须先 `SetOnGetBoardHttp`，再 `Init`；播放器侧使用 `SearchSongEx` / `GetMusicInfo` 等）。集成顺序建议：`IOTSdk::SetOnGetBoardHttp` → `IOTSdk::Init` → `IMusicPlayer::Init` → 注册回调 → `Search` / `Play`。

---

## 1. 错误码宏

| 宏 | 值 | 含义 |
|----|-----|------|
| `EMP_RET_CANCEL` | `2` | 取消播放（如调用 `Quit()` / `ToggleQuit()` 中断） |
| `EMP_RET_EOS` | `1` | 音乐流播放结束（正常播完） |
| `EMP_RET_OK` | `0` | 成功 |
| `EMP_RET_ERR` | `-1` | 通用错误 |
| `EMP_RET_MEM_ERR` | `-2` | 内存错误 |
| `EMP_RET_NETWORK_ERR` | `-3` | 网络连接错误 |
| `EMP_RET_HTTP_ERR` | `-3` | HTTP 请求错误 |
| `EMP_RET_DECODE_ERR` | `-4` | 音频解码错误 |

接口注释中“`@return` 错误码”与上表一致；`Play` / `Search` / `Next` / `Previous` 等方法失败时，可通过 `GetError()` / `GetMessage()` 获取最近一次错误详情。

> **说明：** `EMP_RET_NETWORK_ERR` 与 `EMP_RET_HTTP_ERR` 在头文件中均为 `-3`，判断时请结合 `GetMessage()` 或调用上下文区分。

---

## 2. C++ 接口：`class IMusicPlayer`

单例获取：

```cpp
IMusicPlayer* IMusicPlayer::GetInstance();
```

以下方法均在 `IMusicPlayer` 实例上调用（具体行为以 `EspMusicPlayer` 实现为准，其他实现可替换）。

---

### 2.1 `Init`

```cpp
virtual void Init(const char* config_json) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `config_json` | `const char*` | 播放器配置，**JSON 字符串**；可为 `nullptr` 或 `"{}"`，表示使用实现类默认值 |

**行为（`EspMusicPlayer`）：**

- 解析 `config_json` 中的可选字段，覆盖内部运行参数（未出现的字段保持 [`EspMusicPlayer.h`](../EspMusicPlayer.h) 中的默认值）。
- 打日志输出 SDK 版本与传入的配置原文。
- **不**初始化 `IOTSdk`，**不**创建播放任务；须在 `IOTSdk::Init` 与 `SetOnGetBoardHttp` 完成之后、`Search` / `Play` 之前调用。

**`EspMusicPlayer` 支持的 JSON 字段：**

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `audioResamplerFast` | bool | `true` | 是否使用快速重采样（传给 `EspAudioDec`） |
| `reportOnQuit` | bool | `false` | 为 `true` 时，曲目正常结束或 `Quit` 打断且 `err_code_` 为 `EMP_RET_OK` / `EMP_RET_EOS` 时调用 `IOTSdk::Report` 上报听歌记录 |
| `httpReadTimeoutMs` | number | `5000` | HTTP 拉流读超时（毫秒） |
| `httpRcvBufLength` | number | `2048` | HTTP 接收缓冲区大小（字节，`2 << 10`） |
| `decodeBufferSize` | number | `4700` | MP3 解码 PCM 缓冲区大小（字节） |
| `playTaskPriority` | number | `6` | FreeRTOS 播放任务优先级 |
| `playTaskStackSize` | number | `3500` | FreeRTOS 播放任务栈大小（字） |

**配置示例：**

```json
{
  "audioResamplerFast": true,
  "reportOnQuit": true,
  "httpReadTimeoutMs": 8000,
  "httpRcvBufLength": 4096,
  "decodeBufferSize": 4700,
  "playTaskPriority": 6,
  "playTaskStackSize": 3500
}
```

**返回值：** 无（`void`）。

---

### 2.2 `Search`

```cpp
virtual int Search(const char* text, const char* songName, const char* singerName, const char* tagName) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `text` | `const char*` | **搜索内容**（必填，写入请求体字段 `text`） |
| `songName` | `const char*` | 歌曲名称（可选）；可为 `nullptr` 或空字符串 |
| `singerName` | `const char*` | 歌手名称（可选）；可为 `nullptr` 或空字符串 |
| `tagName` | `const char*` | 标签名称（可选，如流派/分类）；可为 `nullptr` 或空字符串 |

**行为（`EspMusicPlayer`）：**

- 清空上一次搜索结果与播放列表。
- 组装咪咕 `SearchSongEx` 请求（`provider=migu`，`type=1`，`searchType=4`，`pageIndex=1`，`pageSize=10`, `text=xxx` 等），按非空参数填充 `searchRange`（`songName` / `singerName` / `tagName` 各为字符串数组）。
- 成功后将响应中的 `data.searchSong` 数组缓存为内部播放列表，当前索引置为 `0`。

**返回值：** 错误码；`EMP_RET_OK` 表示搜索成功且列表非空。

---

### 2.3 `Play`

```cpp
virtual int Play(int index = 0) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `index` | `int` | 播放列表中的曲目索引，默认 `0`（第一首） |

**前置条件（`EspMusicPlayer`）：**

- 已调用 `Search` 且内部列表非空（参见 `IsAvailable()`）。
- 已通过 `SetAudioSink` 配置有效采样率与通道数。
- 当前无播放任务在运行（`IsPlaying()` 为 `false`）。

**行为：**

- 根据 `index` 选取列表项，解析 `listenUrl`（必要时通过 `GetMusicInfo` 补全 URL）。
- 在独立任务 `PlayTask` 中执行：先调用 `SetOnStarted(songName, singerName)`（若已注册）；再在 `PlayImpl` 中拉流、解码，PCM 经 `SetOnPcmReady` 输出。
- 若注册了 **`SetOnCoverImageReady`**：`PlayImpl` 开头会先下载 `picUrl` 并回调封面，再下载 `lrcUrl` 并解析 LRC 歌词（供进度回调使用）；二者均依赖 `IOTSdk::SetOnGetBoardHttp`（`IOTSDK_MIGU_HTTPS`）。
- 若注册了 **`SetOnProgress`** 且曲目 `length`（`HH:mm:ss`）可解析为 `total_ms > 0`，播放循环中约 **每秒** 回调一次，参数含当前进度与上/当前/下一句歌词。
- 任务结束时调用 `SetOnFinished(code, message)`。
- **`EspMusicPlayer`：** 若 `Init` 时设置 `"reportOnQuit": true`，且曲目以 `EMP_RET_OK` / `EMP_RET_EOS` 结束，则在 `OnFinished` 之前调用 `IOTSdk::Report()` 上报听歌记录（见 [IOTSdk.md](./IOTSdk.md) 第 4.7 节）。

**返回值：** 错误码；`EMP_RET_OK` 表示播放任务已成功创建（异步播放，结束见 `OnFinished`）。

---

### 2.4 `Quit`

```cpp
virtual void Quit(int32_t timeout_ms = 0) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `timeout_ms` | `int32_t` | 等待播放任务结束的超时时间，单位 **毫秒**；`0` 表示一直等待直到任务退出 |

**行为：**

- 置位停止标志并等待播放任务结束。
- **`EspMusicPlayer`：** 同时释放内部歌曲列表（`song_array_`），之后需重新 `Search` 才能再次播放。

**返回值：** 无（`void`）。

---

### 2.5 `Next`

```cpp
virtual int Next() = 0;
```

播放列表中的 **下一首**（当前索引 `+ 1`）。

**前置条件：** 与 `Play` 相同；且存在下一首（索引未越界）。

**返回值：** 错误码；语义同 `Play`。

---

### 2.6 `Previous`

```cpp
virtual int Previous() = 0;
```

播放列表中的 **上一首**（当前索引 `- 1`）。

**前置条件：** 与 `Play` 相同；且存在上一首（索引 `> 0`）。

**返回值：** 错误码；语义同 `Play`。

---

### 2.7 `IsPlaying`

```cpp
virtual bool IsPlaying() const = 0;
```

| 返回值 | 说明 |
|--------|------|
| `true` | 正在播放（播放任务尚未结束） |
| `false` | 未在播放 |

---

### 2.8 `IsAvailable`

```cpp
virtual bool IsAvailable() const = 0;
```

| 返回值 | 说明 |
|--------|------|
| `true` | 播放器可用（头文件注释：歌曲列表是否为空；`EspMusicPlayer` 还要求已配置音频输出） |
| `false` | 不可用（未搜索、列表为空或未设置 `SetAudioSink`） |

**`EspMusicPlayer` 判定条件：** `song_array_` 非空且 `song_count_ > 0`，且 `audio_sink_sample_rate_`、`audio_sink_channels_` 均已大于 0。

---

### 2.9 `GetError`

```cpp
virtual int GetError() const = 0;
```

| 返回值 | 说明 |
|--------|------|
| `int` | 最近一次操作的错误码（见第 1 节） |

---

### 2.10 `GetMessage`

```cpp
virtual const char* GetMessage() const = 0;
```

| 返回值 | 说明 |
|--------|------|
| `const char*` | 最近一次操作的错误描述字符串；成功时可能为空字符串 |

---

### 2.11 `GetSongList`

```cpp
virtual std::string GetSongList() const = 0;
```

获取当前搜索得到的播放队列快照（**JSON 字符串**）。无列表或列表为空时返回 `"{}"`。

**`EspMusicPlayer` JSON 结构：**

| 字段 | 类型 | 说明 |
|------|------|------|
| `active` | bool | 固定为 `true`（表示队列有效） |
| `intent` | string | 固定为 `"play_song"` |
| `songCount` | number | 曲目总数 |
| `songIndex` | number | 当前播放/选中索引 |
| `songs` | array | 曲目列表 |
| `songs[].musicId` | string | 歌曲 ID（来自 `SearchSongEx`） |
| `songs[].songName` | string | 歌曲名称（源字段 `musicName`） |
| `songs[].singerName` | string | 歌手名称 |

**示例：**

```json
{
  "active": true,
  "intent": "play_song",
  "songCount": 10,
  "songIndex": 0,
  "songs": [
    { "musicId": "600913000009663374", "songName": "晴天", "singerName": "周杰伦" }
  ]
}
```

---

## 3. 配置与回调（非虚函数）

以下方法在基类 `IMusicPlayer` 中以内联方式提供，用于集成方注入音频输出参数与接收播放事件。

| 方法 | 说明 |
|------|------|
| `ToggleQuit` | 仅置位停止标志，不等待任务结束 |
| `SetAudioSink` | 配置 PCM 输出采样率与通道数（`Play` 前必填） |
| `SetOnStarted` | 播放任务开始（**可选**） |
| `SetOnFinished` | 播放任务结束（**必选**） |
| `SetOnPcmReady` | 解码 PCM 就绪（**必选**） |
| `SetOnCoverImageReady` | 封面下载完成（**可选**）；`EspMusicPlayer` 中注册后还会下载并解析歌词 |
| `SetOnProgress` | 播放进度与上/当前/下句歌词（**可选**） |

> **板载 HTTP：** 须在 [`IOTSdk::SetOnGetBoardHttp`](./IOTSdk.md)（**`Init` 之前** 注册）中提供 `Http`，用于封面、歌词与音乐流。

### 3.1 `ToggleQuit`

```cpp
void ToggleQuit();
```

**说明：** 仅将内部 `quit_` 标志置为 `true`，**不等待**播放任务结束，也**不释放**歌曲列表。

**与 `Quit()` 的区别：**

| | `ToggleQuit()` | `Quit()` |
|---|----------------|----------|
| 等待任务结束 | 否 | 是（可配置超时） |
| 清空搜索列表 | 否 | 是（`EspMusicPlayer`） |
| 典型用途 | 快速打断当前曲目，随后可 `Play` / `Next` 切歌 | 完全停止并释放资源 |

---

### 3.2 `SetAudioSink`

```cpp
void SetAudioSink(int sample_rate, int channels);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `sample_rate` | `int` | 音频输出采样率（Hz） |
| `channels` | `int` | 音频输出通道数（如 `1` 单声道、`2` 立体声） |

**说明：** 解码器按此参数重采样/输出 PCM。调用 `Play` 前必须配置，否则 `IsAvailable()` 为 `false`。

---

### 3.3 `SetOnStarted`

```cpp
void SetOnStarted(std::function<void(const char* songName, const char* singerName)> on_started);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_started` | 回调函数 | 播放任务开始时调用；头文件标注为 **可选** |
| `songName` | `const char*` | 当前曲目名称（`GetSongName()`，可能为 `nullptr`） |
| `singerName` | `const char*` | 当前歌手名称（`GetSingerName()`，可能为 `nullptr`） |

**说明：** 在 `PlayTask` 入口、拉流之前同步调用；可用于更新 UI 歌名/歌手。

---

### 3.4 `SetOnFinished`

```cpp
void SetOnFinished(std::function<void(int code, const char* message)> on_finished);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_finished` | 回调函数 | 播放任务结束时调用；头文件标注为 **必选** |
| `code` | `int` | 结束原因错误码（如 `EMP_RET_EOS`、`EMP_RET_CANCEL`、失败码等） |
| `message` | `const char*` | 错误描述 |

---

### 3.5 `SetOnPcmReady`

```cpp
void SetOnPcmReady(std::function<void(char* pcm, int length)> on_pcm_ready);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_pcm_ready` | 回调函数 | PCM 数据就绪时调用；头文件标注为 **必选** |
| `pcm` | `char*` | PCM 缓冲区指针（一般为 `int16_t` 交错采样） |
| `length` | `int` | 数据长度（字节） |

**说明：** 在播放任务线程中同步调用，集成方应尽快拷贝或投递到音频输出队列，避免阻塞过久。

---

### 3.6 `SetOnCoverImageReady`

```cpp
void SetOnCoverImageReady(std::function<void(char* data, int datalen)> on_cover_image_ready);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_cover_image_ready` | 回调函数 | 封面二进制数据就绪时调用；**可选** |
| `data` | `char*` | 封面文件缓冲区（通常为 JPEG）；由 `EspHttpDownload` 在堆上分配 |
| `datalen` | `int` | 数据长度（字节） |

**行为（`EspMusicPlayer`）：**

- 回调返回后，播放器侧会对 `data` 调用 `IOTfree`；集成方须在回调内 **拷贝** 或解码完毕，勿保留指针。

---

### 3.7 `SetOnProgress`

```cpp
void SetOnProgress(std::function<void(int current_ms, int total_ms)> on_progress);
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_progress` | 回调函数 | 播放进度更新；**可选** |
| `current_ms` | `int` | 自开播起累计播放时长（毫秒） |
| `total_ms` | `int` | 曲目总时长（毫秒） |

**行为（`EspMusicPlayer`）：**

- 在 `PlayImpl` 主循环中，当 `total_ms > 0` 时约 **每 1 秒** 触发一次。
- `total_ms` 来自当前曲目 JSON 字段 `length` 字符串，格式为 **`HH:mm:ss`**（如 `"04:12:30"`），解析失败则为 `0`，此时不回调。
- `current_ms` 为 `GetTimestampMs() - 开播时刻`，与墙钟对齐，非解码帧精确 PTS。

**UI 集成示例：** 将 `current_ms` / `total_ms` 传入 `Display::SetMusicProgress(current_ms, total_ms)`。

---

## 4. 推荐调用流程

```text
IOTSdk::SetOnGetBoardHttp(...)              // Init 之前，必选（板载 HTTP）
    ↓
IOTSdk::Init(workspace, args)
    ↓
IMusicPlayer::GetInstance()
    ↓
Init(config_json)                          // 可选，配置拉流/解码/任务参数
    ↓
SetAudioSink(sample_rate, channels)
SetOnFinished / SetOnPcmReady              // 必选
SetOnStarted / SetOnCoverImageReady / SetOnProgress  // 可选
AddStateChangeListener（可选，对话态打断音乐，见 8.4）
    ↓
Search(text, songName, singerName, tagName)  // 构建播放列表（text 必填）
    ↓
Play(0) 或 Play(index)
    ↓
EnterMusicPlaybackMode()                  // 与 TTS/对话抢音源时必做，见 7.3
    ↓
[播放中] OnCoverImageReady（可选）→ 显示封面（同路径下载歌词）
    OnProgress（可选）→ 进度条 + 上/当前/下句歌词
    OnPcmReady → 送入扬声器/AudioService
    ↓
OnFinished / Quit → ExitMusicPlaybackMode()
    ↓
Next() / Previous()                       // 切歌（需先停止当前曲目或等其结束）
    ↓
Quit()                                    // 停止并清空列表
```

**切歌注意（`EspMusicPlayer`）：**

- `Play` / `Next` / `Previous` 要求当前 **没有** 正在运行的播放任务；边播边切需先 `ToggleQuit()` 并等待 `IsPlaying()` 变为 `false`，再调用 `Next()` / `Previous()`。
- `Quit()` 会清空列表，之后 `Next` / `Previous` 无效，需重新 `Search`。

---

## 5. 与 IOTSdk 的关系

| 播放器阶段 | 依赖的 IOTSdk 接口 |
|------------|-------------------|
| `Search` | `SearchSongEx`（见 [IOTSdk.md](./IOTSdk.md) 第 4.6 节）；请求体含 `text` 与 `searchRange` |
| `Play`（无 `listenUrl` 时） | `GetMusicInfo`（第 4.8 节） |
| `Play` 封面 / 歌词 | `Config::on_get_board_http(IOTSDK_MIGU_HTTPS)` |
| `Play` 拉流 | `Config::on_get_board_http(IOTSDK_MIGU_MUSIC)` |
| `Play` 正常结束且 `reportOnQuit: true` | `Report`（第 4.7 节） |

搜索结果中每条曲目常用字段（`EspMusicPlayer` / `MusicInfo`）：`musicId`、`musicName`、`singerName`、`listenUrl`、`picUrl`、`lrcUrl`、`length`（`HH:mm:ss`）等，与 `SearchSongEx` 的 `data.searchSong` 条目一致。

---

## 6. ESP-IDF 编译事项

集成预编译静态库 **`migumusic.a`** 或工程内 `iotsdk` 组件时，除完成 [IOTSdk 初始化](./IOTSdk.md) 外，还须满足下列 **ESP-IDF 依赖与 Kconfig** 要求。

### 6.1 依赖组件

`EspMusicPlayer`、`EspAudioDec`、`EspOpusDec` 等源文件在链接时需要以下组件（与 [`projects/esp32s3-idf-v5.5.2/components/iotsdk/CMakeLists.txt`](../../projects/esp32s3-idf-v5.5.2/components/iotsdk/CMakeLists.txt) 中 `REQUIRES` 一致）：

| 组件名 | 说明 |
|--------|------|
| `json` | ESP-IDF 自带 `json` 组件（cJSON） |
| `mbedtls` | ESP-IDF 自带 TLS/加解密 |
| `esp_http_client` | ESP-IDF 自带 HTTP 客户端 |
| `esp_audio_codec` | Component Manager：`espressif/esp_audio_codec`（MP3/Opus 等解码） |
| `esp_audio_effects` | Component Manager：`espressif/esp_audio_effects` |

**`iotsdk` 组件 `CMakeLists.txt` 示例：**

```cmake
idf_component_register(
    SRCS ...
    INCLUDE_DIRS ...
    REQUIRES json mbedtls esp_http_client esp_audio_codec esp_audio_effects
)
```

**`main/idf_component.yml` 中需声明托管组件（若 `main` 直接依赖 iotsdk）：**

```yaml
dependencies:
  espressif/esp_audio_codec: "*"
  espressif/esp_audio_effects: "*"
```

集成方 `main` 的 `CMakeLists.txt` 中 `PRIV_REQUIRES` / `REQUIRES` 须能传递上述依赖（链接预编译 **`migumusic.a`** 时尤须显式加入 `esp_audio_codec`、`esp_audio_effects`）。

### 6.2 mbedTLS：`CONFIG_MBEDTLS_DES_C=y`

IOTSdk 业务加解密路径使用 **DES**，须在工程中开启该选项，否则可能链接失败或 HTTPS/协议层异常。

| 配置项 | 要求值 |
|--------|--------|
| `CONFIG_MBEDTLS_DES_C` | `y`（必选） |

**方式一：`idf.py menuconfig`**

路径：`Component config` → `mbedTLS` → 勾选 **DES module**（界面文案可能为 “Enable DES module”）。

**方式二：`sdkconfig.defaults`**

在工程根目录 `sdkconfig.defaults` 增加一行（参考 `projects/esp32s3-idf-v5.5.2/sdkconfig.defaults`）：

```ini
CONFIG_MBEDTLS_DES_C=y
```

修改后执行 `idf.py fullclean` 再 `idf.py build`，或在 menuconfig 中保存一次以写入 `sdkconfig`。

---

## 7. 集成注意点（摘要）

1. **`IOTSdk::SetOnGetBoardHttp`（Init 前）→ `IOTSdk::Init` → `IMusicPlayer::Init` → 回调 → `Search` / `Play`**。
2. **`SetOnFinished`、`SetOnPcmReady` 为必选**；未设置时行为未定义。
3. **`SetOnStarted`、`SetOnCoverImageReady`、`SetOnProgress` 为可选**；封面/歌词/拉流依赖 `SetOnGetBoardHttp`；歌词与进度需 `SetOnCoverImageReady` 注册且 `lrcUrl` 有效。
4. **`SetAudioSink` 必须在 `Play` 前调用**，否则 `IsAvailable()` 为 `false`。
5. **`Search` 与 `Play` 分离**：`Search` 只拉列表，`Play` 才真正开播；`Quit` 会清空列表（实现相关）。`text` 为必填搜索原文，可与 `songName`/`singerName`/`tagName` 组合填入 `searchRange`。
6. **队列查询**：`GetSongList()` 返回 JSON 字符串。
7. **听歌上报**：`EspMusicPlayer` 仅在 `Init` 配置 `reportOnQuit: true` 且曲目以 `EMP_RET_OK` / `EMP_RET_EOS` 结束时调用 `Report`；若需自定义起止时间或批量上报，请自行调用 `IOTSdk::Report` 或换实现类。
8. **错误处理**：接口返回非 `EMP_RET_OK` 时，配合 `GetError()` / `GetMessage()` 打日志或提示用户。
9. **线程安全**：`OnStarted` / `OnPcmReady` / `OnCoverImageReady` / `OnProgress` / `OnFinished` 在播放任务线程执行，向 UI 或主循环投递时请使用队列/Schedule 等方式切换线程。
10. **封面内存**：`OnCoverImageReady` 收到的 `data` 在回调返回后会被释放，显示层须自行拷贝（如 `SetMusicCoverImage` 内分配缓冲）。
11. **音源协调**：与 TTS/对话共用 `AudioService` 时，开播前调用等效的 `EnterMusicPlaybackMode()`，结束或 `Quit` 时调用 `ExitMusicPlaybackMode()`（见 [8.3](#83-音源协调entermusicplaybackmode--exitmusicplaybackmode)）。
12. **设备状态监听**：通过 `AddStateChangeListener` 在连接云端或 TTS 播报时 `ToggleQuit()` 打断音乐（见 [8.4](#84-设备状态监听addstatechangelistener)）。
13. **编译依赖**：满足 [第 6 节](#6-esp-idf-编译事项) 中的组件依赖与 `CONFIG_MBEDTLS_DES_C=y`。

---

## 8. 接口使用示例

以下示例借鉴 [xiaozhi-esp32-minimal](https://github.com/78/xiaozhi-esp32) 工程中的板级封装与 MCP 接入方式，路径均相对于该仓库：

| 文件 | 作用 |
|------|------|
| `main/boards/common/music.h` | 板级音乐抽象（`Play` / `Quit` / `Next` / `Previous` / `GetSongList`） |
| `main/boards/common/esp32_music.h` / `.cc` | `Esp32Music`：对接 `IMusicPlayer` + `AudioService` |
| `main/boards/common/esp32_music2.h` / `.cc` | `Esp32Music2`：封面/进度/UI 回调示例 |
| `main/boards/common/board.cc` | `Board::GetMusic()` 返回单例 `Esp32Music` |
| `main/mcp_server.cc` | MCP 工具 `self.music.*` 调用 `Music` 接口 |

| 小节 | 内容 |
|------|------|
| [8.1](#81-直接使用-imusicplayer最低层) | 直接调用 `IMusicPlayer` |
| [8.2](#82-板级封装music--esp32music推荐) | `Music` / `Esp32Music` 封装 |
| [8.3](#83-音源协调entermusicplaybackmode--exitmusicplaybackmode) | `EnterMusicPlaybackMode` / `ExitMusicPlaybackMode` |
| [8.4](#84-设备状态监听addstatechangelistener) | `AddStateChangeListener` 打断播放 |
| [8.5](#85-切歌next--previous) | `Next` / `Previous` |
| [8.6](#86-mcp-工具注册mcp_servercc) | MCP `self.music.*` |
| [8.7](#87-集成检查清单) | 集成检查清单 |

---

### 8.1 直接使用 `IMusicPlayer`（最低层）

适用于不经过 `Music` 抽象、自行处理 PCM 与 HTTP 的场景。

```cpp
#include "IOTSdk.h"
#include "IMusicPlayer.h"

// 1. 板载 HTTP（须在 Init 之前），见 IOTSdk.md
IOTSdk::Singleton().SetOnGetBoardHttp([](int action) -> std::unique_ptr<Http> {
    return Board::GetInstance().GetNetwork()->CreateHttp(0);
});
IOTSdk::Singleton().Init("./", init_args_json);

// 2. 获取播放器并初始化配置
IMusicPlayer* player = IMusicPlayer::GetInstance();

const char* player_config = R"({
  "reportOnQuit": true,
  "httpReadTimeoutMs": 8000
})";
player->Init(player_config);

player->SetAudioSink(16000, 1);  // 与 AudioCodec 输出一致

player->SetOnStarted([](const char* songName, const char* singerName) {
    ESP_LOGI("demo", "started: %s - %s", songName ? songName : "", singerName ? singerName : "");
});

player->SetOnFinished([](int code, const char* message) {
    ESP_LOGI("demo", "finished code=%d msg=%s", code, message ? message : "");
});

player->SetOnPcmReady([](char* pcm, int length) {
    // 将 PCM 送入扬声器队列（勿在回调内长时间阻塞）
    // audio_service->PushPcmForPlayback(...);
});

player->SetOnCoverImageReady([](char* data, int datalen) {
    // 须在回调内拷贝或解码；返回后 data 会被释放
    // display->SetMusicCoverImage(data, datalen);
});

player->SetOnProgress([](int current_ms, int total_ms,
                         const char* prev, const char* curr, const char* next) {
    // display->SetMusicProgress(current_ms, total_ms);
    // display->SetMusicLyric(prev, curr, next);
});

// 3. 搜索并播放（text 为搜索原文，可与 songName/singerName/tagName 同时传入）
int rc = player->Search("陈奕迅 十年", "十年", "陈奕迅", nullptr);
if (rc != EMP_RET_OK) {
    ESP_LOGE("demo", "Search failed: %s", player->GetMessage());
    return;
}

// 可选：查看队列
std::string queue_json = player->GetSongList();
ESP_LOGI("demo", "queue: %s", queue_json.c_str());

rc = player->Play(0);  // 播放列表第一首
if (rc != EMP_RET_OK) {
    ESP_LOGE("demo", "Play failed: %s", player->GetMessage());
}

// 4. 停止并释放列表
player->Quit();
```

---

### 8.2 板级封装：`Music` + `Esp32Music`（推荐）

小智工程将 `IMusicPlayer` 藏在 `Esp32Music` 内，对外只暴露简化的 `Music` 接口。

**抽象接口**（`music.h`）：

```cpp
class Music {
public:
    virtual ~Music() = default;
    virtual bool Play(const char* text, const char* songName, const char* singerName, const char* tagName) = 0;
    virtual void Quit() = 0;
    virtual void ToggleQuit() = 0;
    virtual bool Next() = 0;
    virtual bool Previous() = 0;
    virtual bool IsPlaying() const = 0;
    virtual std::string GetSongList() = 0;
};
```

| 方法 | 说明 |
|------|------|
| `Play(text, songName, singerName, tagName)` | 内部 `Search` + `Play(0)`；`text` 为用户搜索原文（必填） |
| `GetSongList()` | 转发 `IMusicPlayer::GetSongList()`；返回队列 JSON 字符串 |
| 其余 | 与 `IMusicPlayer` 同名方法语义一致，返回类型多为 `bool`（由错误码转换） |

**构造时绑定 `IMusicPlayer` 回调**（参考 `esp32_music2.cc` / `esp32_music.cc`；设备状态监听见 [8.4](#84-设备状态监听addstatechangelistener)）：

```cpp
// IOTSdk::Init 之后注册板载 HTTP（非 IMusicPlayer 接口）
IOTSdk::Singleton().SetOnGetBoardHttp([](int action) -> std::unique_ptr<Http> {
    return Board::GetInstance().GetNetwork()->CreateHttp(0);
});

Esp32Music2::Esp32Music2() : music_player_(IMusicPlayer::GetInstance()) {
    // AddStateChangeListener：见 8.4，在 Connecting / Speaking 时 ToggleQuit()

    music_player_->SetOnStarted([this](const char* songName, const char* singerName) {
        Application::GetInstance().Schedule([this]() {
            Display* d = Board::GetInstance().GetDisplay();
            d->SetMusicNowPlaying(music_player_->GetSongName(), music_player_->GetSingerName());
        });
    });

    music_player_->SetOnFinished([this](int code, const char* message) {
        Application::GetInstance().Schedule([this]() { /* ToggleQuit / ExitMusicPlaybackMode */ });
    });

    music_player_->SetOnProgress([](int current_ms, int total_ms,
                                  const char* prev, const char* curr, const char* next) {
        Display* display = Board::GetInstance().GetDisplay();
        display->SetMusicProgress(current_ms, total_ms);
        display->SetMusicLyric(prev, curr, next);
    });

    music_player_->SetOnPcmReady([this](char* pcm, int length) {
        // 拷贝 PCM → AudioCodec / AudioService
    });

    music_player_->SetOnCoverImageReady([](char* data, int datalen) {
        Board::GetInstance().GetDisplay()->SetMusicCoverImage(data, datalen);
    });

    auto codec = Board::GetInstance().GetAudioCodec();
    music_player_->SetAudioSink(codec->output_sample_rate(), codec->output_channels());
}
```

**`Play`：先 `Search` 再 `Play`，并进入“音乐模式”**：

```cpp
bool Esp32Music::Play(const char* text, const char* songName, const char* singerName, const char* tagName) {
    if (music_player_->Search(text, songName, singerName, tagName) != EMP_RET_OK) {
        return false;
    }
    if (music_player_->Play() == EMP_RET_OK) {
        EnterMusicPlaybackMode();  // 详见 8.3
        return true;
    }
    ExitMusicPlaybackMode();
    return false;
}

std::string Esp32Music::GetSongList() {
    return music_player_->GetSongList();
}
```

`Quit()` 中同时调用 `music_player_->Quit()` 与 `ExitMusicPlaybackMode()`。`ToggleQuit()` 仅转发到 `IMusicPlayer`，不退出音乐模式（由 `OnFinished` 或显式 `Quit` 收尾）。

**板级获取**（`board.cc`）：

```cpp
Music* Board::GetMusic() {
    static Esp32Music music;
    return &music;
}
```

**业务侧调用**：

```cpp
Music* music = Board::GetInstance().GetMusic();
music->Play("我想听陈奕迅的十年", "十年", "陈奕迅", nullptr);
music->Quit();

std::string queue_json = music->GetSongList();
// 使用 queue_json ...
```

---

### 8.3 音源协调：`EnterMusicPlaybackMode` / `ExitMusicPlaybackMode`

`IMusicPlayer` 只负责搜歌、拉流、解码 PCM；在小智类设备上，**扬声器通常还与云端 TTS、麦克风上行、Opus 解码队列共用 `AudioService`**。若不在开播前释放对话通道，容易出现 TTS 与音乐抢喇叭、或残留 Opus 帧与 PCM 混在一起。

**推荐调用关系：**

```text
Play/Next/Previous 成功
    → EnterMusicPlaybackMode()     // 抢占喇叭与播放队列
    → OnPcmReady → PushPcmForPlayback

OnFinished / Quit / Play 失败
    → ToggleQuit 或 Quit（按需）
    → ExitMusicPlaybackMode()      // 恢复上行与对话能力
```

`Esp32Music` 在 `esp32_music.h` 中声明两个 **protected** 方法（集成方可照抄或子类化），用于在「音乐独占播放」与「语音对话」之间切换：

| 方法 | 调用时机 | 作用 |
|------|----------|------|
| `EnterMusicPlaybackMode()` | `Play` / `Next` / `Previous` 成功启动播放任务后 | 抢占音源，让音乐 PCM 独占输出路径 |
| `ExitMusicPlaybackMode()` | 播放失败、`Quit`、或 `OnFinished` 收尾时 | 恢复语音上行与对话能力 |

**`EnterMusicPlaybackMode()` 典型步骤**（`esp32_music.cc`）：

```cpp
void Esp32Music::EnterMusicPlaybackMode() {
    if (is_playing_mode) return;
    is_playing_mode = true;
    auto& app = Application::GetInstance();
    audio_service_ = &app.GetAudioService();

    app.AbortSpeaking(kAbortReasonNone);           // 打断正在播报的 TTS
    app.StopListening();                             // 停止拾音 / 监听
    app.CloseAudioChannelIfOpened();                 // 关闭 MQTT/WS 音频通道
    audio_service_->EnableVoiceProcessing(false);    // 关闭上行语音处理（AFE 等）
    audio_service_->ResetDecoder();                  // 清空 Opus 解码/播放/测试队列
    audio_service_->WaitForPlaybackQueueEmpty();     // 等待已有播放队列播完，避免与音乐叠音
}
```

**`ExitMusicPlaybackMode()` 典型步骤**：

```cpp
void Esp32Music::ExitMusicPlaybackMode() {
    if (!is_playing_mode) return;
    is_playing_mode = false;
    audio_service_->EnableVoiceProcessing(true);     // 恢复上行，便于再次对话/唤醒
}
```

**与 `IMusicPlayer` 回调的配合**：

| 场景 | 建议 |
|------|------|
| `Play()` / `Next()` / `Previous()` 返回成功 | 调用 `EnterMusicPlaybackMode()` |
| `Play()` 等返回失败 | 调用 `ExitMusicPlaybackMode()`（未真正开播） |
| `OnFinished`（播完 / 取消 / 错误） | `ToggleQuit()` 后 `ExitMusicPlaybackMode()` |
| `Quit()` | `music_player_->Quit()` 后 **务必** `ExitMusicPlaybackMode()` |

**`OnPcmReady` 与 `audio_service_`**：`EnterMusicPlaybackMode` 内会缓存 `audio_service_` 指针，随后在 `SetOnPcmReady` 中调用 `PushPcmForPlayback` 送入同一播放队列（与 TTS 解码路径分离后的 PCM 队列）。

**自行集成时**：若不用 `Esp32Music`，也应在 `SetOnPcmReady` 之前完成等效的「停 TTS、关通道、清队列、关上行」逻辑，否则仅需保证 PCM 写入与现有 `AudioCodec` 采样率一致（见 `SetAudioSink`）。

---

### 8.4 设备状态监听：`AddStateChangeListener`

用户按键唤醒、云端开始连接或进入 TTS 播报时，应 **主动打断音乐**，避免与对话同时进行。`Esp32Music` 在构造函数里向 `DeviceStateMachine` 注册监听：

```cpp
app.GetDeviceStateMachine().AddStateChangeListener(
    [this](DeviceState prev, DeviceState curr) {
        if (!music_player_->IsPlaying()) {
            return;
        }
        // 即将连接云端或开始播报：打断当前曲目（不释放搜索列表）
        if (curr == kDeviceStateConnecting || curr == kDeviceStateSpeaking) {
            ToggleQuit();
        }
    });
```

| 新状态 `curr` | 建议动作 | 说明 |
|---------------|----------|------|
| `kDeviceStateConnecting` | `ToggleQuit()` | 用户发起新一轮对话，停止音乐以免占带宽/喇叭 |
| `kDeviceStateSpeaking` | `ToggleQuit()` | 云端 TTS 开始，必须让出下行 |
| 其他 | 视产品而定 | 例如 `kDeviceStateListening` 是否打断由业务决定 |

**`ToggleQuit` 与 `Quit` 的选择**：

- 状态监听里多用 **`ToggleQuit()`**：只打断当前播放任务，**保留** `Search` 得到的列表，用户对话结束后仍可能 `Next()`。
- 用户明确「停止播放」或 MCP `self.music.stop` 使用 **`Quit()`**：停止并清空列表，同时应 **`ExitMusicPlaybackMode()`**。

**集成要点**：

1. 监听注册宜在 **单例初始化阶段**（如 `Esp32Music` 构造函数）完成一次即可。
2. 回调内避免阻塞；若需改 UI/设备态，使用 `Application::Schedule` 投递到主循环。
3. 与 `EnterMusicPlaybackMode` 成对设计：进入对话态打断音乐，音乐播完或 `Quit` 时 `ExitMusicPlaybackMode` 恢复语音能力。

---

### 8.5 切歌：`Next` / `Previous`

`Esp32Music` 要求 **列表仍有效且当前没有播放任务**（上一首需已结束或被 `ToggleQuit` 打断）：

```cpp
bool Esp32Music::Next() {
    if (music_player_->IsAvailable() && !music_player_->IsPlaying()) {
        if (music_player_->Next() == EMP_RET_OK) {
            EnterMusicPlaybackMode();
            return true;
        }
        ExitMusicPlaybackMode();
    }
    return false;
}
```

**典型用法：**

```cpp
Music* music = Board::GetInstance().GetMusic();

// 用户说「下一首」：若仍在播放，先打断再切
if (music->IsPlaying()) {
    music->ToggleQuit();
    // 可轮询或等待 OnFinished 后再 Next
}
music->Next();
```

> `Quit()` 会清空 `IMusicPlayer` 内部列表，之后 `Next` / `Previous` 无效，需重新 `Play(text, songName, singerName, tagName)`（内部会再次 `Search`）。

---

### 8.6 MCP 工具注册（`mcp_server.cc`）

在 `McpServer::AddCommonTools()` 中，若 `board.GetMusic()` 非空，注册云端可控的播放工具（`xiaozhi-esp32-minimal` 当前实现）：

```cpp
auto music = board.GetMusic();
if (music) {
    AddTool("self.music.play_online", "...",
        PropertyList({
            Property("text", kPropertyTypeString),                              // 必填
            Property("singerName", kPropertyTypeString, std::string("")),
            Property("songName", kPropertyTypeString, std::string("")),
            Property("tagName", kPropertyTypeString, std::string(""))
        }),
        [music](const PropertyList& properties) -> ReturnValue {
            auto text = properties["text"].value<std::string>();
            auto singer = properties["singerName"].value<std::string>();
            auto song = properties["songName"].value<std::string>();
            auto tag = properties["tagName"].value<std::string>();
            return music->Play(text.c_str(), song.c_str(), singer.c_str(), tag.c_str());
        });

    AddTool("self.music.stop", "...", PropertyList(),
        [music](const PropertyList&) -> ReturnValue {
            music->Quit();
            return true;
        });

    AddTool("self.music.next", "...", PropertyList(),
        [music](const PropertyList&) -> ReturnValue {
            return music->Next();   // 返回 bool
        });

    AddTool("self.music.prev", "...", PropertyList(),
        [music](const PropertyList&) -> ReturnValue {
            return music->Previous();
        });

    AddTool("self.music.set_play_mode", "...",
        PropertyList({ Property("repeat_mode", ...), Property("shuffle", ...) }),
        [](const PropertyList&) -> ReturnValue {
            return false;   // minimal 工程占位，未接在线队列
        });

    AddTool("self.music.get_queue", "...",
        PropertyList(),
        [music](const PropertyList&) -> ReturnValue {
            return music->GetSongList();   // 返回 std::string（JSON 文本）
        });
}
```

| MCP 工具 | 对应调用 | 返回值 | 说明 |
|----------|----------|--------|------|
| `self.music.play_online` | `Music::Play(text, songName, singerName, tagName)` | `bool` | **`text` 必填**（用户原话，如「我想听陈奕迅的十年」）；`songName`/`singerName`/`tagName` 可选，用于 `searchRange` |
| `self.music.stop` | `Music::Quit()` | `true` | 停止播放、清空列表并 `ExitMusicPlaybackMode` |
| `self.music.next` | `Music::Next()` | `bool` | 列表有效且当前未在播放时可切下一首（约 10 首缓存列表） |
| `self.music.prev` | `Music::Previous()` | `bool` | 同上，上一首 |
| `self.music.set_play_mode` | （未实现） | `false` | minimal 工程占位；循环/随机模式需在线队列方案时再对接 |
| `self.music.get_queue` | `Music::GetSongList()` | `std::string` | 返回队列 JSON 文本（`active`、`songCount`、`songs` 等），无列表时为 `"{}"` |

**`play_online` 参数顺序（MCP → `Music::Play`）：**

```text
text → songName → singerName → tagName
```

与 `IMusicPlayer::Search(text, songName, singerName, tagName)` 一致。

---

### 8.7 集成检查清单

1. `IOTSdk::SetOnGetBoardHttp` → `IOTSdk::Init` → `IMusicPlayer::Init`（可为 `nullptr` / `"{}"`）→ 回调 → `Search` / `Play`。
2. [第 6 节](#6-esp-idf-编译事项)：`REQUIRES` 含 `json`、`mbedtls`、`esp_http_client`、`esp_audio_codec`、`esp_audio_effects`，且 `CONFIG_MBEDTLS_DES_C=y`。
3. `SetAudioSink` 采样率、声道与 `AudioCodec` 一致（如 16000 Hz、单声道）。
4. 必须设置 `SetOnFinished`、`SetOnPcmReady`；按需 `SetOnStarted`、`SetOnCoverImageReady`（封面+歌词）、`SetOnProgress`。
6. `Search(text, ...)` / `Music::Play` / MCP `play_online`：`text` 为用户搜索原文（头文件与 MCP 均为必填）。
7. `GetSongList()` 返回 JSON 字符串，可直接用于 MCP `get_queue` 或日志输出。
8. 开播成功调用 **`EnterMusicPlaybackMode()`**，停止/播完/失败调用 **`ExitMusicPlaybackMode()`**（[8.3](#83-音源协调entermusicplaybackmode--exitmusicplaybackmode)）。
9. 注册 **`AddStateChangeListener`**，在 `Connecting` / `Speaking` 等态对正在播放的音乐 **`ToggleQuit()`**（[8.4](#84-设备状态监听addstatechangelistener)）。
10. MCP / UI 切歌前确认 `IsPlaying()`，必要时 `ToggleQuit()` 再 `Next()`。
11. 完整停止并清列表用 `Quit()`（含 `ExitMusicPlaybackMode`），快速打断当前曲用 `ToggleQuit()`。
12. `EspMusicPlayer` 仅在 `Init` 中设置 `"reportOnQuit": true` 时于播完/正常 `Quit` 上报 `Report`；取消/错误结束不会上报。
13. MCP `set_play_mode` 在 `xiaozhi-esp32-minimal` 当前为占位，勿依赖其生效。

---

## 9. 文档维护

- 规范来源：[`IMusicPlayer.h`](../IMusicPlayer.h)。
- 默认实现参考：[`EspMusicPlayer.cpp`](../EspMusicPlayer.cpp)。
- 使用示例参考：`xiaozhi-esp32-minimal` 的 `music.h`、`esp32_music.cc`、`esp32_music2.cc`、`mcp_server.cc`。
- 若头文件中的错误码、接口签名或回调约定有变更，请同步更新本文档。
