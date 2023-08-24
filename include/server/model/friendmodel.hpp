#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <vector>
using namespace std;

// 提供好友信息的操作类
class FriendModel {
public:
    // 添加好友关系
    bool insert(int userid, int friendid);

    // 返回用户好友列表 friendid name
    vector<User> query(int userid);


};

#endif


