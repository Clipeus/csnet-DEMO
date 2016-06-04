#pragma once

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#endif

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace csnet
{
namespace shared
{

// socket wrapper class
class socket_t
{
public:

#ifdef _WIN32
    typedef SOCKET SOCKET_HANDLE;
#else
    typedef int SOCKET_HANDLE;
#endif

    static constexpr SOCKET_HANDLE INVALID_SOCKET_HANDLE = -1;

public:
    // default constuctor
    socket_t();
    // move constructor (copy constructor is not allowed)
    socket_t(const socket_t&& socket);
    // create from socket handle
    explicit socket_t(int socket);
    // destructor, clear resource
    virtual ~socket_t();

    // move socket from other and than detach it
    socket_t& operator=(const socket_t&& socket);
    // create socket object
    bool create(int family = AF_INET, int type = SOCK_STREAM, int protocol = 0);
    // close handle 
    void close();
    // set or clear blocking socket
    bool set_blocking(bool blocking) const;

public:
    // initiate a connection on a socket
    bool connect(const sockaddr* addr, size_t len) const;
    bool connect(const std::string& host, uint16_t port) const;
    // accept a connection on a socket
    bool accept(socket_t& socket, sockaddr* addr = nullptr, size_t* len = nullptr) const;
    // bind a name to a socket
    bool bind(const sockaddr* addr = nullptr, size_t len = 0) const;
    //listen for connections on a socket
    bool listen(int queue) const;
    
    // cheack is socket ready to read
    // if sec != -1 use timeout
    // is sec == -1 w/o waiting
#ifdef _WIN32
    bool read_ready(int sec = -1, int usec = 0) const;
#else
    bool read_ready(int sec = -1, int usec = 0, const sigset_t* sigmask = nullptr) const;
#endif
    // cheack is socket ready to write
    // if sec != -1 use timeout
    // is sec == -1 w/o waiting
#ifdef _WIN32
    bool write_ready(int sec = -1, int usec = 0) const;
#else
    bool write_ready(int sec = -1, int usec = 0, const sigset_t* sigmask = nullptr) const;
#endif

    // receive a message from a socket
    size_t receive(void *buf, size_t size, int flags = 0, sockaddr* addr = nullptr, size_t* len = nullptr) const;
    // send a message on a socket
    size_t send(const void* buf, size_t size, int flags = 0, const sockaddr* addr = nullptr, size_t len = 0) const;
    
public:
    // receive a data from a socket
    // use vector<int8_t> as ingoing buffer
    size_t receive(std::vector<int8_t>& buf, size_t size = -1, int flags = 0, sockaddr* addr = nullptr, size_t* len = nullptr) const
    {
        if (size != -1)
        {
            // allocate buffer
            buf.resize(size);
            if (buf.size() == 0)
                return 0;
            
            // read a fix size data
            size_t num = receive(buf.data(), buf.size() * sizeof(int8_t), flags, addr, len);
            if (num >= 0)
                buf.resize(num);
            return num;
        }
        else
        {
            // read a variable size data
            //int8_t buf_temp[_buffsize];
            std::array<int8_t, _buffsize> buf_temp;
            size_t num = 0;
            
            do
            {
                // read chunk of data
                num = receive(buf_temp.data(), _buffsize, flags, addr, len);
                if (num > 0)
                    buf.insert(buf.cend(), buf_temp.cbegin(), buf_temp.cend());
            }
            while (num > 0 && num == _buffsize);
            
            if (num < 0)
            {
                //error occured
                set_error(errno);
                return num;
            }
            return buf.size();
        }
    }
    
    // receive a string from a socket
    // use std::string as ingoing buffer
    size_t receive(std::string& str, size_t size = -1, int flags = 0, sockaddr* addr = nullptr, size_t* len = nullptr) const
    {
        if (size != -1)
        {
            // allocate buffer
            str.resize(size);
            if (str.size() == 0)
                return 0;
            
            // read a fix size string
            size_t num = receive(&str.front(), str.size(), flags, addr, len);
            if (num >= 0)
                str.resize(num);
            return num;
        }
        else
        {
            // read a variable size string
            //char buf_temp[_buffsize + 1];
            std::array<char, _buffsize + 1> buf_temp;
            size_t num = 0;
            size_t size = 0;
            
            do
            {
                // read chunk of string
                num = receive(buf_temp.data(), _buffsize, flags, addr, len);
                if (num > 0)
                {
                    buf_temp[num] = 0;
                    str += buf_temp.data();
                    size += num;
                }
            }
            while (num > 0 && num == _buffsize);

            
            if (num < 0)
            {
                //error occured
                set_error(errno);
                return num;
            }
            return size;
        }
    }
    
    // send a data on a socket
    // use vector<int8_t> as outgoing buffer
    size_t send(const std::vector<int8_t>& buf, int flags = 0, const sockaddr* addr = nullptr, size_t len = 0) const
    {
        return send(buf.data(), buf.size() * sizeof(int8_t), flags, addr, len);
    }
    
    // send a string on a socket
    // use std::string as outgoing buffer
    size_t send(const std::string str, int flags = 0, const sockaddr* addr = nullptr, size_t len = 0) const
    {
        return send(str.c_str(), str.size(), flags, addr, len);
    }

public:
    // close the handle and attach new socket handle
    bool attach(SOCKET_HANDLE socket);
    // return the socket handle and detach it
    SOCKET_HANDLE detach()
    {
        int s = socket();
        _socket = INVALID_SOCKET_HANDLE;
        return s;
    }
    // return the socket handle
    SOCKET_HANDLE socket() const
    {
        return _socket;
    }
    // return the socket handle
    operator SOCKET_HANDLE () const
    {
        return socket();
    }
    // return the read timeout
    int receive_timeout() const              
    { 
        return _recv_timeout; 
    }
    // return the write timeout
    int send_timeout() const
    {
        return _send_timeout; 
    }
    // set new read timeout
    void set_receive_timeout(int timeout)
    { 
        _recv_timeout = timeout;
    }
    // set new write timeout
    void set_send_timeout(int timeout)
    {
        _send_timeout = timeout; 
    }
    // get last socket error
    int error() const                   
    { 
        return _error; 
    }
    // get last socket error message
    std::string error_msg() const;
    
protected:
    // cheack is socket ready
#ifdef _WIN32
    bool is_ready(fd_set* rs, fd_set* ws = nullptr, fd_set* es = nullptr, timeval* tv = nullptr) const;
#else
    bool is_ready(fd_set* rs, fd_set* ws = nullptr, fd_set* es = nullptr, timeval* tv = nullptr, const sigset_t* sigmask = nullptr) const;
#endif
    // get ip address by host name
    hostent* gethostbyname(const std::string& name) const;
    // move socket from other and than detach it
    void move(const socket_t& socket);
    // set last error socket
    void set_error(int e) const
    {
        _error = e;
        _error_msg.clear();
    }
    std::string socket_strerror(int e) const;

protected:
    // socket handle
    SOCKET_HANDLE _socket = INVALID_SOCKET_HANDLE;
    // communication family
    int _family = AF_INET;
    // read timeout
    int _send_timeout = -1;
    // write timeout
    int _recv_timeout = -1;
    // temp buffer size
    static constexpr size_t _buffsize = 128;
    // last error socket
    mutable int _error = 0;
    // last socket error message
    mutable std::string _error_msg;
};

}
}
