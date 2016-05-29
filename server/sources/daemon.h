#pragma once

namespace csnet
{

class daemon_t
{
public:
    daemon_t(int argc = 0, char** args = nullptr);
    ~daemon_t();
    
public:
    int run();
    
protected:
    virtual bool parse_cmd();
    virtual int process();
    
protected:
    int _argc;
    char** _args;
};

}