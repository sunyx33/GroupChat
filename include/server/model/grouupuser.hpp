#ifndef GROUUPUSER_H
#define GROUUPUSER_H

#include "user.hpp"

class GroupUser : public User {
public:
    void setRole(string role) {this->role = role;}
    string getRole() {return this->role;}
private:
    string role;
};

#endif
