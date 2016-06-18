#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include <chrono>  

#include "csnet_api.h"

#ifndef WSAECONNREFUSED
#define WSAECONNREFUSED ECONNREFUSED
#endif

namespace csnet
{
namespace shared
{

// client net api base class
csnet_api_t::csnet_api_t(packet_kind kind) : _kind(kind)
{
}

csnet_api_t::~csnet_api_t()
{
}

// send action to server
void csnet_api_t::send(packet_code action) const
{
    if (!_socket.send(packet_info_t(_kind, packet_type::P_NULL_TYPE, action)))
        throw csnet_api_error(_socket.error_msg());
}

// send data to server
void csnet_api_t::send(packet_code action, const void* data, size_t size) const
{
    if (!_socket.send(packet_info_t(_kind, packet_type::P_DATA_TYPE, action), data, size))
        throw csnet_api_error(_socket.error_msg());
}

// send text to server
void csnet_api_t::send(packet_code action, const std::string& text) const
{
    if (!_socket.send(packet_info_t(_kind, packet_type::P_TEXT_TYPE, action), text))
        throw csnet_api_error(_socket.error_msg());
}

// send error to server
void csnet_api_t::send(packet_code action, uint32_t error, const std::string& text) const
{
    packet_info_t packet(_kind, packet_type::P_ERROR_TYPE, action);

    size_t size = sizeof(packet_error_t) + text.size() + sizeof(char);
    std::vector<char> data(size);
    packet_error_t* packet_error = reinterpret_cast<packet_error_t*>(data.data());

    std::memmove(packet_error, &packet, sizeof(packet_info_t));
    std::strcpy(packet_error->error_text, text.c_str());
    packet_error->size = size;
    packet_error->error_code = error;

    if (!_socket.send(packet_error))
        throw csnet_api_error(_socket.error_msg());
}

// did server return error?
void csnet_api_t::iserror(packet_info_t* packet, packet_type type, packet_code action) const
{
    iserror(packet);
    if (packet->kind != _kind || packet->type != type || packet->action != action)
        throw csnet_api_error("Unknown packet");
}

// did server return error?
void csnet_api_t::iserror(packet_info_t* packet) const
{
    if (!packet)
        throw csnet_api_error(_socket.error_msg().size() ? _socket.error_msg() : "Error receiving packet");
    else if (packet->kind == _kind && packet->type == packet_type::P_ERROR_TYPE)
        throw csnet_api_error(static_cast<packet_error_t*>(packet)->error_text);
}

// receive null from server
void csnet_api_t::receive(packet_code action) const
{
    std::unique_ptr<packet_info_t> packet(_socket.receive());
    iserror(packet.get(), packet_type::P_NULL_TYPE, action);
}

// receive text from server
std::string csnet_api_t::receive_text(packet_code action) const
{
    std::unique_ptr<packet_text_t> packet(_socket.receive_text());
    iserror(packet.get(), packet_type::P_TEXT_TYPE, action);

    return packet->text;
}

// receive data from server
void csnet_api_t::receive_data(packet_code action, std::vector<int8_t>& data) const
{
    std::unique_ptr<packet_data_t> packet(_socket.receive_data());
    iserror(packet.get(), packet_type::P_DATA_TYPE, action);

    data.resize(packet->size_data());
    std::copy(packet->data, packet->data + packet->size_data(), data.begin());
}

/////////////////////////////////////////////////
// server net api wrapper
server_api_t::server_api_t(packet_kind kind)
{
}

server_api_t::~server_api_t()
{
}

// on accept
void server_api_t::onaccept(packet_socket_t&& socket)
{
    _socket = std::move(socket);
}

// receive data from server
bool server_api_t::receive()
{
    _packet.reset(_socket.receive());
    return _packet != nullptr;
}

// is packet of the type
bool server_api_t::is_packet_of(packet_type type, packet_code action) const
{
    return _packet != nullptr ? (_packet->kind == _kind && _packet->type == type && _packet->action == action) : false;
}

/////////////////////////////////////////////////
// client net api wrapper
client_api_t::client_api_t(packet_kind kind) : csnet_api_t(kind)
{
}

client_api_t::~client_api_t()
{
}

// connect to server w/o credentials
void client_api_t::connect(const std::string& host, int port, int connect_attempts, int next_attempt)
{
    //close();

    //// init socket object

    //if (!_socket.create())
    //    throw csnet_api_error(_socket.error_msg());

    // try to connect
    int res = 0;
    for (int i = 0; i < connect_attempts; i++)
    {
        close();

        // init socket object

        if (!_socket.create())
            throw csnet_api_error(_socket.error_msg());

        res = _socket.connect(host, port);
        if (res)
            break; // connected
        else if (WSAECONNREFUSED != _socket.error()) // checking for "No connection could be made because the target machine actively refused it"
            throw csnet_api_error(_socket.error_msg()); 

        // server is busy, waiting for next connection
        std::this_thread::sleep_for(std::chrono::milliseconds(next_attempt));
    }
    
    if (!res) // attempts have been exhausted
        throw csnet_api_error(_socket.error_msg());

    // it is OK!
}

// close connection
void client_api_t::close()
{
    _socket.close();
}

// check server connection
uint64_t client_api_t::ping(uint64_t data) const
{
    // send request to server
    send(packet_code::P_PING_ACTION, &data, sizeof(data));

    // receive response from server
    std::vector<int8_t> ret;
    receive_reply_data(packet_code::P_PING_ACTION, ret);

    uint64_t result;
    std::memcpy(&result, ret.data(), std::min(sizeof(uint64_t), ret.size()));
    return result;
}

// receive null reply from server
void client_api_t::receive_reply(packet_code action) const
{
    receive(action | packet_code::P_RETURN_ACTION);
}

// receive reply text from server
std::string client_api_t::receive_reply_text(packet_code action) const
{
    return receive_text(action | packet_code::P_RETURN_ACTION);
}

// receive reply data from server
void client_api_t::receive_reply_data(packet_code action, std::vector<int8_t>& data) const
{
    receive_data(action | packet_code::P_RETURN_ACTION, data);
}

}
}