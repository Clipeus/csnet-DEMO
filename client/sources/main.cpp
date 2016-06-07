#include <sys/types.h>
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
#include <functional>

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
std::string gettime()
{
    try
    {
        packet_socket_t socket = get_socket();
        
        // send request to server
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_DATA_TYPE, packet_code::P_TIME_ACTION)))
        {
            // receive response from server
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

// send text to echo server and get echo from server
std::string echo(const std::string& text)
{
    try
    {
        packet_socket_t socket = get_socket();
        
        // send request to server
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_ECHO_ACTION), text))
        {
            // receive response from server
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

// send command to server and get command's result
std::string execmd(const std::string& cmd)
{
    try
    {
        packet_socket_t socket = get_socket();
        
        // send request to server
        if (socket.send(packet_info_t(packet_kind::P_BASE_KIND, packet_type::P_TEXT_TYPE, packet_code::P_EXECMD_ACTION), cmd))
        {
            // receive response from server
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
template <class T, typename... Args>
void do_in_thread(int count, T func, Args&&... args)
{
    std::vector<std::thread> threads(count);

    for (int i = 0; i < count; i++)
    {
        // pass function and its param(s) to a thread function
        threads[i] = std::thread([=](Args&&... _args)
        {
            // exceute function
            std::string r = func(std::forward<Args>(_args)...);

            // print result
            std::lock_guard<std::mutex> lck(_mtx);
            std::cout << "recieved: " << r << std::endl;

        }, std::forward<Args>(args)...);
    }

    // wait thread(s)
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
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

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
            std::getline(std::cin, cmd);
            
            if (cmd == "q") // quit
            {
                std::cout << "myclient is finished" << std::endl << std::endl;
                break;
            }
            else if (cmd == "h") // help
            {
                help();
            }
            else if (cmd == "t") // set threads count
            {
                std::cout << "current request threads count: " << threads << std::endl;
                std::cout << std::endl << "threads: ";
                std::getline(std::cin, cmd);
                threads = std::atoi(cmd.c_str());
                if (threads <= 0)
                    threads = 1;
                
                std::cout << "current request threads count: " << threads << std::endl << std::endl;
            }
            else if (cmd == "1") // send text to echo server
            {
                std::cout << std::endl << "text: ";
                std::getline(std::cin, cmd);
                std::cout << "sending: " << cmd << std::endl;
                do_in_thread(threads, std::function<std::string(const std::string&)>(echo), cmd);
            }
            else if (cmd == "2") // get time from server
            {
                std::cout << "send time request" << std::endl;
                do_in_thread(threads, std::function<std::string()>(gettime));
            }
            else if (cmd == "3") // execute command on server
            {
                std::cout << std::endl << "exec: ";
                std::getline(std::cin, cmd);
                do_in_thread(threads, std::function<std::string(const std::string&)>(execmd), cmd);
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
    
#ifdef _WIN32
    WSACleanup();
#endif

    return -1;
}
