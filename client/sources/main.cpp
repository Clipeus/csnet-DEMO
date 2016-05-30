#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include <iomanip>

#include "mysettings.h"
#include "socket.h"
#include "packsock.h"

using namespace csnet;
using namespace csnet::shared;

static std::mutex _mtx; // locker for std::cout

// init socket object
packet_socket_t get_socket()
{
    std::stringstream buf;
    packet_socket_t socket;

    if (!socket.create())
    {
        buf << "Error occurred: " << socket.error_msg();
        throw std::runtime_error(buf.str());
    }
    if (!socket.connect(mysettings_t::instance()->host().c_str(), mysettings_t::instance()->port()))
    {
        buf << "Error occurred: " <<  socket.error_msg();
        throw std::runtime_error(buf.str());
    }
    
    return socket;
}

// send request to server and get current time from server
std::string time()
{
    try
    {
        packet_socket_t socket = get_socket();
        
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_DATA_TYPE, packet_code::P_TIME_ACTION)))
        {
            std::unique_ptr<packet_data_t> packet(socket.receive_data());
            if (packet && packet->kind == packet_kind::P_BASE_KIND && packet->type == packet_type::P_DATA_TYPE && packet->action == (packet_code::P_TIME_ACTION | packet_code::P_RETURN_ACTION))
            {
                tm* timeinfo;
                timeinfo = localtime((std::time_t*)&packet->data);
                
                std::string time;
                time.resize(80);
                size_t size = strftime(&time.front(), time.size(), "%Y-%m-%d %X", timeinfo); 
                time.resize(size);
                return time;
            }
            else if (packet)
            {
                return "Unknown packet";
            }
            else
            {
                return socket.error_msg().size() ? socket.error_msg() : "error receiving packet";
            }
        }
        else
        {
            return socket.error_msg();
        }
    }
    catch (std::exception& e)
    {
        std::stringstream ret;
        ret << "Error occurred: " << e.what() << std::endl;
        return ret.str();
    }
}

// send request to server and get current time from server in a thread
void time_thread(int count)
{
    std::vector<std::thread> threads(count);

    for (int i = 0; i < count; i++)
    {
        threads[i] = std::thread([]
        {
            std::string r = time();
            std::lock_guard<std::mutex> lck (_mtx);
            std::cout << "recieved: " << r << std::endl;
        });
    }

    for (int i = 0; i < count; i++)
        threads[i].join();
}


// send text to echo server and get echo from server
std::string send(const std::string& text)
{
    try
    {
        packet_socket_t socket = get_socket();
        
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_ECHO_ACTION), text))
        {
            std::unique_ptr<packet_text_t> packet(socket.receive_text());
            if (packet)
                return packet->text;
            else
                return socket.error_msg().size() ? socket.error_msg() : "error receiving packet";
        }
        else
        {
            return socket.error_msg();
        }
    }
    catch (std::exception& e)
    {
        std::stringstream ret;
        ret << "Error occurred: " << e.what() << std::endl;
        return ret.str();
    }
}

// send text to echo server and get echo from server in a thread
void send_thread(int count, const std::string& text)
{
    std::vector<std::thread> threads(count);

    for (int i = 0; i < count; i++)
    {
        threads[i] = std::thread([text]
        {
            std::string r = send(text);
            std::lock_guard<std::mutex> lck (_mtx);
            std::cout << "recieved: " << r << std::endl;
        });
    }

    for (int i = 0; i < count; i++)
        threads[i].join();
    
}

// send command to server and get command's result
std::string execmd(const std::string& cmd)
{
    try
    {
        packet_socket_t socket = get_socket();
        
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_EXECMD_ACTION), cmd))
        {
            std::unique_ptr<packet_text_t> packet(socket.receive_text());
            if (packet)
                return packet->text;
            else
                return socket.error_msg().size() ? socket.error_msg() : "error receiving packet";
        }
        else
        {
            return socket.error_msg();
        }
    }
    catch (std::exception& e)
    {
        std::stringstream ret;
        ret << "Error occurred: " << e.what() << std::endl;
        return ret.str();
    }
}

// send command to server and get command's result in a thread
void execmd_thread(int count, const std::string& cmd)
{
    std::vector<std::thread> threads(count);

    for (int i = 0; i < count; i++)
    {
        threads[i] = std::thread([cmd]
        {
            std::string r = execmd(cmd);
            std::lock_guard<std::mutex> lck (_mtx);
            std::cout << "recieved: " << r << std::endl;
        });
    }

    for (int i = 0; i < count; i++)
        threads[i].join();
    
}

// print help screen
void help()
{
    std::cout << std::endl << "type a following command:" << std::endl;
    std::cout << "1 - send text to the server" << std::endl;
    std::cout << "2 - get time from the server" << std::endl;
    std::cout << "3 - execute command" << std::endl;
    std::cout << "t - set request threads count (default 1)" << std::endl;
    std::cout << "h - help screen" << std::endl;
    std::cout << "q - quit" << std::endl;
}

int main(int argc, char** args)
{
    try
    {
        mysettings_t::instance()->load();

        std::cout << std::endl << "Demo Client Application, version 1.0" << std::endl;
        std::cout << "host: " << mysettings_t::instance()->host() << std::endl;
        std::cout << "port: " << mysettings_t::instance()->port() << std::endl;
        
        help();
        
        int threads = 1;
            
        while (true)
        {
            std::cout << std::endl << "cmd: ";
            std::string cmd;
            getline(std::cin, cmd);
            
            if (cmd == "q")
            {
                std::cout << "myclient is finished" << std::endl << std::endl;
                break;
            }
            else if (cmd == "h")
            {
                help();
            }
            else if (cmd == "t")
            {
                std::cout << "current request threads count: " << threads << std::endl;
                std::cout << std::endl << "threads: ";
                getline(std::cin, cmd);
                threads = std::atoi(cmd.c_str());
                if (threads <= 0)
                    threads = 1;
                
                std::cout << "current request threads count: " << threads << std::endl << std::endl;
            }
            else if (cmd == "1")
            {
                std::cout << std::endl << "text: ";
                getline(std::cin, cmd);
                std::cout << "sending: " << cmd << std::endl;
                if (threads == 1)
                    std::cout << "recieved: " << send(cmd) << std::endl;
                else
                    send_thread(threads, cmd);
            }
            else if (cmd == "2")
            {
                std::cout << "send time request" << std::endl;
                if (threads == 1)
                    std::cout << "recieved: " << time() << std::endl;
                else
                    time_thread(threads);
            }
            else if (cmd == "3")
            {
                std::cout << std::endl << "exec: ";
                getline(std::cin, cmd);
                if (threads == 1)
                    std::cout << "recieved: " << execmd(cmd) << std::endl;
                else
                    execmd_thread(threads, cmd);
            }
            else
            {
                std::cout << "invalid command" << std::endl;
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Error occurred: " << "unexception error." << std::endl;
    }
    
    return -1;
}
