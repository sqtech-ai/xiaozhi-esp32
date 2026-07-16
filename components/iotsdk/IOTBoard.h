//
//  IOTBoard.h
//
#ifndef _IOT_BOARD_H_
#define _IOT_BOARD_H_

#include <string>
#include <functional>

class BoardHttp {
public:
    virtual void SetTimeout(int timeout_ms) = 0;
    virtual void SetHeader(const std::string& key, const std::string& value) = 0;
    virtual void SetContent(std::string&& content) = 0;
    virtual bool Open(const std::string& method, const std::string& url) = 0;
    virtual void Close() = 0;
    virtual int Read(char* buffer, size_t buffer_size) = 0;
    virtual int Write(const char* buffer, size_t buffer_size) = 0;
    virtual int GetStatusCode() = 0;
    virtual std::string GetResponseHeader(const std::string& key) const = 0;
    virtual size_t GetBodyLength() = 0;
    virtual std::string ReadAll() = 0;
};

class BoardMqtt {
public:
    virtual void SetKeepAlive(int keep_alive_seconds) = 0;
    virtual bool Connect(const std::string broker_address, int broker_port, const std::string client_id, const std::string username, const std::string password) = 0;
    virtual void Disconnect() = 0;
    virtual bool Publish(const std::string topic, const std::string payload, int qos = 0) = 0;
    virtual bool Subscribe(const std::string topic, int qos = 0) = 0;
    virtual bool Unsubscribe(const std::string topic) = 0;
    virtual bool IsConnected() = 0;
    virtual void OnConnected(std::function<void()> callback) = 0;
    virtual void OnDisconnected(std::function<void()> callback) = 0;
    virtual void OnMessage(std::function<void(const std::string& topic, const std::string& payload)> callback) = 0;
    virtual void OnError(std::function<void(const std::string& error)> callback) = 0;
};

#endif // _IOT_BOARD_H_