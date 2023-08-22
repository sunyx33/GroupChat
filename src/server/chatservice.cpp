#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>

using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService* ChatService::getInstance() {
    static ChatService service;
    return &service;
}

// 注册消息以及对应的回调函数
ChatService::ChatService() {
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
}

// 处理登录业务  id  pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPassword() == pwd) {
        if(user.getState() == "online") {
            // 不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经在线，禁止重复登陆";
            conn->send(response.dump());
        } else {            
            // 登录成功，记录用户连接信息
            { // 单独开代码块是为了让锁尽快释放掉
                lock_guard<mutex> lock(_connMapMutex);
                _userConnMap.insert({id, conn});
            }

            // 登录成功，更新用户状态信息 offline -> online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 处理离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()) {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }

            // 查询好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()) {
                vector<string> vec2;
                for(User &user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            
            conn->send(response.dump());
        }

    } else {
        // 用户不存在 或 用户存在密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        conn->send(response.dump());
    }
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time){
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool ret = _userModel.insert(user);
    if(ret) { // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else { // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); 
    }

}

// 获取消息对应的处理函数
MsgHandler ChatService::getHandler(int msgid) {
    // 记录错误日志：msgid没有对应的事件处理回调
    auto it= _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()) {
        // 返回一个空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " can't find handler!";
        };
    } else {
        return _msgHandlerMap[msgid];
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMapMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()) {
            // toid在线，转发消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 添加好友信息
    _friendModel.insert(userid, friendid);
}

// 服务器异常，业务重置方法
void ChatService::reset() {
    // online -> offline
    _userModel.resetState();
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        lock_guard<mutex> lock(_connMapMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
            if(it->second == conn) {
                user.setId(it->first);
                // 从map删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 更新用户的状态
    if(user.getId() != -1) {
        user.setState("offline");
        _userModel.updateState(user);
    }
}