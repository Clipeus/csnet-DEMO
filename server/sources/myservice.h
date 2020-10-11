#pragma once

#include "srvapi.h"

namespace csnet
{

  class myservice_t : public service_i
  {
  public:
    myservice_t();
    ~myservice_t();

  public:
    // ping command
    uint64_t ping(uint64_t data) const;
    // check client credentials
    bool check_credentials(const std::string& login, const std::string& password) const;
    // echo server command
    std::string sendmsg(const std::string& msg) const;
    // get current time command
    std::time_t gettime() const;
    // execute command
    std::string execmd(const std::string& cmd) const;
    // calculate command
    std::string calculate(const std::string& input) const;

  protected:
    // execute command and get its result
    std::string exec(const std::string& cmd) const;
  };

}