#include <cstdlib>
#include <sstream>
#include "mysettings.h"
#include "cfgparser.h"

namespace csnet
{

using namespace shared;

// static instance
template <> std::unique_ptr<mysettings_t, singleton<mysettings_t>::deleter> singleton<mysettings_t>::_instance = nullptr;

mysettings_t::mysettings_t(csnet::shared::settings_provider_t* provider) : settings_t(provider)
{
    reset();
}

mysettings_t::~mysettings_t()
{
    delete _provider;
}

// create standalone object
mysettings_t* mysettings_t::create()
{
    std::unique_ptr<mysettings_t, singleton<mysettings_t>::deleter> temp(new mysettings_t(new cfgparser_t("client.cfg")));
    _instance = std::move(temp);
    return _instance.get();
}

// load settings
void mysettings_t::load()
{
    _host = get_value("connect", "host");
    
    std::string val = get_value("connect", "port");
    _port = std::atoi(val.c_str());
    
    _login = get_value("connect", "login");
    _password = get_value("connect", "password");
}

// save settings
void mysettings_t::save()
{
}

// set default values
void mysettings_t::reset()
{
    _port = 0;
    _host.clear();
    _login.clear();
    _password.clear();
}

}