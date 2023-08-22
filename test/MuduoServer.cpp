/*
muoduo网络库给用户提供了两个主要的类：
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序
好处：能把网络IO代码和业务代码区分开
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace muduo::net;
using namespace placeholders;

/*
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer的构造函数需要什么参数，创建ChatServer的构造函数
4. 在当前服务器类的构造函数，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo库会自己划分io线程和worker线程
*/
class ChatServer{
public:
    ChatServer(EventLoop* loop,  // 事件循环
            const InetAddress& listenAddr,  // IP + port
            const string& nameArg)  // 服务器的名字
            : _server(loop, listenAddr, nameArg), _loop(loop) {
                // 给服务器注册用户连接创建断开回调
                _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
                
                // 给服务器注册用户读写事件的回调
                _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

                // 设置服务器端的线程数量 1个io线程 3个worker线程
                _server.setThreadNum(4);
            }

    // 开启事件循环
    void start() {
        _server.start();
    }
private:
    // 专门处理用户的连接创建和断开 listenfd -> accept -> epoll
    // muoduo库封装好了，我们只用实现回调函数即可
    void onConnection(const TcpConnectionPtr& conn) {
            if(conn->connected()) {
                cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " online" << endl;
            } else {
                cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << " offline" << endl;
                conn->shutdown();
            }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,    //连接
                            Buffer* buffer,    // 缓冲区
                            muduo::Timestamp time) // 接收的时间信息
    {
        string buf = buffer->retrieveAllAsString(); 
        cout << "recv data:" << buf << " time:" << time.toString() << endl;
        conn->send(buf);
    } 
    TcpServer _server;   // #1
    EventLoop* _loop;    // #2 epoll
};

int main() {
    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start(); // listenfd -> epoll
    loop.loop();    // 相当于epoll_wait
    return 0;
}