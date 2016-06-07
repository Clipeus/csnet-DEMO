#pragma once

#include <string>
#include <stdexcept>
#include <ctime>

#include "packsock.h"

namespace csnet
{

// hi-level client api implementation

// client api exception
class clnapi_error : public std::runtime_error
{
public:
    explicit clnapi_error(const std::string& msg) : std::runtime_error(msg.c_str())
    {
    }
    explicit clnapi_error(const char* msg) : std::runtime_error(msg)
    {
    }
};

// client api wrapper
class clnapi_t
{
    struct credentials_info
    {
        char login[32 + 1];
        char password[32 + 1];
    };

public:
    clnapi_t();
    virtual ~clnapi_t();

public:
    // connect to server w/o credentials
    void connect(const std::string& host, int port);
    // connect to server with credentials
    void connect(const std::string& host, int port, const std::string& login, const std::string& password);
    // close connection
    void close();

    // send text to echo server and get echo from server
    std::string sendmsg(const std::string& msg) const;
    // send request to server and get current time from server
    std::time_t gettime() const;
    // send command to server and get command's result
    std::string execmd(const std::string& cmd) const;

protected:
    // send credentials to server to check them
    void check_credentials(const std::string& login, const std::string& password) const;

protected:
    shared::packet_socket_t _socket;
};

}