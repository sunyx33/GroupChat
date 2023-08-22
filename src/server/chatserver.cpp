#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include <string>
#include "chatservice.hpp"

using namespace std;
using namespace placeholders;
using namespace nlohmann;
using json = nlohmann::json;

// 初始化服务器对象
ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg)
: _server(loop, listenAddr, nameArg), _loop(loop) {
    // 给服务器注册用户连接创建断开回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
                
    // 给服务器注册用户读写事件的回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务器端的线程数量 1个io线程 3个worker线程
    _server.setThreadNum(4);

}

// 启动服务
void ChatServer::start() {
    _server.start();
}


// 链接的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    // 客户端断开连接
    if(!conn->connected()) {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 读写的回调函数
void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime) {
    string buf = buffer->retrieveAllAsString();
    // 解码json文件
    json js = json::parse(buf);
    // 解耦网络与业务代码（不在这里调用具体的业务函数）
    // 1. 获取消息类型对应的处理器
    auto msgHandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    // 2. 执行处理器
    msgHandler(conn, js, receiveTime);
}
