#pragma once

#include "socket.h"

namespace csnet
{
namespace shared
{

// define packet kinds
enum class packet_kind : uint16_t
{
    P_BASE_KIND = 0 // default
};

// define packet type
enum class packet_type : uint16_t
{
    P_NULL_TYPE = 0, // packet w/o data
    P_ERROR_TYPE = 1, // error describer packet
    P_DATA_TYPE = 2, // packet with bin data
    P_TEXT_TYPE = 3, // packet with text data
    P_WTEXT_TYPE = 4 // packet with wide text data
};

enum class packet_code : uint16_t
{
    P_NO_ACTION = 0, // packet default action
    P_RETURN_ACTION = 0x8000, // answered packet 
    P_ECHO_ACTION = 1, // echo server action
    P_TIME_ACTION = 2, // get time action
    P_EXECMD_ACTION = 3, // execute command
    P_CREDENTIALS_ACTION = 4 // check credentials
};

//overloading operator + to use OR for enum class type
packet_code operator | (const packet_code& lhs, const packet_code& rhs);

//overloading operator + to use OR for enum class type
packet_code& operator |= (packet_code& lhs, const packet_code& rhs);

#pragma pack(push, 1)

// packet head
struct packet_info_t
{
    packet_info_t(packet_kind k, packet_type t, packet_code a) : kind (k), type(t), action(a) {}
    uint16_t size_data()
    {
        return size - sizeof(packet_info_t);
    }
    
    uint16_t size = sizeof(packet_info_t); // whole packet - sizeof (packet_info_t) + size of data if exists
    packet_kind kind = packet_kind::P_BASE_KIND; // see packet_kind
    packet_type type = packet_type::P_NULL_TYPE; // see packet_type
    packet_code action = packet_code::P_NO_ACTION; // see packet_code
};

// packet with bin data, P_DATA_TYPE type
struct packet_data_t : public packet_info_t
{
    int8_t data[0];
};

// packet with text data, P_TEXT_TYPE type
struct packet_text_t : public packet_info_t
{
    char text[0];
};

// packet with wide text data, P_TEXT_TYPE type
struct packet_wtext_t : public packet_info_t
{
    wchar_t text[0];
};

#pragma pack(pop)

// packet socket class
class packet_socket_t : public socket_t
{
public:
    packet_socket_t() {}
    //packet_socket_t(const socket_t& socket) : socket_t(socket) {}
    explicit packet_socket_t(int socket) : socket_t(socket) {}

    // receive text packet from socket
    // caller should delete the pointer
    packet_text_t* receive_text() const
    {
        return static_cast<packet_text_t*>(receive());
    }
    
    // receive data packet from socket
    // caller should delete the pointer
    packet_data_t* receive_data() const
    {
        return static_cast<packet_data_t*>(receive());
    }
    
    // receive any type packet from socket
    // caller should delete the pointer
    packet_info_t* receive() const;
    
    // send text packet to socket
    // union 'packet' and 'text' and send them
    bool send(const packet_info_t& packet, const std::string& text) const;
    
    // send data packet to socket
    // union 'packet' and 'data' and send them
    template <typename T>
    bool send(const packet_info_t& packet, const std::vector<T> data) const
    {
        packet.size = sizeof(packet_info_t) + data.size();
        return send_data(packet, data.data(), data.size() * sizeof(T));
    }
    
    // send data packet to socket
    // union 'packet' and 'data' and send them
    bool send(const packet_info_t& packet, const void* data, size_t data_size) const;
    
    // send packet w/o data from socket
    bool send(const packet_info_t& packet) const
    {
        //packet.size = sizeof(packet_info_t);
        return send(&packet);
    }

    // send any type packet from socket
    bool send(const packet_info_t* packet) const
    {
        return socket_t::send((int8_t*)packet, (size_t)packet->size) == packet->size;
    }
};

}
}
