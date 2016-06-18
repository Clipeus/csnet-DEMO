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
    size_t num = socket_t::receive(&size, sizeof(int16_t)); // read packet size (2 bytes)
    if (num != sizeof(int16_t) || size == 0)
        return nullptr;
    
    std::vector<int8_t> data;
    num = socket_t::receive(data, (size_t)size * sizeof(int8_t) - sizeof(int16_t)); // read whole data (size - 2 bytes)
    
    // check result, size should be equ (packet size) - (size field) i.e. size - sizeof(int16_t)
    if (num != (size - sizeof(int16_t)) || data.size() != (size - sizeof(int16_t)))
        return nullptr;
    
    // insert the size at front of the data
    data.insert(data.begin(), reinterpret_cast<int8_t*>(&size), reinterpret_cast<int8_t*>(&size) + sizeof(int16_t));

    // allocate memory for packet with data
    int8_t* placement = new int8_t[data.size()];
    // resize the packet in the allocated memory
    packet_info_t* packet = new (placement) packet_info_t;

    // copy received data to the packet
    std::memmove(packet, data.data(), data.size());
    
    return packet;
}

// send text packet to socket
// union 'packet' and 'text' and send them
bool packet_socket_t::send(const packet_info_t& packet, const std::string& text) const
{
    size_t size = sizeof(packet_info_t) + text.size() + sizeof(char); // calculate full data size
    std::vector<char> data(size); // allocate a meory for the packet

    packet_text_t* packet_text = reinterpret_cast<packet_text_t*>(data.data()); // map the packet on allocated memory
    
    std::memmove(packet_text, &packet, sizeof(packet_info_t)); // copy the packet info to the packet
    std::strcpy(packet_text->text, text.c_str()); // copy the text to the packet
    packet_text->size = size; // set full size of the packet with data
    
    return send(packet_text);
}

// send data packet to socket
// union 'packet' and 'data' and send them
bool packet_socket_t::send(const packet_info_t& packet, const void* data, size_t data_size) const
{
    size_t size = sizeof(packet_info_t) + data_size; // calculate full data size
    std::vector<int8_t> d(size); // allocate a meory for the packet

    packet_data_t* packet_data = reinterpret_cast<packet_data_t*>(d.data()); // map the packet on allocated memory
    
    std::memmove(packet_data, &packet, sizeof(packet_info_t)); // copy the packet info to the packet
    std::memmove(packet_data->data, data, data_size); // copy the data to the packet
    packet_data->size = size; // set full size of the packet with data
    
    return send(packet_data);
}

}
}
        