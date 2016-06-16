#pragma once

#include "signals.h"
#include "srvapi.h"

namespace csnet
{

// socket server class
class myserver_t //: public shared::csnet_api_t
{
public:
    myserver_t(std::unique_ptr<service_i> handler);
    virtual ~myserver_t();

public:
    // start server
    int start(int port, int pool_count);
    // stopt server
    void stop();
    // signal handler
    void onsignal(const shared::signal_t<myserver_t>* sender, int signal);

protected:
    void init_socket(int port, int pool_count);
    void init_signal();
    // true if need to exit
    bool is_finished();

protected:
    shared::packet_socket_t _socket;
    std::unique_ptr<service_i> _handler;
    shared::signal_t<myserver_t> _signal;
    bool _finished = false;
#ifdef _WIN32
    SOCKET _cancel = INVALID_SOCKET;
#endif
};

}