#include <cstring>
#include <sstream>

#include "srvapi.h"
#include "threadpool.h"
#include "logger.h"

namespace csnet
{

  using namespace shared;

  // service api wrapper
  srvapi_t::srvapi_t(shared::packet_kind kind) : server_api_t(kind)
  {
  }

  srvapi_t::~srvapi_t()
  {
  }

  // send reply to server
  void srvapi_t::send_reply(packet_code action) const
  {
    send(action | packet_code::P_RETURN_ACTION);
  }

  // send data reply to client
  void srvapi_t::send_reply(packet_code action, const void* data, size_t size) const
  {
    send(action | packet_code::P_RETURN_ACTION, data, size);
  }

  // send text reply to client
  void srvapi_t::send_reply(packet_code action, const std::string& text) const
  {
    send(action | packet_code::P_RETURN_ACTION, text);
  }

  // send error to server
  void srvapi_t::send_reply(packet_code action, uint32_t error, const std::string& text) const
  {
    send(action | packet_code::P_RETURN_ACTION, error, text);
  }

}
