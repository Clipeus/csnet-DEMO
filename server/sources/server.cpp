#include <cstring>
#include <sstream>

#include "server.h"
#include "threadpool.h"
#include "logger.h"

namespace csnet
{

using namespace shared;

// socket server class
myserver_t::myserver_t(std::unique_ptr<service_i> handler) : _handler(std::move(handler)), _signal(this, &myserver_t::onsignal)
{
#ifdef _WIN32
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

myserver_t::~myserver_t()
{
    stop();

#ifdef _WIN32
    if (_cancel == INVALID_SOCKET)
        ::closesocket(_cancel);
    ::WSACleanup();
#endif
}

// init server
void myserver_t::init_socket(int port, int queue_count)
{
    if (!_socket.create())
        throw csnet_api_error(_socket.error_msg());

    _socket.set_unblocking(true);

    // init and bind socket
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (!_socket.bind((sockaddr *)&addr, sizeof(addr)))
        throw csnet_api_error(_socket.error_msg());

    _socket.listen(queue_count);
}

void myserver_t::init_signal()
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
void myserver_t::onsignal(const signal_t<myserver_t>* sender, int signal)
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

// start server
int myserver_t::start(int port, int pool_count, int queue_count)
{
    int status = 0;
    try
    {
        init_socket(port, queue_count);
        init_signal();

        // init thread pool by threads number
        thread_pool_t pool(pool_count);

        // main server loop
        while (!is_finished())
        {
            LOGLINE("Waitnig for connection.");

            // wait socket data to read
#ifdef _WIN32
            int ret = _socket.read_ready(-1, 0, _cancel);
#else
            int ret = _socket.read_ready();
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
                    buf << "Socket selecting failed, errno: " << _socket.error_msg();
                    throw std::runtime_error(buf.str());
                }
            }
            else if (ret)
            {
                LOGLINE("Accepting socket.");
                packet_socket_t accepted;
                if (!_socket.accept(accepted))
                {
                    std::stringstream buf;
                    buf << "Socket accepting failed: " << _socket.error_msg();
                    throw std::runtime_error(buf.str());
                }

                socket_t::SOCKET_HANDLE hsocket = accepted.detach();

                LOGLINE("Add job to pool.");
                pool.enqueue([this, hsocket] // handle net request
                {
                    // thread code
                    try
                    {
                        srvapi_t srvapi;
                        srvapi.onaccept(packet_socket_t(hsocket));

                        LOGLINE("Recieving socket data.");

                        if (!srvapi.receive())
                        {
                            LOGLINE("There is no any data recieved.");
                            //LOGLINE((srvapi.error_msg().empty() ? "There is no any data recieved." : srvapi.error_msg()));
                        }
                        else if (srvapi.is_packet_of(packet_type::P_DATA_TYPE, packet_code::P_CREDENTIALS_ACTION))
                        {
                            // it is check credentials action
                            packet_data_t* packet_data = srvapi.packet<packet_data_t>();

                            credentials_info_t* ci = reinterpret_cast<credentials_info_t*>(packet_data->data);

                            // extract params
                            std::string login(ci->data, ci->login_len);
                            std::string password(ci->data + ci->login_len, ci->password_len);

                            // check login and password
                            if (!_handler->check_credentials(login, password))
                                srvapi.send_reply(packet_data->action, (uint32_t)-1, "Invalid credentials");
                            else
                                srvapi.send_reply(packet_data->action); // replay OK to client
                        }
                        else if (srvapi.is_packet_of(packet_type::P_DATA_TYPE, packet_code::P_PING_ACTION))
                        {
                            // it is ping action
                            packet_data_t* packet_data = srvapi.packet<packet_data_t>();
                            uint64_t result = _handler->ping(reinterpret_cast<uint64_t>(packet_data->data));

                            // replay pind data to client
                            srvapi.send_reply(packet_data->action, reinterpret_cast<int8_t*>(result), sizeof(uint64_t));
                        }
                        else if (srvapi.is_packet_of(packet_type::P_TEXT_TYPE, packet_code::P_ECHO_ACTION))
                        {
                            // it is echo server action
                            packet_text_t* packet_text = srvapi.packet<packet_text_t>();
                            std::string result = _handler->sendmsg(packet_text->text);

                            // replay string to client
                            srvapi.send_reply(packet_text->action, result);
                        }
                        else if (srvapi.is_packet_of(packet_type::P_NULL_TYPE, packet_code::P_TIME_ACTION))
                        {
                            // it is gettime action
                            std::time_t now = _handler->gettime();

                            // replay time to client
                            srvapi.send_reply(packet_code::P_TIME_ACTION, reinterpret_cast<int8_t*>(&now), sizeof(now));
                        }
                        else if (srvapi.is_packet_of(packet_type::P_TEXT_TYPE, packet_code::P_EXECMD_ACTION))
                        {
                            // it is execute cmd action
                            packet_text_t* packet_text = srvapi.packet<packet_text_t>();
                            std::string result = _handler->execmd(packet_text->text);

                            // replay command's result to client
                            srvapi.send_reply(packet_text->action, result);
                        }
                        else
                        {
                            // it is unknown action
                            packet_info_t* packet = srvapi.packet<packet_info_t>();
                            srvapi.send_reply(packet->action, (uint32_t)-2, "Unknown packet");
                        }
                    }
                    catch (std::exception& e)
                    {
                        LOGLINE("Error occurred: " << e.what());
                    }
                    catch (...)
                    {
                        LOGLINE("Error occurred: " << "unexception error.");
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

// stopt server
void myserver_t::stop()
{
}

// true if need to exit
bool myserver_t::is_finished()
{
    LOGLINE("is_finished: " << _finished << ".");
    return _finished;
}

}
