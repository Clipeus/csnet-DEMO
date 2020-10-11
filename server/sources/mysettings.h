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
    static constexpr int _THREADS_ON_CORE = 2;
    static const int _MIN_THREAD_POOL;
    static constexpr int _MAX_THREAD_POOL = 1024;

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
    // is process daemon
    bool daemon() const
    {
      return _daemon;
    }
    void set_daemon(bool d)
    {
      _daemon = d;
    }
    // get net port
    int port() const
    {
      return _port;
    }
    // get thread pool count
    int pool_count() const
    {
      return _pool_count;
    }
    // get listen queue count
    int queue_count() const
    {
      return _queue_count;
    }
    // get is log disabled
    bool log_disabled() const
    {
      return _log_disabled;
    }
    // get logfile path
    std::string logfile() const
    {
      return _logfile;
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

  protected:
    // check values and correct
    virtual void check_values();

  private:
    bool _daemon;
    int _port;
    int _pool_count;
    int _queue_count;
    std::string _logfile;
    bool _log_disabled;
    std::string _login;
    std::string _password;
  };

}