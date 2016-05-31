#include <cstring>
#include "packsock.h"

namespace csnet
{
namespace shared
{
    
//overloading operator + to use OR for enum class type
packet_code operator | (const packet_code& lhs, const packet_code& rhs)
{
    return static_cast<packet_code>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

//overloading operator + to use OR for enum class type
packet_code& operator |= (packet_code& lhs, const packet_code& rhs)
{
    lhs = static_cast<packet_code>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
    return lhs;
}

// receive any type packet from socket
// caller should delete the pointer
packet_info_t* packet_socket_t::receive() const
{
    int16_t size;
    size_t num = socket_t::receive(&size, sizeof(int16_t));
    if (num != sizeof(int16_t) || size == 0)
        return nullptr;
    
    std::vector<int8_t> data;
    num = socket_t::receive(data, (size_t)size * sizeof(int8_t));
    
    if (num != (size - sizeof(int16_t)) || data.size() != (size - sizeof(int16_t)))
        return nullptr;
    
    data.insert(data.begin(), (int8_t*)&size, (int8_t*)&size + sizeof(int16_t));
    packet_info_t* packet = reinterpret_cast<packet_info_t*>(new int8_t[data.size()]);
    std::memmove(packet, data.data(), data.size());
    
    return packet;
}

// send text packet to socket
// union 'packet' and 'text' and send them
bool packet_socket_t::send(const packet_info_t& packet, const std::string& text) const
{
    size_t size = sizeof(packet_info_t) + text.size() + sizeof(char);
    std::vector<char> data(size);
    packet_text_t* packet_text = reinterpret_cast<packet_text_t*>(data.data());
    
    std::memmove(packet_text, &packet, sizeof(packet_info_t));
    std::strcpy(packet_text->text, text.c_str());
    packet_text->size = size;
    
    return send(packet_text);
}

bool packet_socket_t::send(const packet_info_t& packet, const int8_t* data, size_t data_size) const
{
    size_t size = sizeof(packet_info_t) + data_size;
    std::vector<int8_t> d(size);
    packet_data_t* packet_data = reinterpret_cast<packet_data_t*>(d.data());
    
    std::memmove(packet_data, &packet, sizeof(packet_info_t));
    std::memmove(packet_data->data, data, data_size);
    packet_data->size = size;
    
    return send(packet_data);
}

}
}
        