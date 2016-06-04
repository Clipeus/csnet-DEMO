#pragma once

#include <iostream>
#include <fstream>
#include <mutex>

namespace csnet
{
namespace shared
{
    
// log out class
class logger_t
{
protected:
    logger_t();
    ~logger_t();
    
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
    // log out like C++ stream w/o doubling to stdout
    std::ostream& logout();
 
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
