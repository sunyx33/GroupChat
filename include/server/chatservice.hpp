#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <json.hpp>
#include <mutex>
#include "usermodel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;
// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 聊天服务器的业务类，单例模式
class ChatService{
public:
    // 获取单例对象的接口函数
    static ChatService* getInstance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常，业务重置方法
    void reset();

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 获取消息对应的处理函数
    MsgHandler getHandler(int msgid);

private:
    ChatService();

    // 存储消息id（类型）及其对应的处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 需要把连接存下来，因为a->b，服务器不仅仅收a的消息，还要给b进行推送，所以要获得与b的连接以主动推送
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap线程安全
    mutex _connMapMutex;
    
    // 数据操作类对象
    UserModel _userModel;

    // 离线消息操作对象
    OfflineMsgModel _offlineMsgModel;

    // 好友信息操作对象
    FriendModel _friendModel;

    // 群组操作对象
    GroupModel _groupModel;

};

#endif