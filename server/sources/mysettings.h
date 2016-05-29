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
    // get logfile path
    std::string logfile() const
    {
        return _logfile;
    }
    
private:

    bool _daemon;
    int _port;
    int _pool_count;
    std::string _logfile;
};

}