#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#else
#include <unistd.h>
#include <libgen.h>
#include <sys/time.h>
#endif

#include <cstdio>
#include <cstdarg>
#include <thread>
#include <chrono>
#include <ctime>
#include <limits.h>
#include <cstdlib>
#include <array>
#include <iomanip>

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
    std::tr2::sys::path log_path(log);
    if (log.empty()) // if file is not specified use exe-name + .log
    {
        // get process full path
        std::array<char, MAX_PATH + 1> full_path;
        ::GetModuleFileNameA(::GetModuleHandle(nullptr), full_path.data(), MAX_PATH);

        std::tr2::sys::path path(full_path.data());
        log = path.parent_path().string() + '\\' + path.replace_extension(".log").filename().string();
    }
    else if (!log_path.has_root_path()) // if dir is not specified use exe dir
    {
        // get process full path
        std::array<char, MAX_PATH + 1> full_path;
        ::GetModuleFileNameA(::GetModuleHandle(nullptr), full_path.data(), MAX_PATH);

        std::tr2::sys::path path(full_path.data());
        log = path.parent_path().string() + '\\' + log_path.filename().string();
    }
#else
    if (log.empty()) // if file is not specified use exe-name + .log
    {
        char dest[PATH_MAX];
        if (readlink("/proc/self/exe", dest, PATH_MAX) != -1) // get process full path
        {
            // process path
            std::string fn = basename(dest);
            // process name
            std::string dn = dirname(dest);

            // build log file
            log = dn;
            log += "/" + fn;
            log += ".log";
        }
    }
    else if (dirname(&log.front()) == nullptr || *dirname(&log.front()) == '.') // if dir is not specified use exe dir
    {
        char dest[PATH_MAX];
        if (readlink("/proc/self/exe", dest, PATH_MAX) != -1) // get process full path
        {
            // process name
            std::string fn = basename(&log.front());
            // process name
            std::string dn = dirname(dest);
            
            // build log file
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
    int msec;

#ifdef _WIN32
    SYSTEMTIME systime;
    ::GetLocalTime(&systime);
    msec = systime.wMilliseconds;
#else
    timeval tv;
    gettimeofday(&tv, 0);
    msec = tv.tv_usec * 100;
#endif

    std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(today);
    
    tm* timeinfo;
    timeinfo = localtime(&now);
    
    std::stringstream buf;

    std::array<char, 80> time;
    strftime(time.data(), time.size(), "%Y-%m-%d %X", timeinfo);
    buf << time.data() << "." << std::setfill('0') << std::setw(3) << msec;
    
    return buf.str();
}

}
}
