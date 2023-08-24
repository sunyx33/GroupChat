#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    bool addGroup(int userid, int groupid, string role);
    // 查询用户所在的群组
    vector<Group> queryGroups(int userid);
    // 查询群组成员，除userid自己，用于发群消息
    vector<int> queryGroupUsers(int userid, int groupid);
private:
};

#endif


