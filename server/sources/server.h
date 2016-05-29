#pragma once

#include "signals.h"
#include "socket.h"

namespace csnet
{

class server_t
{
public:
    server_t();
    ~server_t();    
    
protected:
    void init_socket();
    void init_signal();
    void close();
    bool is_finished();
    std::string exec(const std::string& cmd);

public:
    int run();
    void onsignal(const csnet::shared::signal_t<server_t>* sender, int signal);
    
protected:
    shared::socket_t _listener;
    bool _finished = false;
    shared::signal_t<server_t> _signal;
};

}