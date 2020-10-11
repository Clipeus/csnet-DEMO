#pragma once

#include <ctime>

#include "signals.h"
#include "csnet_api.h"

namespace csnet
{

  // hi-level server api implementation

  // service api interface
  class service_i
  {
  public:
    // ping command
    virtual uint64_t ping(uint64_t data) const = 0;
    // check client credentials
    virtual bool check_credentials(const std::string& login, const std::string& password) const = 0;
    // echo server command
    virtual std::string sendmsg(const std::string& msg) const = 0;
    // get current time command
    virtual std::time_t gettime() const = 0;
    // execute command
    virtual std::string execmd(const std::string& cmd) const = 0;
    // calculate command
    virtual std::string calculate(const std::string& input) const = 0;
  };

  // service api wrapper
  class srvapi_t : public shared::server_api_t
  {
  public:
    srvapi_t(shared::packet_kind kind = shared::packet_kind::P_BASE_KIND);
    virtual ~srvapi_t();

  public:
    // send reply to client
    void send_reply(shared::packet_code action) const;
    // send data reply to client
    void send_reply(shared::packet_code action, const void* data, size_t size) const;
    // send text reply to client
    void send_reply(shared::packet_code action, const std::string& text) const;
    // send error to client
    void send_reply(shared::packet_code action, uint32_t error, const std::string& text) const;
  };

}