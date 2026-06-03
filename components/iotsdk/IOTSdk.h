//
//  IOTSdk.h
//
#ifndef _IOT_SDK_H_
#define _IOT_SDK_H_

#include <string>
#include <memory>
#include "EspBoard.h"

/*------------------------- 错误码定义 -------------------------*/
/**
 * 成功
 */
#define IOTSDK_RET_OK 0
/**
 * 未知错误
 */
#define IOTSDK_RET_ERROR -1
/**
 * 无效的参数
 */
#define IOTSDK_RET_INVALID_PARAMS -2
/**
 * 资源未就绪
 */
#define IOTSDK_RET_RESOURCE_NOT_READY -3
/**
 * 请求发生异常
 */
#define IOTSDK_RET_REQUEST_EXCEPTION -4
/**
 * 请求不支持/无效的请求
 */
#define IOTSDK_RET_INVALID_REQUEST -5
/**
 * 请求超时
 */
#define IOTSDK_RET_TIMEOUT -6

/*------------------------- 内容提供商定义 -------------------------*/
/**
 * 内容提供商：咪咕
 */
#define IOTSDK_PROVIDER_MIGU "migu"
/**
 * 内容提供商：喜马拉雅
 */
#define IOTSDK_PROVIDER_XMLY "xmly"

/*------------------------- HTTP连接意图定义 -------------------------*/
/**
 * 咪咕HTTPS API接口
 */
#define IOTSDK_MIGU_HTTPS 1
/**
 * 咪咕HTTPS 音乐流
 */
#define IOTSDK_MIGU_MUSIC 2
/**
 * IOTSdk OpenAPI接口
 */
#define IOTSDK_OPENAPI_HTTPS 3

class IOTSdk
{
public:
    /**
     * 获取单例
     */
    static IOTSdk& Singleton();
    /**
     * 初始化
     * @param workspace 工作目录
     * @param args 初始化参数(json字符串)
     * deviceId: 设备唯一标识(必填)
     * appLicenseId: 许可证id(必填)
     * appKey: 区域编码(必填)
     * serverToken: 服务端token(必填)
     * regionCode: 区域编码(必填)
     * servicePackageCode: 服务套餐码(必填)
     * env: 环境选择。生产环境(prod)，测试环境(test)
     * @return 0 成功，其他失败
     */
    virtual int Init(const std::string& workspace, const std::string& args) = 0;
    /**
     * 设置获取板载HTTP客户端回调
     * 特别注意：
     * 1.必须在IOTSdk::Init()之前注册
     * 2.BoardHttp实现可以参考EspBoard.cpp
     * 3.如果是板载4G模组，必须调用该函数设置回调，否则无法使用网络功能
     * 4.如果是板载WiFi模组，不建议调用该函数设置回调，更节省内存，性能更优
     */
     virtual void SetOnGetBoardHttp(std::function<std::unique_ptr<BoardHttp>(int action)> on_get_board_http) = 0;
    /**
     * 查询歌曲
     * @param input 输入参数(json字符串)
     * provider: 内容供应商(必填, 仅支持migu)
     * content: 输入文本内容(必填)
     * @param output 返回结果(json字符串)
     *   MusicInfo(咪咕):
     *      musicId: 歌曲ID（版权ID）
     *      musicName: 歌曲名称
     *      bpm: 歌曲步频
     *      singerName: 歌手名称
     *      albumNames: 专辑名称列表（最多返回2个）
     *      songAuthorName: 曲作者
     *      lyricAuthorName: 词作者
     *      length: 歌曲时长（格式：HH:mm:ss）
     *      language: 歌曲语种
     *      picUrl: 歌曲封面图URL
     *      listenUrl: 试听地址（标清格式）
     *      hqListenUrl: 试听地址（高清格式）
     *      sqListenUrl: 试听地址（无损格式）
     *      lrcUrl: 歌词地址
     *      isCollection: 是否收藏（1-已收藏，0-未收藏）
     *      isCpAuth: 是否CP授权（1-已授权，0-未授权）
     *      singerId: 歌手ID
     *      musicSource: 歌曲来源类型（1-咪咕善跑APP，2-咪咕音乐APP）
     *      auditionsFlag: 试听标签，详见下方说明
     *      VIP: VIP标识（0-CP未授权，1-普通歌曲，2-白金会员歌曲，3-敏感库歌曲，4-数字专辑，5-单曲按次）
     *      contentId: 内容ID
     *      isUserCollection: 用户是否收藏（1-收藏，0-未收藏）
     *      listenFlag: 试听标识（0-不可以试听，1-可以试听）
     *   MusicInfo(喜马拉雅):
     *      trackId: 声音Id
     *      coverUrl: 封面图片地址
     *      playUrl: 音频播放地址,加密
     *      playSize: 音频文件大小,单位字节
     *      duration: 声音时长,单位秒
     *      positionIndex: 位点字段,由调用方原样传递
     *      hasNext: 是否还有下一页,请根据该字段判断是否有下一页,不能通过数据条数判断
     *      title: 声音标题
     *      albumName: 专辑名
     *      artists: 艺人名,多个用逗号隔开
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */
    virtual int SearchPlay(const std::string& input, std::string& output, int timeout = 10000) = 0;
    /**
     * 关键字搜歌（NEW，咪咕）
     * @param input 输入参数(json字符串)
     * provider: 内容供应商(必填, 仅支持migu)
     * text: 搜索内容(必填)；当 type=1 且 searchType=3，或 type=7 时 text 为歌手 id
     * type: 搜索目的(可选)；默认为1（1-歌曲 2-专辑 3-歌手 4-标签下歌曲 5-无维度 6-联想 7-歌手下单曲专辑MV 8-歌词）
     * pageIndex: 当前页(起始页为1)，可选
     * pageSize: 每页条数，[0-50]，可选
     * searchType: 搜索类型：type 为 1 或 5 时使用（1-智能 2-关键词 3-歌手下歌曲 4-指定范围搜索等，见文档）
     * issemantic: 语义判定：type 为 1/2/3/4 时有效（1-语义 0-否）
     * isCorrect: 容错：type 为 1/2/3/4/5 时有效（0-关 1-开）
     * searchRange: 对象，type=1 且 searchType=4 时精确指定搜索范围（意图），见接口文档
     * @param output 返回结果(json字符串)
     * code: 由接口 resCode 映射
     * message: 由接口 resMsg 映射
     * 业务数据对象，【重要】随请求 type 不同，主要业务数据落在对应根字段（其余字段可能为空）
     *      searchSong: SearchSongNewRsp，type=1 搜歌曲
     *      searchAlbum: SearchAlbumNewRsp，type=2 搜专辑
     *      searchSinger: SearchSingerNewRsp，type=3 搜歌手
     *      searchTag: SearchSongNewRsp，type=4 标签下歌曲（结构与 searchSong 同类）
     *      searchAll: SearchAllNewRsp，type=5 无维度搜索
     *      searchSuggest: SearchSuggestNewRsp，type=6 联想搜索
     *      searchSingerSAM: SearchSingerSAMNewRsp，type=7 歌手下单曲/专辑/MV
     *      searchLyric: SearchLyricNewRsp，type=8 歌词搜歌
     *
     * 【SearchSongNewRsp / searchTag 内层】（与搜歌、标签下歌曲相关，对应 searchSong、searchTag）
     *      rc: 返回码，0 表示成功
     *      error: 错误描述，rc>0 时返回
     *      correct: 容错结果 string[]
     *      data: SearchSongNewData
     *      semantic: 容错结果类型 SemanticResult[]
     *      data.total: 总记录数
     *      data.result: SongNew[] 歌曲列表
     *      data.resultType: 1-标签下歌曲(默认) 2-关键词搜索 3-搜索歌手下歌曲
     *      data.fullSongTotal: 全曲总数
     *
     * 【SearchAlbumNewRsp】
     *      rc, error, correct, data, semantic（语义同 SearchSongNewRsp）
     *      data.total: 总记录数
     *      data.result: AlbumNew[] 专辑列表
     *
     * 【SearchSingerNewRsp】
     *      rc, error, correct, data, semantic
     *      data.total: 总记录数
     *      data.result: SingerNew[] 歌手列表
     *
     * 【SearchAllNewRsp】（无维度）
     *      rc, error, data
     *      data.songResult: SearchSongNewRsp
     *      data.albumResult: SearchAlbumNewRsp
     *      data.singerResult: SearchSingerNewRsp
     *      data.tagSongResult: SearchSongNewRsp
     *      data.bestShowResult: BestShowResult（最佳展示）
     *
     * 【SearchSuggestNewRsp】（联想）
     *      rc, error, data
     *      data.songList: SongNew[]
     *      data.albumList: AlbumNew[]
     *      data.singerList: SingerNew[]
     *
     * 【SearchSingerSAMNewRsp】（歌手下单曲/专辑/MV）
     *      rc, error, data
     *      data.songResult: SearchSongNewRsp
     *      data.albumResult: SearchAlbumNewRsp
     *
     * 【SearchLyricNewRsp】（歌词搜歌）
     *      rc, error, correct, data, semantic
     *      data.total: 总记录数
     *      data.result: SongNew[]
     *      data.resultType: 1-标签下歌曲(默认) 2-关键词搜索 3-搜索歌手下歌曲
     *
     * SongNew（歌曲条目，与搜歌结果中歌曲结构同类，常用字段）:
     *      id: 歌曲 id
     *      name: 歌曲名称
     *      highlightLyricStr: 高亮歌词
     *      multiLyricStr: 匹配到的三行歌词
     *      singers: SingerNew[] 歌手信息
     *      albums: AlbumNew[] 专辑信息
     *      movieNames: 影视名称
     *      televisionNames: 电视节目名称
     *      strlyric: 歌词文本
     *      fullSongs: ToneNew[] 全曲数据
     *      singerName: 歌手名（多歌手「 ,」分隔）
     *      lcsscore: 匹配度得分
     *
     * SingerNew:
     *      id, name, songNum, albumNum, fullSongTotal, lcsscore 等
     *
     * AlbumNew:
     *      id, name, type, dalbumID, copyrightID, singers, albumPic, fullSongTotal, lcsscore 等
     *
     * ToneNew（全曲）:
     *      id: 产品 id（18 位）
     *      copyrightId: 版权 id（11 或 12 位，见文档）
     *      price: 价格
     *      expireDate: 有效期
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */
    virtual int SearchByKey(const std::string& input, std::string& output, int timeout = 10000) = 0;
    /**
     * 搜歌曲（NEW，咪咕）
     * @param input 输入参数(json字符串)
     * 入参字段与 SearchByKey() 相同：
     * provider: 内容供应商(必填, 仅支持migu)
     * text: 搜索内容(必填)；当 type=1 且 searchType=3，或 type=7 时 text 为歌手 id
     * type: 搜索目的(可选)；默认为1（1-歌曲 2-专辑 3-歌手 4-标签下歌曲 5-无维度 6-联想 7-歌手下单曲专辑MV 8-歌词）
     * pageIndex: 当前页(起始页为1)，可选
     * pageSize: 每页条数，[0-50]，可选
     * searchType: 搜索类型：type 为 1 或 5 时使用（1-智能 2-关键词 3-歌手下歌曲 4-指定范围搜索等，见文档）
     * issemantic: 语义判定：type 为 1/2/3/4 时有效（1-语义 0-否）
     * isCorrect: 容错：type 为 1/2/3/4/5 时有效（0-关 1-开）
     * searchRange: 对象，type=1 且 searchType=4 时精确指定搜索范围（意图），见接口文档
     * @param output 返回结果(json字符串)
     * code: 返回码（示例："1"）
     * message: 返回消息（示例："Success"）
     * traceId: 链路追踪ID
     * data: 业务数据对象
     *      searchSong: 歌曲列表（数组）
     *          musicId: 歌曲ID（18位产品ID）
     *          musicName: 歌曲名称
     *          singerName: 歌手名称
     * responseSign: 响应签名（可能为 null）
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */
    virtual int SearchSong(const std::string& input, std::string& output, int timeout = 10000) = 0;
    /**
     * 搜歌曲扩展（咪咕）
     * @param input 输入参数(json字符串)，与 SearchSong() 完全一致（字段含义见 SearchSong()）。
     * @param output 返回结果(json字符串)，整体与 SearchSong() 基本一致：同为 code、message、data 等结构，
     *      data.searchSong 为歌曲列表；每条记录在 SearchSong 已提供的 singerName、musicName（歌曲名称）等基础上，
     *      额外包含 listenUrl（试听地址）；其余业务字段与 SearchSong 返回对齐。
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */
    virtual int SearchSongEx(const std::string& input, std::string& output, int timeout = 10000) = 0;
    /**
     * 用户听歌记录上报
     * @param input 输入参数(json字符串)
     * provider: 内容供应商(必填, 仅支持migu)
     * 咪咕要求参数:
     *      contentId: 歌曲ID(必填)
     *      toneQuality: 音质。1：标清；2：高清；3：无损
     *      runningTime: 播放时长(毫秒)
     *      startTime: 播放开始时间  格式yyyymmdd HHMMSS.mmmmmm
     *      stopTime: 播放停止时间  格式yyyymmdd HHMMSS.mmmmmm
     *      userId: 用户ID（合作伙伴自定义，不校验）
     * @param output 返回结果(json字符串)
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */    
    virtual int Report(const std::string& input, std::string& output, int timeout = 10000) = 0;
    /**
     * 根据歌曲ID查询歌曲信息
     * @param input 输入参数(json字符串)
     * provider: 内容供应商(必填, 仅支持migu)
     * contentId: 歌曲ID(必填, 支持11位版权ID或18位内容ID)
     * picSize: 图片尺寸(可选, L-大图，S-小图，M-中图，默认M)
     * @param output 返回结果(json字符串)
     *   musicInfo:
     *      musicId: 歌曲ID（版权ID）
     *      musicName: 歌曲名称
     *      bpm: 歌曲步频
     *      singerName: 歌手名称
     *      albumNames: 专辑名称列表（最多返回2个）
     *      songAuthorName: 曲作者
     *      lyricAuthorName: 词作者
     *      length: 歌曲时长（格式：HH:mm:ss）
     *      language: 歌曲语种
     *      picUrl: 歌曲封面图URL
     *      listenUrl: 试听地址（标清格式）
     *      hqListenUrl: 试听地址（高清格式）
     *      sqListenUrl: 试听地址（无损格式）
     *      lrcUrl: 歌词地址
     *      isCollection: 是否收藏（1-已收藏，0-未收藏）
     *      isCpAuth: 是否CP授权（1-已授权，0-未授权）
     *      singerId: 歌手ID
     *      musicSource: 歌曲来源类型（1-咪咕善跑APP，2-咪咕音乐APP）
     *      auditionsFlag: 试听标签
     *      VIP: VIP标识（0-CP未授权，1-普通歌曲，2-白金会员歌曲，3-敏感库歌曲，4-数字专辑，5-单曲按次）
     *      contentId: 内容ID
     *      isUserCollection: 用户是否收藏（1-收藏，0-未收藏）
     *      listenFlag: 试听标识（0-不可以试听，1-可以试听）
     * @param timeout 超时时长，单位ms
     * @return 0 成功，其他失败
     */
    virtual int GetMusicInfo(const std::string& input, std::string& output, int timeout = 10000) = 0;
};

#endif//_IOT_SDK_H_
