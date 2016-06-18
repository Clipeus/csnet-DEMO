#ifdef _WIN32
#include <Winsock2.h>
#define socket_errno() WSAGetLastError()
#undef min
#undef max
#else
#include <sys/socket.h>
#endif

#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <thread>

#include "mysettings.h"
#include "cfgparser.h"

namespace csnet
{

using namespace shared;

// static instance
template <> std::unique_ptr<mysettings_t, singleton<mysettings_t>::deleter> singleton<mysettings_t>::_instance = nullptr;

int mysettings_t::_MIN_THREAD_POOL = std::max<int>(std::thread::hardware_concurrency() * _THREADS_ON_CORE, _THREADS_ON_CORE);

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
    std::unique_ptr<mysettings_t, singleton<mysettings_t>::deleter> temp(new mysettings_t(new cfgparser_t("server.cfg")));
    _instance = std::move(temp);
    return _instance.get();
}

// load settings
void mysettings_t::load()
{
    std::string val = get_value("connect", "port");
    _port = std::atoi(val.c_str());
    
    _login = get_value("connect", "login");
    _password = get_value("connect", "password");

    val = get_value("behavior", "pool_count");
    _pool_count = std::atoi(val.c_str());
    val = get_value("behavior", "queue_count");
    _queue_count = std::atoi(val.c_str());

    _logfile = get_value("debug", "logfile");

    val = get_value("debug", "log_disabled");
    _log_disabled = to_bool(val);

    check_values();
}

// save settings
void mysettings_t::save()
{
    /*set_value("connect", "port", val);
    
    std::stringstream val;
    val << _port;
    set_value("debug", "logfile", val.str());*/
}

// set default values
void mysettings_t::reset()
{
    _daemon = false;
    _port = 0;
    _pool_count = _MIN_THREAD_POOL;
    _queue_count = _MIN_THREAD_POOL;
    _logfile.clear();
    _login.clear();
    _password.clear();
}

void mysettings_t::check_values()
{
    if (_pool_count > _MAX_THREAD_POOL)
        _pool_count = _MAX_THREAD_POOL;

    if (_queue_count > _MAX_THREAD_POOL)
        _queue_count = _MAX_THREAD_POOL;

    if (_pool_count == 0)
        _pool_count = _MIN_THREAD_POOL;

    if (_queue_count == 0)
        _pool_count = _MIN_THREAD_POOL;
}

}
