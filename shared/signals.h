#pragma once

#ifdef _WIN32
#include <sstream>
#else
#endif

#include <csignal>
#include <cstring>
#include <utility>
#include <set>

namespace csnet
{
namespace shared
{

// linux signal class helper
template <class T>
class signal_t
{
typedef void (T::*onsignal_ptr_t)(const signal_t*, int); // signal handler pointer

public:
    // set class object and signal handler pointer
    signal_t(T* parent, onsignal_ptr_t onsignal) : _parent(parent), _onsignal(onsignal)
    {
    }
    
    ~signal_t()
    {
        // clear all handlers
        for (auto it = _sigdata.cbegin(); it != _sigdata.cend(); )
        {
            // find own handlers
            if (it->first == this)
            {
                std::signal(it->second, SIG_DFL);
                _sigdata.erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }
    
public:
    // Set signal handler to handle signal
    bool connect(int signal)
    {
        if (_sigdata.end() == _sigdata.find(std::make_pair(this, signal)))
        {
            _sigdata.insert(std::make_pair(this, signal));
            std::signal(signal, signal_t<T>::signal_handler);
            return true;
        }
        return false;
    }

    // clear signal handler
    bool disconnect(int signal)
    {
        if (_sigdata.end() != _sigdata.find(std::make_pair(this, signal)))
        {
            // set a default handle
            std::signal(signal, SIG_DFL);
            _sigdata.erase(std::make_pair(this, signal));
            return true;
        }
        return false;
    }
    
    // gt signal name
    std::string signame(int signal) const
    {
#ifdef _WIN32
        std::stringstream msg;
        msg << "Windows signal #" << signal;
        return msg.str();
#else
        return strsignal(signal);
#endif
    }

    // internal signal handler
    virtual void onsignal(int signal)
    {
        (_parent->*_onsignal)(this, signal); // forward to parent object
    }

protected:
    // parent object
    T* _parent = nullptr;
    // signal handlet method of a parent object
    onsignal_ptr_t _onsignal = nullptr;

protected:
    // internal signal handler
    static void signal_handler(int signal)
    {
        for (auto it: _sigdata)
        {
            if (it.second == signal)
                it.first->onsignal(it.second);
        }
    }
    // store all signal handlers
    static std::set<std::pair<signal_t<T>*, int> > _sigdata;
};

template <class T>
std::set<std::pair<signal_t<T>*, int> > signal_t<T>::_sigdata;

}
}
