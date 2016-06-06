#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>

namespace csnet
{
namespace shared
{

#define STREAM(esc) \
do \
{ \
    std::stringstream buf; \
    buf << esc; \
    buf.str(); \
} while (false)

#define LOGOUT(seq) \
do \
{ \
    std::stringstream buf; \
    buf << seq; \
    shared::logger_t::instance()->logout(buf.str()); \
} while (false)

#define LOGLINE(seq) \
do  \
{ \
    std::stringstream buf; \
    buf << seq << std::endl; \
    shared::logger_t::instance()->logout(buf.str()); \
} while (false)
    
    // log out class
class logger_t
{
protected:
    logger_t();
    ~logger_t();

    // do not allow copy and move global singleton logger object

    logger_t(const logger_t& rhs) = delete;
    logger_t(logger_t&& rhs) = delete;

    logger_t& operator = (const logger_t& rhs) = delete;
    logger_t& operator = (logger_t&& rhs) = delete;

public:
    // open logfile by name
    bool open(const std::string& filename);
    // close logfile
    void close();
    
public:
    // is logfile opened
    bool opened() const
    {
        return _file.is_open();
    }
    // get instance to use logger
    static logger_t* instance()
    {
        return &_instance;
    }
    // double a log to stdout
    void use_stdout(bool out)
    {
        _stdout = out;
    }
    
    // log out like a printf format
    void logout(const char* format, ...);
    // log out
    void logout(const std::string& log)
    {
        logout(log.c_str());
    }

private:
    // find log file
    std::string findfile(const std::string& filename) const;
    // return current system time
    std::string cur_time() const;
    // return current thread id 
    std::string thread_id() const;
    
protected:
    // multithreading locker
    std::mutex _mtx;
    std::ofstream _file;
    bool _stdout = false;
    static logger_t _instance;
};

}
}
