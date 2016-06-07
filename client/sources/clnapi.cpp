#include <cstring>

#include "clnapi.h"

namespace csnet
{
    
using namespace shared;

clnapi_t::clnapi_t()
{
}

clnapi_t::~clnapi_t()
{
}

// connect to server w/o credentials
void clnapi_t::connect(const std::string& host, int port)
{
    close();

    // init socket object

    if (!_socket.create())
        throw clnapi_error(_socket.error_msg());

    if (!_socket.connect(host, port))
        throw clnapi_error(_socket.error_msg());
}

// connect to server with credentials
void clnapi_t::connect(const std::string& host, int port, const std::string& login, const std::string& password)
{
    connect(host, port);
    check_credentials(login, password);
}

// send credentials to server to check them
void clnapi_t::check_credentials(const std::string& login, const std::string& password) const
{
    credentials_info ci;
    std::strncpy(ci.login, login.c_str(), 32);
    std::strncpy(ci.password, password.c_str(), 32);

    // send request to server
    if (!_socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_DATA_TYPE, packet_code::P_CREDENTIALS_ACTION), &ci, sizeof(ci)))
        throw clnapi_error(_socket.error_msg());

    // receive response from server
    std::unique_ptr<packet_text_t> packet(_socket.receive_text());
    if (!packet)
        throw clnapi_error(_socket.error_msg().size() ? _socket.error_msg() : "error receiving packet");
    else if (packet->kind != packet_kind::P_BASE_KIND || packet->type != packet_type::P_TEXT_TYPE || packet->action != (packet_code::P_CREDENTIALS_ACTION | packet_code::P_RETURN_ACTION))
        throw clnapi_error("Unknown packet");

    if (packet->text)
        throw clnapi_error(packet->text); // invalid credentials
}

// close connection
void clnapi_t::close()
{
    _socket.close();
}

// send text to echo server and get echo from server
std::string clnapi_t::sendmsg(const std::string& msg) const
{
    // send request to server
    if (!_socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_ECHO_ACTION), msg))
        throw clnapi_error(_socket.error_msg());

    // receive response from server
    std::unique_ptr<packet_text_t> packet(_socket.receive_text());
    if (!packet)
        throw clnapi_error(_socket.error_msg().size() ? _socket.error_msg() : "error receiving packet");
    else if (packet->kind != packet_kind::P_BASE_KIND || packet->type != packet_type::P_TEXT_TYPE || packet->action != (packet_code::P_ECHO_ACTION | packet_code::P_RETURN_ACTION))
        throw clnapi_error("Unknown packet");

    return packet->text;
}

// send request to server and get current time from server
std::time_t clnapi_t::gettime() const
{
    // send request to server
    if (!_socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_DATA_TYPE, packet_code::P_TIME_ACTION)))
        throw clnapi_error(_socket.error_msg());

    // receive response from server
    std::unique_ptr<packet_data_t> packet(_socket.receive_data());
    if (!packet)
        throw clnapi_error(_socket.error_msg().size() ? _socket.error_msg() : "error receiving packet");
    else if (packet->kind != packet_kind::P_BASE_KIND && packet->type != packet_type::P_DATA_TYPE && packet->action != (packet_code::P_TIME_ACTION | packet_code::P_RETURN_ACTION))
        throw clnapi_error("Unknown packet");

    std::time_t time = *(std::time_t*)packet->data;
    return time;
}

// send command to server and get command's result
std::string clnapi_t::execmd(const std::string& cmd) const
{
    // send request to server
    if (!_socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_EXECMD_ACTION), cmd))
        throw clnapi_error(_socket.error_msg());

    // receive response from server
    std::unique_ptr<packet_text_t> packet(_socket.receive_text());
    if (!packet)
        throw clnapi_error(_socket.error_msg().size() ? _socket.error_msg() : "error receiving packet");
    else if (packet->kind != packet_kind::P_BASE_KIND || packet->type != packet_type::P_TEXT_TYPE || packet->action != (packet_code::P_EXECMD_ACTION | packet_code::P_RETURN_ACTION))
        throw clnapi_error("Unknown packet");

    return packet->text;
}

}