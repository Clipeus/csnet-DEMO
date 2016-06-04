#ifdef _WIN32
#else
#include <unistd.h>
#include <libgen.h>
#endif

#include <cstdio>
#include <cstdarg>
#include <thread>
#include <chrono>
#include <ctime>
#include <sstream>
#include <limits.h>
#include <cstdlib>

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
    std::string fn = findfile(filename);
    _file.open(fn, std::fstream::out | std::fstream::trunc);
    return (bool)_file;
}

// find log file
std::string logger_t::findfile(const std::string& filename) const
{
    std::string log = filename;
    
#ifdef _WIN32
#else
    if (log.empty()) // if file is not specified use exe-name + .log
    {
        char dest[PATH_MAX];
        if (readlink("/proc/self/exe", dest, PATH_MAX) != -1) // get process full path
        {
            // process name + '.log' OR log name w/o path
            log = (basename(dest) + std::string(".log"));
        }
    }
    else if (dirname(&log.front()) == nullptr || *dirname(&log.front()) == '.') // if dir is not specified use exe dir
    {
        char dest[PATH_MAX];
        if (readlink("/proc/self/exe", dest, PATH_MAX) != -1) // get process full path
        {
            // process name + '.log' OR log name w/o path
            std::string fn = basename(&log.front());
            // process path w/o name
            std::string dn = dirname(dest);
            
            //try to find it in the process path
            log = dn;
            log += "/" + fn;
        }
    }
#endif

    return log;
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
