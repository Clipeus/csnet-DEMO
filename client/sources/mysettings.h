#pragma once

#include "singleton.h"
#include "settings.h"

namespace csnet
{

  // provide application settings class
  class mysettings_t : public shared::settings_t, public shared::singleton<mysettings_t>
  {
    friend struct deleter;
    template<class U> friend class shared::singleton;

    static constexpr int _CONNECT_ATTEMPT = 5; // what is count attempt to connect if server is busy?
    static constexpr int _WAIT_NEXT_CONNECT_ATTEMPT = 100; // time in ms to wait next attempt

  protected:
    mysettings_t(csnet::shared::settings_provider_t* provider);
    ~mysettings_t();

    // create standalone object
    static mysettings_t* create();

  public:
    // load settings
    void load();
    // save settings
    void save();
    // set default values
    void reset();

  public:
    //get host name
    std::string host() const
    {
      return _host;
    }
    // get net port
    int port() const
    {
      return _port;
    }
    // get user login
    std::string login() const
    {
      return _login;
    }
    // get user password
    std::string password() const
    {
      return _password;
    }
    // get count attempt to connect if server is busy?
    int connect_attempts() const
    {
      return _connect_attempts;
    }
    // get time in ms to wait next attempt
    int next_attempt() const
    {
      return _next_attempt;
    }

  protected:
    // check values and correct
    virtual void check_values();

  private:
    int _port;
    std::string _host;
    std::string _login;
    std::string _password;
    int _connect_attempts;
    int _next_attempt;
  };

}