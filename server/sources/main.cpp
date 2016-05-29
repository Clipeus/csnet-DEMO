#include <iostream>
#include <exception>

#include "daemon.h"

int main(int argc, char** args)
{
    try
    {
        csnet::daemon_t daemon(argc, args);
        return daemon.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Error occurred: " << "unexception error." << std::endl;
    }
    
    return -1;
}
