#ifdef _WIN32
#else
#include <unistd.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>

#include "logger.h"
#include "server.h"
#include "daemon.h"
#include "mysettings.h"

namespace csnet
{

using namespace shared;

daemon_t::daemon_t(int argc, char** args) : _argc(argc), _args(args)
{
    mysettings_t::instance()->load();
    logger_t::instance()->use_stdout(true);
}

daemon_t::~daemon_t()
{
    LOGLINE("Server finished.");
}

int daemon_t::run()
{
    std::cout << "Demo Server Application, version 1.0" << std::endl;
    
    if (!parse_cmd())
        return -1;
    
    logger_t::instance()->open(mysettings_t::instance()->logfile());

    if (mysettings_t::instance()->daemon())
        LOGLINE("Server started as deamon application.");
    else
        LOGLINE("Server started as console application.");
    
    return process();
}

bool daemon_t::parse_cmd()
{
    if (_argc <= 1)
    {
        std::cout << "Usage ./myserver -d for daemon or ./myserver -i for interactive" << std::endl;
        return false;
    }
    else if (std::strcmp(_args[1], "-i") == 0)
    {
        mysettings_t::instance()->set_daemon(false);
        return true;
    }
    else if (std::strcmp(_args[1], "-d") == 0)
    {
#ifdef _WIN32
        std::cerr << "Cannot create daemon in Windows" << std::endl;
        return false;
#else
        if (daemon(0, 0) == -1)
        {
            std::cerr << "daemon failed" << std::endl;
            return false;
        }
        mysettings_t::instance()->set_daemon(true);
        logger_t::instance()->use_stdout(false);
        return true;
#endif
    }
    else
    {
        //std::cout << "Usage ./test -d for daemon or ./test -i for interactive" << std::endl;
        std::cout << "Invalid agrument '" << _args[1] << "'" << std::endl;
        return false;
    }
}

int daemon_t::process()
{
    server_t server;
    return server.run();
}

}