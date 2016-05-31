#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>

#include "socket.h"

namespace csnet
{
namespace shared
{

// default constuctor
socket_t::socket_t()
{
}

// move constructor (copy constructor is not allowed)
socket_t::socket_t(const socket_t&& socket)
{
    move(socket);
}

// create from socket handle
socket_t::socket_t(int socket)
{
    attach(socket);
}

// destructor, clear resource
socket_t::~socket_t()
{
    close();
}

// move socket from other and than detach it
socket_t& socket_t::operator=(const socket_t&& socket)
{
    if (&socket != this)
        move(socket);

    return *this;
}

// move socket from other and than detach it
void socket_t::move(const socket_t& socket)
{
    close();
    
    _socket = socket._socket;
    _family = socket._family;
    _send_timeout = socket._send_timeout;
    _recv_timeout = socket._recv_timeout;

    _error = socket._error;
    _error_msg = socket._error_msg;
    
    // need to detach source socket
    socket_t* s = const_cast<socket_t*>(&socket);
    _socket = s->detach();
}

// create socket object
bool socket_t::create(int family, int type, int protocol)
{
    _family = family;
    
    _socket = ::socket(_family, type, protocol);
    if (_socket == INVALID_SOCKET_HANDLE)
        set_error(errno);
    
    return error() == 0;
}

// close handle 
void socket_t::close()
{
    if (_socket > 0)
    {
        ::close(_socket);
        _socket = INVALID_SOCKET_HANDLE;
    }
}

// close the handle and attach new socket handle
bool socket_t::attach(int socket)
{
    close();
    
    _socket = socket;
    
    int8_t addr[1024];
    socklen_t len = sizeof(addr);

    if (::getsockname(_socket, (sockaddr*)addr, &len) < 0)
        set_error(errno);
    else
        _family = ((sockaddr_in*)addr)->sin_family;
    
    return error() == 0;
}

// set or clear blocking socket
bool socket_t::set_blocking(bool blocking) const
{
    if (::fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
        set_error(errno);
    return error() == 0;
}

// initiate a connection on a socket
bool socket_t::connect(const sockaddr* addr, size_t len) const
{
    if (::connect(_socket, addr, len) < 0)
        set_error(errno);
    return error() == 0;
}

// initiate a connection on a socket
bool socket_t::connect(const std::string& host, uint16_t port) const
{
    // getting ip-address from host name
    hostent* he = gethostbyname(host);
    if (nullptr == he)
        return false;
    
    //initiate an internet socket address
    sockaddr_in sin;
    std::copy(reinterpret_cast<char*>(he->h_addr), 
        reinterpret_cast<char*>(he->h_addr) + he->h_length, reinterpret_cast<char*>(&sin.sin_addr.s_addr));
    sin.sin_family = _family;
    sin.sin_port = htons(port);
    
    return connect(reinterpret_cast<sockaddr*>(&sin), sizeof(sin));
}

// accept a connection on a socket
bool socket_t::accept(socket_t& socket, sockaddr* addr, size_t* len) const
{
    int new_socket = ::accept(_socket, addr, (socklen_t*)len);
    if (new_socket < 0)
    {
        set_error(errno);
        return false;
    }
    return socket.attach(new_socket);
}

// bind a name to a socket
bool socket_t::bind(const sockaddr* addr, size_t len) const
{
   if (::bind(_socket, addr, len) < 0)
        set_error(errno);
    return error() == 0;
}

//listen for connections on a socket
bool socket_t::listen(int queue) const
{
   if (::listen(_socket, queue) < 0)
        set_error(errno);
    return error() == 0;
}

// cheack is socket ready to read
// if sec != -1 use timeout
// is sec == -1 w/o waiting
bool socket_t::read_ready(int sec, int nsec, const sigset_t* sigmask) const
{
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_socket, &fds);

    timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = nsec;
    
    return is_ready(&fds, nullptr, nullptr, sec == -1 ? nullptr : &ts, sigmask);
}

// cheack is socket ready to write
// if sec != -1 use timeout
// is sec == -1 w/o waiting
bool socket_t::write_ready(int sec, int nsec, const sigset_t* sigmask) const
{
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_socket, &fds);

    timespec ts;
    ts.tv_sec  = sec;
    ts.tv_nsec = nsec;

    return is_ready(nullptr, &fds, nullptr, sec == -1 ? nullptr : &ts, sigmask);
}

// cheack is socket ready
// use 'pselect' to check it
bool socket_t::is_ready(fd_set* rs, fd_set* ws, fd_set* es, timespec* ts, const sigset_t* sigmask) const
{
    if (::pselect(_socket + 1, rs, ws, es, ts, sigmask) <= 0)
        set_error(errno);
    return error() == 0;
}

// receive a message from a socket
size_t socket_t::receive(void* buf, size_t size, int flags, sockaddr* addr, size_t* len) const
{
    if (size == 0)
        return 0;
    
    // if read timeout is set will check is whether data available 
    if (receive_timeout() >= 0 && !read_ready(receive_timeout()))
      return 0;

    int num = 0;
    if (addr != nullptr && len != 0)
        num = ::recvfrom(_socket, buf, size, flags, addr, (socklen_t*)len);
    else
        num = ::recv(_socket, buf, size, flags);
    
    if (num == -1)
        set_error(errno);
    
    return num;
}

// send a message on a socket
size_t socket_t::send(const void* buf, size_t size, int flags, const sockaddr* addr, size_t len) const
{
    if (size == 0)
        return 0;
    
    // if write timeout is set will check is ready to write
    if (send_timeout() >= 0 && !write_ready(send_timeout()))
      return 0;

    int num = 0;
    if (addr != nullptr && len != 0)
        num = ::sendto(_socket, buf, size, flags, addr, (socklen_t)len);
    else
        num = ::send(_socket, buf, size, flags);
    
    if (num == -1)
        set_error(errno);
    
    return num;
}

// cheack is socket ready
hostent* socket_t::gethostbyname(const std::string& name) const
{
    hostent* he = ::gethostbyname(name.c_str());
    if (he == nullptr)
    {
        // build error string
        std::stringstream buf;
        switch (h_errno)
        {
            case HOST_NOT_FOUND:
            buf << "The host \"" << name << "\" is unknown.";
            break;
            
            case NO_ADDRESS:
            //case NO_DATA:
            buf << "The host \"" << name << "\" is valid but does not have an IP address.";
            break;
            
            case NO_RECOVERY:
            buf << "A nonrecoverable name server error occurred.";
            break;
            
            //case TRY_AGAIN:
            default:
            buf << "A temporary error occurred on an authoritative name server. Try again later.";
            break;
        }
        _error = EADDRNOTAVAIL;
        _error_msg = buf.str();
    }
    return he;
}

// set last error socket
std::string socket_t::error_msg() const                   
{
    if (!_error_msg.empty())
        return _error_msg;
    
    if (_error != 0)
        _error_msg = strerror(_error);
    
    return _error_msg; 
}

}
}
