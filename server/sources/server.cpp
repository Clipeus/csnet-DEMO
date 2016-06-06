#ifdef _WIN32
#else
#include <unistd.h>
#endif

#include <stdexcept>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>
#include <chrono>
#include <ctime>

#include "server.h"
#include "logger.h"
#include "threadpool.h"
#include "mysettings.h"
#include "packsock.h"

namespace csnet
{

using namespace shared;

server_t::server_t() : _signal(this, &server_t::onsignal)
{
#ifdef _WIN32
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

server_t::~server_t()
{
    close();

#ifdef _WIN32
    if  (_cancel == INVALID_SOCKET)
        ::closesocket(_cancel);
    ::WSACleanup();
#endif
}

void server_t::close()
{
    // nothink to close
}

void server_t::init_socket()
{
    if (!_listener.create())
    {
        std::stringstream buf;
        buf << "Socket creating failed: " << _listener.error_msg();
        throw std::runtime_error(buf.str());
    }
    
    _listener.set_unblocking(true);
    
    // init and bind socket
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mysettings_t::instance()->port());
    addr.sin_addr.s_addr = INADDR_ANY;
    if (!_listener.bind((sockaddr *)&addr, sizeof(addr)))
    {
        std::stringstream buf;
        buf << "Socket binding failed: " << _listener.error_msg();
        throw std::runtime_error(buf.str());
    }

    _listener.listen(mysettings_t::instance()->pool_count() * 2);
}

void server_t::init_signal()
{
    // connect to signals

#ifdef _WIN32
    // init cancel event for Ctrl+C signal
    // need to notify select() to exit
    _cancel = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    _signal.connect(SIGQUIT);
#endif

    _signal.connect(SIGINT);
    _signal.connect(SIGTERM);
    //_signal.connect(SIGHUP);
}

// signals handler
void server_t::onsignal(const signal_t<server_t>* sender, int signal)
{
    LOGLINE("Signal occurred: " << sender->signame(signal).c_str() << " (" << signal << ").");

#ifdef _WIN32
    if (signal == SIGINT || signal == SIGTERM)
#else
    if (signal == SIGQUIT || signal == SIGINT || signal == SIGTERM)
#endif
    {
        // need to exit
        _finished = true;

#ifdef _WIN32
        // set cancel event to exit
        if (_cancel != INVALID_SOCKET)
        {
            // notify select() to exit
            ::closesocket(_cancel);
            _cancel = INVALID_SOCKET;
        }
#endif
    }
    /*else if (signal == SIGHUP)
    {
    }*/
}

// server main routine
int server_t::run()
{
    int status = 0;

    try
    {
        init_socket();
        init_signal();
        
        // init thread pool by threads number
        thread_pool_t pool(mysettings_t::instance()->pool_count());
        
        // main server loop
        while (!is_finished())
        {
            LOGLINE("Waitnig for connection.");

            // wait socket data to read
#ifdef _WIN32
            int ret = _listener.read_ready(-1, 0, _cancel);
#else
            int ret = _listener.read_ready();
#endif
            if (ret < 0 || is_finished())
            {
                if (is_finished())
                {
                    LOGLINE("Leave the server loop.");
                    break;
                }
                else
                {
                    std::stringstream buf;
                    buf << "Socket selecting failed, errno: " << _listener.error_msg();
                    throw std::runtime_error(buf.str());
                }
            }
            else if (ret)
            {
                LOGLINE("Accepting socket.");
                packet_socket_t accepted;
                if (!_listener.accept(accepted))
                {
                    std::stringstream buf;
                    buf << "Socket accepting failed: " << _listener.error_msg();
                    throw std::runtime_error(buf.str());
                }

                socket_t::SOCKET_HANDLE hsocket = accepted.detach();
                
                LOGLINE("Add job to pool.");
                pool.enqueue([this, hsocket] // handle net request
                {
                    packet_socket_t socket(hsocket);
                    // thread code
                    LOGLINE("Recieving socket data.");
                    
                    // read packet
                    std::unique_ptr<packet_info_t> packet(socket.receive());
                    if (packet == nullptr)
                    {
                        LOGLINE((socket.error_msg().empty() ? "There is no any data recieved." : socket.error_msg()));
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_TEXT_TYPE && packet->action == packet_code::P_ECHO_ACTION)
                    {
                        // it is echo server action
                        LOGLINE("Demo echo server action.");
                        packet_text_t* packet_text = static_cast<packet_text_t*>(packet.get());
                        LOGLINE("Recieved text: " << packet_text->text << ".");

                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // set string to upper
                        std::transform(packet_text->text, packet_text->text + std::strlen(packet_text->text), packet_text->text, ::toupper);                            
                        packet_text->action |= packet_code::P_RETURN_ACTION;
                        
                        LOGLINE("Send text: " << packet_text->text << ".");
                        
                        // replay string to client
                        if (!socket.send(packet_text))
                            LOGLINE("Sending fialed: " << socket.error_msg() << ".");
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_DATA_TYPE && packet->action == packet_code::P_TIME_ACTION)
                    {
                        // it is gettime action
                        LOGLINE("Demo get time action.");
                        
                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // get current server time
                        std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
                        std::time_t now = std::chrono::system_clock::to_time_t(today);
                        
                        LOGLINE("Send time.");
                        
                        // replay time to client
                        if (!socket.send(packet_info_t(packet->kind, packet->type, packet->action | packet_code::P_RETURN_ACTION), (int8_t*)&now, sizeof(now)))
                            LOGLINE("Sending fialed: " << socket.error_msg() << ".");
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_TEXT_TYPE && packet->action == packet_code::P_EXECMD_ACTION)
                    {
                        // it is execute cmd action
                        LOGLINE("Demo execute command.");
                        packet_text_t* packet_text = static_cast<packet_text_t*>(packet.get());
                        LOGLINE("Recieved command data: " << packet_text->text << ".");

                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // execute command and get command's result
                        std::string result = exec(packet_text->text);
                        
                        LOGLINE("Send command resul: " << result << ".");
                        
                        // replay command's result to client
                        if (!socket.send(packet_info_t(packet->kind, packet->type, packet->action | packet_code::P_RETURN_ACTION), result))
                            LOGLINE("Sending fialed: " << socket.error_msg() << ".");
                    }
                    else
                    {
                        // it is unknown action
                        LOGLINE("Recieved unknown packet.");
                        
                        if (socket.send(packet_info_t(packet->kind, packet_type::P_ERROR_TYPE, packet->action | packet_code::P_RETURN_ACTION), "Unknown packet"))
                            LOGLINE("Sending fialed: " << socket.error_msg() << ".");
                    }
                });
            }
            
            if (is_finished())
                break;
        }

        // wait and close all thread tasks
        pool.close(true, true);
    }
    catch (std::exception& e)
    {
        LOGLINE("Error occurred: " << e.what());
        status = -1;
    }
    catch (...)
    {
        LOGLINE("Error occurred: " << "unexception error.");
        status = -2;
    }

    return status;
}

// true if need to exit
bool server_t::is_finished()
{
    LOGLINE("is_finished: " << _finished << ".");
    return _finished;
}

// execute command and get its result
std::string server_t::exec(const std::string& cmd) 
{
    std::string result = "";
    
#ifdef _WIN32
    result = "Execute command is not implementet on Windows yet.";
#else
    std::array<char, 128 + 1> buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) 
        throw std::runtime_error("popen() failed!");
    
    try 
    {
        while (!feof(pipe)) 
        {
            if (fgets(buffer.data(), 128, pipe) != NULL)
            {
                buffer[128] = 0;
                result += buffer.data();
            }
        }
    } 
    catch (...) 
    {
        pclose(pipe);
        throw;
    }
    
    pclose(pipe);
#endif
    return result;
}

}
