#pragma once

#include <string>
#include <stdexcept>

#include "packsock.h"

namespace csnet
{
  namespace shared
  {

    // hi-level server api implementation

    // server api exception
    class csnet_api_error : public std::runtime_error
    {
    public:
      explicit csnet_api_error(const std::string& msg) : std::runtime_error(msg.c_str())
      {
      }
      explicit csnet_api_error(const char* msg) : std::runtime_error(msg)
      {
      }
    };

    // client net api base class
    class csnet_api_t
    {
    protected:
      csnet_api_t(packet_kind kind = packet_kind::P_BASE_KIND);
      virtual ~csnet_api_t();

    protected:
      // send action to server
      void send(packet_code action) const;
      // send data to server
      void send(packet_code action, const void* data, size_t size) const;
      // send text to server
      void send(packet_code action, const std::string& text) const;
      // send error to server
      void send(packet_code action, uint32_t error, const std::string& text) const;

      // did server return error?
      virtual void iserror(packet_info_t* packet, packet_type type, packet_code action) const;
      // did server return error?
      virtual void iserror(packet_info_t* packet) const;
      // receive null from server
      void receive(packet_code action) const;
      // receive text from server
      std::string receive_text(packet_code action) const;
      // receive data from server
      void receive_data(packet_code action, std::vector<int8_t>& data) const;

    protected:
      packet_kind _kind;
      packet_socket_t _socket;
    };

    // packet with credentials data
    struct credentials_info_t
    {
      uint8_t login_len;
      uint8_t password_len;
      char data[];
    };

    // server net api wrapper
    class server_api_t : public csnet_api_t
    {
    public:
      server_api_t(packet_kind kind = packet_kind::P_BASE_KIND);
      virtual ~server_api_t();

    public:
      // on accept
      void onaccept(packet_socket_t&& socket);
      // receive data from server
      bool receive();
      // is packet of the type
      bool is_packet_of(packet_type type, packet_code action) const;
      // get typed packet
      template<typename T>
      T* packet() const
      {
        return static_cast<T*>(_packet.get());
      }

    protected:
      std::unique_ptr<packet_info_t> _packet;
    };

    // client net api wrapper
    class client_api_t : public csnet_api_t
    {
      static constexpr int _CONNECT_ATTEMPT = 5; // what is count attempt to connect if server is busy?
      static constexpr int _WAIT_NEXT_CONNECT_ATTEMPT = 100; // time in ms to wait next attempt

    public:
      client_api_t(packet_kind kind = packet_kind::P_BASE_KIND);
      virtual ~client_api_t();

    public:
      // connect to server w/o credentials
      void connect(const std::string& host, int port, int connect_attempts = _CONNECT_ATTEMPT, int next_attempt = _WAIT_NEXT_CONNECT_ATTEMPT);
      // close connection
      void close();
      // check server connection
      uint64_t ping(uint64_t data) const;

    protected:
      // receive null reply from server
      virtual void receive_reply(packet_code action) const;
      // receive reply text from server
      virtual std::string receive_reply_text(packet_code action) const;
      // receive reply data from server
      virtual void receive_reply_data(packet_code action, std::vector<int8_t>& data) const;
    };

  }
}