#include <unistd.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>
#include <chrono>

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
}

server_t::~server_t()
{
    close();
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
    
    _listener.set_blocking(true);
    
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
    _signal.connect(SIGQUIT);
    _signal.connect(SIGINT);
    _signal.connect(SIGTERM);
    
    //_signal.connect(SIGHUP);
}

// signals handler
void server_t::onsignal(const signal_t<server_t>* sender, int signal)
{
    logger_t::instance()->logout("Signal occurred: %s (%u).\n", sender->signame(signal).c_str(), signal);
    
    if (signal == SIGQUIT || signal == SIGINT || signal == SIGTERM)
    {
        // need to exit
        _finished = true;
    }
    /*else if (signal == SIGHUP)
    {
    }*/
}

// server main routine
int server_t::run()
{
    try
    {
        init_socket();
        init_signal();
        
        // init thread pool by threads number
        thread_pool_t pool(mysettings_t::instance()->pool_count());
        
        int status = 0;
        
        // main server loop
        while (!is_finished())
        {
            logger_t::instance()->logout("Waitnig for connection.\n");
            
            // wait socket data to read
            int ret = _listener.read_ready();
            if ( ret < 0)
            {
                if (is_finished())
                {
                    logger_t::instance()->logout("Leave the server loop.\n");
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
                logger_t::instance()->logout("Accepting socket.\n");
                packet_socket_t accepted;
                if (!_listener.accept(accepted))
                {
                    std::stringstream buf;
                    buf << "Socket accepting failed: " << _listener.error_msg();
                    throw std::runtime_error(buf.str());
                }
                
                logger_t::instance()->logout("Add job to pool.\n");
                pool.enqueue([this, socket = std::move(accepted)] // handle net request
                {
                    // thread code
                    logger_t::instance()->logout("Recieving socket data.\n");
                    
                    // read packet
                    std::unique_ptr<packet_info_t> packet(socket.receive());
                    if (packet == nullptr)
                    {
                        logger_t::instance()->logout(socket.error_msg().empty() ? "There is no any data recieved.\n" : "%s\n", socket.error_msg().c_str());
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_TEXT_TYPE && packet->action == packet_code::P_ECHO_ACTION)
                    {
                        // it is echo server action
                        logger_t::instance()->logout("Demo echo server action\n");
                        packet_text_t* packet_text = static_cast<packet_text_t*>(packet.get());
                        logger_t::instance()->logout("Recieved text: %s.\n", packet_text->text);

                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // set string to upper
                        std::transform(packet_text->text, packet_text->text + std::strlen(packet_text->text), packet_text->text, ::toupper);                            
                        packet_text->action |= packet_code::P_RETURN_ACTION;
                        
                        logger_t::instance()->logout("Send text: %s.\n", packet_text->text);
                        
                        // replay string to client
                        if (!socket.send(packet_text))
                            logger_t::instance()->logout("Sending fialed: %s.\n", socket.error_msg().c_str());
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_DATA_TYPE && packet->action == packet_code::P_TIME_ACTION)
                    {
                        // it is gettime action
                        logger_t::instance()->logout("Demo get time action\n");
                        
                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // get current server time
                        std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
                        std::time_t now = std::chrono::system_clock::to_time_t(today);
                        
                        logger_t::instance()->logout("Send time.\n");
                        
                        // replay time to client
                        if (!socket.send(packet_info_t(packet->kind, packet->type, packet->action | packet_code::P_RETURN_ACTION), (int8_t*)&now, sizeof(now)))
                            logger_t::instance()->logout("Sending fialed: %s.\n", socket.error_msg().c_str());
                    }
                    else if (packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_TEXT_TYPE && packet->action == packet_code::P_EXECMD_ACTION)
                    {
                        // it is execute cmd action
                        logger_t::instance()->logout("Demo execute command\n");
                        packet_text_t* packet_text = static_cast<packet_text_t*>(packet.get());
                        logger_t::instance()->logout("Recieved command data: %s.\n", packet_text->text);

                        //std::this_thread::sleep_for(std::chrono::seconds(3));
                        
                        // execute command and get command's result
                        std::string result = exec(packet_text->text);
                        
                        logger_t::instance()->logout("Send command result %s.\n", result.c_str());
                        
                        // replay command's result to client
                        if (!socket.send(packet_info_t(packet->kind, packet->type, packet->action | packet_code::P_RETURN_ACTION), result))
                            logger_t::instance()->logout("Sending fialed: %s.\n", socket.error_msg().c_str());
                    }
                    else
                    {
                        // it is unknown action
                        logger_t::instance()->logout("Recieved unknown packet.\n");
                        
                        if (socket.send(packet_info_t(packet->kind, packet_type::P_ERROR_TYPE, packet->action | packet_code::P_RETURN_ACTION), "Unknown packet"))
                            logger_t::instance()->logout("Sending fialed: %s.\n", socket.error_msg().c_str());
                    }
                });
            }
            
            if (is_finished())
                break;
        }

        // wait and close all thread tasks
        pool.close(true, true);
        return status;
    }
    catch (std::exception& e)
    {
        std::stringstream buf;
        buf << "Error occurred: " << e.what() << std::endl;
        logger_t::instance()->logout(buf.str().c_str());
    }
    catch (...)
    {
        std::stringstream buf;
        buf << "Error occurred: " << "unexception error." << std::endl;
        logger_t::instance()->logout(buf.str().c_str());
    }
}

// true if need to exit
bool server_t::is_finished()
{
    logger_t::instance()->logout("is_finished: %d.\n", _finished);
    return _finished;
}

// execute command and get its result
std::string server_t::exec(const std::string& cmd) 
{
    std::vector<char> buffer(128 + 1);
    std::string result = "";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) 
        throw std::runtime_error("popen() failed!");
    
    try 
    {
        while (!feof(pipe)) 
        {
            if (fgets(&buffer.front(), 128, pipe) != NULL)
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
    return result;
}

}