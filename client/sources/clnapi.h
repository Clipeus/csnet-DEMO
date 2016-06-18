#pragma once

#include <ctime>

#include "csnet_api.h"

namespace csnet
{

// hi-level client api implementation

// client net api wrapper
class clnapi_t : public shared::client_api_t
{
public:
    clnapi_t(shared::packet_kind kind = shared::packet_kind::P_BASE_KIND);
    virtual ~clnapi_t();

public:
    // send text to echo server and get echo from server
    std::string sendmsg(const std::string& msg) const;
    // send request to server and get current time from server
    std::time_t gettime() const;
    // send command to server and get command's result
    std::string execmd(const std::string& cmd) const;
    // send credentials to server to check them
    void check_credentials(const std::string& login, const std::string& password) const;
};

}