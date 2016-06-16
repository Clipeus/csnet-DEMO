#include "clnapi.h"

namespace csnet
{
    
using namespace shared;

clnapi_t::clnapi_t(packet_kind kind) : client_api_t(kind)
{
}

clnapi_t::~clnapi_t()
{
}

// send text to echo server and get echo from server
std::string clnapi_t::sendmsg(const std::string& msg) const
{
    // send request to server
    send(packet_code::P_ECHO_ACTION, msg);
    // receive response from server
    return receive_reply_text(packet_code::P_ECHO_ACTION);
}

// send request to server and get current time from server
std::time_t clnapi_t::gettime() const
{
    // send request to server
    send(packet_code::P_TIME_ACTION);

    // receive response from server
    std::vector<int8_t> data;
    receive_reply_data(packet_code::P_TIME_ACTION, data);
    std::time_t time = *reinterpret_cast<std::time_t*>(data.data());
    return time;
}

// send command to server and get command's result
std::string clnapi_t::execmd(const std::string& cmd) const
{
    // send request to server
    send(packet_code::P_EXECMD_ACTION, cmd);
    // receive response from server
    return receive_reply_text(packet_code::P_EXECMD_ACTION);
}

}