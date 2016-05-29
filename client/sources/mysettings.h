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
    
private:
    int _port;
    std::string _host;
    std::string _login;
    std::string _password;
};

}