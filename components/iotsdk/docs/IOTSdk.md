# IOTSdk 对接接口说明

本文档依据源码头文件 [`IOTSdk.h`](../IOTSdk.h) 整理，供第三方在集成 **IOTSdk** 时对照使用。入参、出参均为 **JSON 字符串**。

---

## 1. 错误码宏

| 宏 | 值 | 含义 |
|----|-----|------|
| `IOTSDK_RET_OK` | `0` | 成功 |
| `IOTSDK_RET_ERROR` | `-1` | 未知错误 |
| `IOTSDK_RET_INVALID_PARAMS` | `-2` | 无效参数 |
| `IOTSDK_RET_RESOURCE_NOT_READY` | `-3` | 资源未就绪 |
| `IOTSDK_RET_REQUEST_EXCEPTION` | `-4` | 请求异常 |
| `IOTSDK_RET_INVALID_REQUEST` | `-5` | 请求不支持或无效 |
| `IOTSDK_RET_TIMEOUT` | `-6` | 请求超时 |
| `IOTSDK_RET_INVALID_DEVICE_ID` | `-7` | 设备 ID 为空 |
| `IOTSDK_RET_UNPLAYABLE` | `-8` | 不可播放（`VIP`、`isCpAuth` 不满足播放条件） |

接口注释中“`@return 0 成功，其他失败`”与上述错误码约定一致；具体失败时返回哪一个负值，以实现为准。

`IOTSDK_RET_UNPLAYABLE` 常见于调用 `GetMusicInfo` 成功后校验曲目可播性：`musicInfo` 中 `isCpAuth`、`VIP` 须均为 `"1"`，且存在有效 `listenUrl`；否则视为不可播放（参见 `MusicInfo::Playable`）。

---

## 2. 内容提供商（provider）

| 宏 | 字符串值 | 说明 |
|----|-----------|------|
| `IOTSDK_PROVIDER_MIGU` | `"migu"` | 咪咕 |
| `IOTSDK_PROVIDER_XMLY` | `"xmly"` | 喜马拉雅 |

业务 JSON 中的 `provider` 字段应使用上表中的字符串值（可直接使用宏对应的字面量）。

---

## 3. HTTP 连接意图（action）

通过 `SetOnGetBoardHttp` 注册板载 HTTP 工厂时（**4G 板卡必选，WiFi 板卡通常省略**），SDK 会按 **action** 区分连接用途；回调须返回 [`BoardHttp`](../IOTBoard.h) 实现实例。

| 宏 | 值 | 说明 |
|----|-----|------|
| `IOTSDK_MIGU_HTTPS` | `1` | 咪咕 HTTPS API 接口 |
| `IOTSDK_MIGU_MUSIC` | `2` | 咪咕 HTTPS 音乐流 |
| `IOTSDK_OPENAPI_HTTPS` | `3` | IOTSdk OpenAPI 接口 |

实现侧示例：`IOTHttpClient_board` 在访问咪咕 API 时使用 `IOTSDK_MIGU_HTTPS`，访问 OpenAPI 时使用 `IOTSDK_OPENAPI_HTTPS`；`EspMusicPlayer` 拉流时使用 `IOTSDK_MIGU_MUSIC`。

---

## 4. C++ 接口：`class IOTSdk`

单例获取：

```cpp
IOTSdk& IOTSdk::Singleton();
```

以下方法均在 `IOTSdk` 上调用（具体实现由工程提供）。板载 HTTP 相关类型为 [`BoardHttp`](../IOTBoard.h)（由 `IOTSdk.h` 通过 `#include "IOTBoard.h"` 引入）。

### 4.1 `Init`

```cpp
virtual int Init(const std::string& workspace, const std::string& args) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `workspace` | `std::string` | 工作目录 |
| `args` | `std::string` | 初始化参数，**JSON 字符串** |

**`args` JSON 字段说明（摘自头文件注释）：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `deviceId` | 是 | 设备唯一标识（通常为 WiFi STA 的 MAC 地址，格式如 `88:56:a6:e8:88:60`） |
| `appLicenseId` | 是 | 许可证 ID |
| `appKey` | 是 | 区域编码 |
| `serverToken` | 是 | 服务端 token |
| `regionCode` | 是 | 区域编码 |
| `servicePackageCode` | 是 | 服务套餐码 |
| `env` | 否 | 环境：`prod`（生产）、`test`（测试） |

**`deviceId` 获取指引：**

1. **小智设备**：可从设备串口日志中获取 WiFi STA 模式的 MAC 地址，作为 `deviceId`。日志示例：

   ```
   I (615) wifi:mode : sta (88:56:a6:e8:88:60)
   ```

   其中括号内的 `88:56:a6:e8:88:60` 即为设备唯一标识，填写到 `args` 的 `deviceId` 字段。

**返回值：** `0` 表示成功，非 `0` 表示失败（见第 1 节错误码）。

---

### 4.2 `SetOnGetBoardHttp`

```cpp
virtual void SetOnGetBoardHttp(
    std::function<std::unique_ptr<BoardHttp>(int action)> on_get_board_http) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `on_get_board_http` | 回调 | 按 **action**（见第 3 节）创建板载 `BoardHttp` 实例；返回 `nullptr` 表示无法提供实例 |

**说明（摘自头文件）：**

1. 须在 **`Init()` 之前** 注册（若需要注册）。
2. `BoardHttp` 实现可参考 [`EspBoard.cpp`](../EspBoard.cpp)（`BoardHttpImpl` 包装板载网络栈的 `Http`）。
3. **板载 4G 模组**：**必须**调用该函数设置回调，否则无法使用网络功能。
4. **板载 WiFi 模组**：**不建议**调用该函数设置回调，更节省内存、性能更优。

若已注册该回调，`EspMusicPlayer` 封面下载、歌词下载、音乐拉流均通过该回调按 `action` 创建 `BoardHttp` 实例。

**回调示例（4G 板卡）：**

```cpp
IOTSdk::Singleton().SetOnGetBoardHttp([](int action) -> std::unique_ptr<BoardHttp> {
    return std::make_unique<BoardHttpImpl>();
});
IOTSdk::Singleton().Init(workspace, init_args_json);
```

---

### 4.3 `SearchPlay`

```cpp
virtual int SearchPlay(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | 请求参数，**JSON 字符串** |
| `output` | `std::string&` | 返回结果，**JSON 字符串**（由实现写入） |
| `timeout` | `int` | 超时时间，单位 **毫秒**，默认 `10000` |

**`input` JSON 字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `provider` | 是 | 内容供应商，见第 2 节 |
| `content` | 是 | 查询用文本内容 |

**`output` 中 MusicInfo 结构（头文件分供应商描述）：**

- **咪咕（migu）** 字段示例：`musicId`, `musicName`, `bpm`, `singerName`, `albumNames`, `songAuthorName`, `lyricAuthorName`, `length`, `language`, `picUrl`, `listenUrl`, `hqListenUrl`, `sqListenUrl`, `lrcUrl`, `isCollection`, `isCpAuth`, `singerId`, `musicSource`, `auditionsFlag`, `VIP`, `contentId`, `isUserCollection`, `listenFlag` 等（含义以头文件注释为准）。
- **喜马拉雅（xmly）** 字段示例：`trackId`, `coverUrl`, `playUrl`（加密）, `playSize`, `duration`, `positionIndex`, `hasNext`, `title`, `albumName`, `artists` 等。

**返回值：** `0` 成功，其他失败。

---

### 4.4 `SearchByKey`

咪咕关键字搜歌（NEW），对应 HTTP `/{api_version}/search/music/new/listbykey`（实现以工程为准）。

```cpp
virtual int SearchByKey(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | 请求参数，**JSON 字符串** |
| `output` | `std::string&` | 返回结果，**JSON 字符串**（由实现写入） |
| `timeout` | `int` | 超时时间，单位 **毫秒**，默认 `10000` |

**`input` JSON 字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `provider` | 是 | 仅支持 **`migu`** |
| `text` | 是 | 搜索内容；当 `type=1` 且 `searchType=3`，或 `type=7` 时，`text` 为歌手 id |
| `type` | 否 | 搜索目的，默认 `1`：`1` 歌曲、`2` 专辑、`3` 歌手、`4` 标签下歌曲、`5` 无维度、`6` 联想、`7` 歌手下单曲/专辑/MV、`8` 歌词 |
| `pageIndex` | 否 | 当前页（起始为 1） |
| `pageSize` | 否 | 每页条数，`[0-50]` |
| `searchType` | 否 | `type` 为 `1` 或 `5` 时有效：`1` 智能、`2` 关键词、`3` 歌手下歌曲、`4` 指定范围搜索等（见《咪咕 API 接口规范》第 8 节） |
| `issemantic` | 否 | `type` 为 `1/2/3/4` 时有效：`1` 语义、`0` 否 |
| `isCorrect` | 否 | `type` 为 `1/2/3/4/5` 时有效：`0` 关、`1` 开 |
| `searchRange` | 否 | JSON **对象**，`type=1` 且 `searchType=4` 时精确搜索范围（意图），见接口文档 |

**`output` JSON 结构（以头文件注释为准）：**

- **`code`**：由接口 `resCode` 映射。
- **`message`**：由接口 `resMsg` 映射。
- **业务数据对象**：随请求 `type` 不同，主要业务数据落在对应根字段（其余字段可能为空）：
  - **`searchSong`**（`SearchSongNewRsp`）：`type=1` 搜歌曲。
  - **`searchAlbum`**（`SearchAlbumNewRsp`）：`type=2` 搜专辑。
  - **`searchSinger`**（`SearchSingerNewRsp`）：`type=3` 搜歌手。
  - **`searchTag`**（`SearchSongNewRsp`）：`type=4` 标签下歌曲（与 `searchSong` 同类结构）。
  - **`searchAll`**（`SearchAllNewRsp`）：`type=5` 无维度搜索。
  - **`searchSuggest`**（`SearchSuggestNewRsp`）：`type=6` 联想搜索。
  - **`searchSingerSAM`**（`SearchSingerSAMNewRsp`）：`type=7` 歌手下单曲/专辑/MV。
  - **`searchLyric`**（`SearchLyricNewRsp`）：`type=8` 歌词搜歌。

各 `Search*NewRsp` 内层字段（如 `rc`、`error`、`correct`、`semantic`、`data.total`、`data.result`、`resultType`、`SongNew`、`AlbumNew`、`ToneNew` 等）与头文件 [`IOTSdk.h`](../IOTSdk.h) 中 **`SearchByKey`** 注释一致；也可对照《咪咕 API 接口规范》第 8 节。

**返回值：** `0` 成功，其他失败。

---

### 4.5 `SearchSongEx`

咪咕搜歌曲扩展接口。

```cpp
virtual int SearchSongEx(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | 请求参数，**JSON 字符串** |
| `output` | `std::string&` | 返回结果，**JSON 字符串**（由实现写入） |
| `timeout` | `int` | 超时时间，单位 **毫秒**，默认 `10000` |

**`input` JSON 字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `provider` | 是 | 仅支持 **`migu`** |
| 其余字段 | — | 与下文 **`Search`** 的「咪咕扩展」及通用 `text` 相同（见第 4.6 节） |

**`output` JSON：**

- `code` / `message` / `traceId` / `responseSign`
- `data.searchSong`：歌曲列表（数组）；字段含义同第 4.6 节 **`Search`** 返回结果中的 `searchSong`

**返回值：** `0` 成功，其他失败。

---

### 4.6 `Search`

喜马拉雅 + 咪咕**合并内容搜索**。调用 OpenAPI `/openapi/iot/content/search`：服务端按 `providerOrder` 编排供应商搜索顺序（未传时通常先喜马后咪咕）。SDK 将顶层扁平入参拆分为 `xmly` / `migu` 分区提交，再把命中分区提升为与 `SearchSongEx` 相近的 `data` 结构返回。

详细字段与编排逻辑可参见 [`喜马拉雅+咪咕合并内容查询-接口设计.md`](./喜马拉雅+咪咕合并内容查询-接口设计.md)。

```cpp
virtual int Search(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | 请求参数，**JSON 字符串**（扁平传入，勿再嵌套 `xmly`/`migu`；由 SDK 内部组装） |
| `output` | `std::string&` | 返回结果，**JSON 字符串**（由实现写入） |
| `timeout` | `int` | 超时时间，单位 **毫秒**，默认 `10000` |

**`input` JSON 字段：**

**通用：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `text` | 是 | 搜索内容；映射为咪咕 `migu.text`，同时作为喜马 `xmly.content`；当 `type=1` 且 `searchType=3`，或 `type=7` 时，`text` 为歌手 id |
| `providerOrder` | 否 | 内容供应商**搜索顺序**，`string` 数组；元素为 `"xmly"` / `"migu"`，按数组先后决定服务端编排优先级。示例：`["xmly","migu"]` 先喜马后咪咕；`["migu"]` 仅搜咪咕。未传时由服务端默认编排（通常先喜马后咪咕）。SDK 将其置于请求体顶层透传 |

**喜马扩展（可选，进入 `xmly` 分区）：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `intent` | 否 | 意图：`listen_audiobook`（有声书，**默认**）、`listen_music`（音乐） |
| `position` | 否 | 有声书集数；未传时可用 `pageIndex` 映射 |
| `isRecommend` | 否 | 未搜到时是否返回推荐（boolean，默认 `false`） |

**咪咕扩展（可选，进入 `migu` 分区；亦为 `SearchSongEx` 入参字段含义）：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `type` | 否 | 搜索目的，默认 `1`：`1` 歌曲、`2` 专辑、`3` 歌手、`4` 标签下歌曲、`5` 无维度、`6` 联想、`7` 歌手下单曲/专辑/MV、`8` 歌词 |
| `pageIndex` | 否 | 当前页（起始为 1）；亦可用于映射喜马 `position` |
| `pageSize` | 否 | 每页条数，`[0-50]` |
| `searchType` | 否 | `type` 为 `1` 或 `5` 时有效：`1` 智能、`2` 关键词、`3` 歌手下歌曲、`4` 指定范围搜索等 |
| `issemantic` | 否 | `type` 为 `1/2/3/4` 时有效：`1` 语义、`0` 否 |
| `isCorrect` | 否 | `type` 为 `1/2/3/4/5` 时有效：`0` 关、`1` 开 |
| `searchRange` | 否 | JSON **对象**，`type=1` 且 `searchType=4` 时精确搜索范围（意图） |

**`output` JSON：**

- `code` / `message`：平台统一业务码与消息
- `data`：命中侧结果（已从服务端的 `data.xmly` 或 `data.migu` 提升为顶层 `data`）
  - `provider`：内容来源，`"xmly"` 或 `"migu"`
  - `searchSong`：歌曲列表（数组）；喜马结果已字段对齐为咪咕形态
    - `musicId`：歌曲 ID
    - `musicName`：歌曲名称
    - `singerName`：歌手名称
    - `listenUrl`：试听地址
    - `picUrl`：封面下载地址
    - `lrcUrl`：歌词下载地址
    - `length`：歌曲时长（格式：`HH:mm:ss`）
    - 喜马侧由 `trackId` / `albumName` / `artists` / `playUrl` / `coverUrl` / `duration` 等转换而来

**示例入参：**

```json
{
  "text": "稻香",
  "providerOrder": ["xmly", "migu"],
  "intent": "listen_music",
  "type": 1,
  "pageIndex": 1,
  "pageSize": 10,
  "searchType": 4,
  "searchRange": {
    "songName": ["稻香"],
    "singerName": ["周杰伦"]
  }
}
```

仅搜咪咕（向前兼容）示例：

```json
{
  "text": "稻香",
  "providerOrder": ["migu"],
  "type": 1,
  "pageIndex": 1,
  "pageSize": 10
}
```

**返回值：** `0` 成功，其他失败。

---

### 4.7 `Report`

```cpp
virtual int Report(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | **JSON 字符串** |
| `output` | `std::string&` | **JSON 字符串** |
| `timeout` | `int` | 超时（毫秒），默认 `10000` |

**`input` 公共字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `provider` | 是 | 内容供应商（仅支持 **`migu`**） |

**咪咕（migu）侧字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `contentId` | 是 | 歌曲 ID |
| `toneQuality` | 否 | 音质：`1` 标清，`2` 高清，`3` 无损 |
| `runningTime` | 否 | 播放时长（毫秒） |
| `startTime` | 否 | 开始时间，格式 `yyyymmdd HHMMSS.mmmmmm` |
| `stopTime` | 否 | 停止时间，格式同上 |
| `userId` | 否 | 用户 ID（合作方自定义，不校验） |

**返回值：** `0` 成功，其他失败。

---

### 4.8 `GetMusicInfo`

```cpp
virtual int GetMusicInfo(const std::string& input, std::string& output, int timeout = 10000) = 0;
```

| 参数 | 类型 | 说明 |
|------|------|------|
| `input` | `std::string` | **JSON 字符串** |
| `output` | `std::string&` | **JSON 字符串** |
| `timeout` | `int` | 超时（毫秒），默认 `10000` |

**`input` JSON 字段：**

| 字段 | 必填 | 说明 |
|------|------|------|
| `provider` | 是 | 仅支持 **`migu`** |
| `contentId` | 是 | 11 位版权 ID 或 18 位内容 ID |
| `picSize` | 否 | 封面尺寸：`L` / `S` / `M`，默认 `M` |

**`output` 中 `musicInfo` 字段：** 与头文件中咪咕 MusicInfo 描述一致（`musicId`, `musicName`, `bpm`, `singerName`, `albumNames`, `songAuthorName`, `lyricAuthorName`, `length`, `language`, `picUrl`, 各清晰度试听 URL、`lrcUrl`, `isCollection`, `isCpAuth`, `singerId`, `musicSource`, `auditionsFlag`, `VIP`, `contentId`, `isUserCollection`, `listenFlag` 等）。

**可播性：** 集成方若需判断曲目是否可播，可校验 `isCpAuth`、`VIP` 均为 `"1"` 且 `listenUrl` 有效；不满足时上层可能返回 `IOTSDK_RET_UNPLAYABLE`（见第 1 节）。

**返回值：** `0` 成功，其他失败。

---

## 5. 集成注意点（摘要）

1. **板载 HTTP**：**4G 模组**须在 `Init()` 之前注册 `SetOnGetBoardHttp`，否则无法使用网络功能；**WiFi 模组**不建议注册，更省内存、性能更优。若注册，`BoardHttp` 实现可参考 [`EspBoard.cpp`](../EspBoard.cpp)。`args` 中必填项需齐全，并与 `env`（prod/test）一致。
2. 按第 3 节 **action** 区分咪咕 API（`IOTSDK_MIGU_HTTPS`）、音乐流（`IOTSDK_MIGU_MUSIC`）、OpenAPI（`IOTSDK_OPENAPI_HTTPS`）。
3. **JSON 格式**：字段名、类型需与文档及头文件注释一致。
4. **`GetMusicInfo`**、**`SearchByKey`**、**`SearchSongEx`**、**`Report`** 的 `provider` 仅支持 **`migu`**；**`SearchPlay`** 另支持 **`xmly`**（见第 4.3 节）；**`Search`** 为喜马+咪咕合并搜索，用可选 `providerOrder`（如 `["xmly","migu"]`）指定搜索顺序，未传则由服务端默认编排（见第 4.6 节）。

---

## 6. 文档维护

- 规范来源：[`IOTSdk.h`](../IOTSdk.h)。
- 若头文件中的宏、接口签名或 JSON 字段有变更，请同步更新本文档。
