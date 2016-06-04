
#ifdef _WIN32
#include <WS2tcpip.h>
#define socket_errno() WSAGetLastError()
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define closesocket(socket) close(socket)
#define socket_errno() errno
#endif

#include <cstring>
#include <sstream>
#include <array>

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
        set_error(socket_errno());
    
    return error() == 0;
}

// close handle 
void socket_t::close()
{
    if (_socket > 0)
    {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET_HANDLE;
    }
}

// close the handle and attach new socket handle
bool socket_t::attach(SOCKET_HANDLE socket)
{
    close();
    
    _socket = socket;
    
    std::array<int8_t, 1024> addr;
    socklen_t len = sizeof(addr);

    if (::getsockname(_socket, (sockaddr*)addr.data(), &len) < 0)
        set_error(socket_errno());
    else
        _family = ((sockaddr_in*)addr.data())->sin_family;
    
    return error() == 0;
}

// set or clear blocking socket
bool socket_t::set_blocking(bool blocking) const
{
#ifdef _WIN32
    unsigned long ul = 1;
    if (::ioctlsocket(_socket, FIONBIO, &ul) < 0)
#else
    if (::fcntl(_socket, F_SETFL, O_NONBLOCK) < 0)
#endif
        set_error(socket_errno());
    return error() == 0;
}

// initiate a connection on a socket
bool socket_t::connect(const sockaddr* addr, size_t len) const
{
    if (::connect(_socket, addr, len) < 0)
        set_error(socket_errno());
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
        set_error(socket_errno());
        return false;
    }
    return socket.attach(new_socket);
}

// bind a name to a socket
bool socket_t::bind(const sockaddr* addr, size_t len) const
{
   if (::bind(_socket, addr, len) < 0)
        set_error(socket_errno());
    return error() == 0;
}

//listen for connections on a socket
bool socket_t::listen(int queue) const
{
   if (::listen(_socket, queue) < 0)
        set_error(socket_errno());
    return error() == 0;
}

// cheack is socket ready to read
// if sec != -1 use timeout
// is sec == -1 w/o waiting
#ifdef _WIN32
bool socket_t::read_ready(int sec, int usec) const
#else
bool socket_t::read_ready(int sec, int usec, const sigset_t* sigmask) const
#endif
{
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_socket, &fds);

    timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

#ifdef _WIN32
    return is_ready(&fds, nullptr, nullptr, sec == -1 ? nullptr : &tv);
#else
    return is_ready(&fds, nullptr, nullptr, sec == -1 ? nullptr : &tv, sigmask);
#endif
}

// cheack is socket ready to write
// if sec != -1 use timeout
// is sec == -1 w/o waiting
#ifdef _WIN32
bool socket_t::write_ready(int sec, int usec) const
#else
bool socket_t::write_ready(int sec, int usec, const sigset_t* sigmask) const
#endif
{
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(_socket, &fds);

    timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

#ifdef _WIN32
    return is_ready(nullptr, &fds, nullptr, sec == -1 ? nullptr : &tv);
#else
    return is_ready(nullptr, &fds, nullptr, sec == -1 ? nullptr : &tv, sigmask);
#endif
}

// cheack is socket ready
// use 'pselect' to check it
#ifdef _WIN32
bool socket_t::is_ready(fd_set* rs, fd_set* ws, fd_set* es, timeval* tv) const
#else
bool socket_t::is_ready(fd_set* rs, fd_set* ws, fd_set* es, timeval* tv, const sigset_t* sigmask) const
#endif
{
#ifdef _WIN32
    if (::select(_socket + 1, rs, ws, es, tv) <= 0)
#else
    timespec* pts = nullptr;
    timespec ts;
    if (tv != nullptr)
    {
        ts.tv_sec = tv->tv_sec;
        ts.tv_nsec = tv->tv_usec * 1000;
        pts = &ts;
    }

    if (::pselect(_socket + 1, rs, ws, es, pts, sigmask) <= 0)
#endif
        set_error(socket_errno());
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
        num = ::recvfrom(_socket, reinterpret_cast<char*>(buf), size, flags, addr, (socklen_t*)len);
    else
        num = ::recv(_socket, reinterpret_cast<char*>(buf), size, flags);
    
    if (num == -1)
        set_error(socket_errno());
    
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
        num = ::sendto(_socket, reinterpret_cast<const char*>(buf), size, flags, addr, (socklen_t)len);
    else
        num = ::send(_socket, reinterpret_cast<const char*>(buf), size, flags);
    
    if (num == -1)
        set_error(socket_errno());
    
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
        _error_msg = socket_strerror(_error);
    
    return _error_msg; 
}

std::string socket_t::socket_strerror(int e) const
{
#ifdef _WIN32
    std::array<char, 1024> buffer;
    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, e, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), buffer.data(), 1024, nullptr))
        return "Unknown error";

    return buffer.data();
#else
    return strerror(_error);
#endif
}

}
}
