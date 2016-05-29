#include <cstdio>
#include <cstdarg>
#include <thread>
#include <chrono>
#include <ctime>
#include <sstream>

#include "logger.h"

namespace csnet
{
namespace shared
{

logger_t logger_t::_instance;

logger_t::logger_t()
{
}

logger_t::~logger_t()
{
    close();
}

// open logfile by name
bool logger_t::open(const std::string& filename)
{
    _file.open(filename, std::fstream::out | std::fstream::trunc);
    return _file;
}

// close logfile
void logger_t::close()
{
    _file.close();
}

// log out like a printf format
void logger_t::logout(const char* format, ...)
{
    if (opened())
    {
        std::lock_guard<std::mutex> lck (_mtx);
        
        // get buffer size
        va_list args;
        va_start(args, format);
        size_t size = vsnprintf(nullptr, 0, format, args) + 1;
        
        // allocate buffer
        std::string str;
        str.resize(size);
        
        // format message
        va_start(args, format);
        vsnprintf(&str.front(), str.size(), format, args);
       
        // write to file
        _file << cur_time() << ", thread id: " << thread_id() << ". " << str.c_str();
        _file.flush();
        
        if (_stdout)
            std::clog << str.c_str(); // double a log to stdout
        
        va_end(args);
    }
}

// log out like C++ stream w/o doubling to stdout

std::ostream& logger_t::logout()
{
    _file << cur_time() << ", thread id: " << thread_id() << ". ";
    return _file;
}

// return current system time
std::string logger_t::thread_id() const
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

// return current thread id
std::string logger_t::cur_time() const
{
    std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(today);
    
    tm* timeinfo;
    timeinfo = localtime(&now);
    
    std::string time;
    time.resize(80);
    size_t size = strftime(&time.front(), time.size(), "%Y-%m-%d %X", timeinfo); 
    time.resize(size);
    
    return time;
}

}
}
