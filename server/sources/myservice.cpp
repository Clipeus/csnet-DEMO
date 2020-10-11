#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#else
#include <unistd.h>
#endif

#include <chrono>
#include <ctime>
#include <cctype>

#include "myservice.h"
#include "logger.h"
#include "mysettings.h"
#include "expression.h"

namespace csnet
{

  myservice_t::myservice_t()
  {
  }


  myservice_t::~myservice_t()
  {
  }

  // ping command
  uint64_t myservice_t::ping(uint64_t data) const
  {
    LOGLINE("Ping action: " << data << ".");
    return data;
  }

  // check client credentials
  bool myservice_t::check_credentials(const std::string& login, const std::string& password) const
  {
    LOGLINE("Check client credentials, login: " << login << ", password: " << password << ".");

    return (login == mysettings_t::instance()->login() && password == mysettings_t::instance()->password());
  }

  // echo server command
  std::string myservice_t::sendmsg(const std::string& msg) const
  {
    LOGLINE("Echo server action.");

    std::string result(msg.size(), 0);
    // set string to upper
    std::transform(msg.cbegin(), msg.cend(), result.begin(), toupper);

    LOGLINE("Echo: " << result << ".");
    return result;
  }

  // get current time command
  std::time_t myservice_t::gettime() const
  {
    LOGLINE("Get time action.");

    // get current server time
    std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(today);

    LOGLINE("Time: " << now << ".");
    return now;
  }

  // execute command
  std::string myservice_t::execmd(const std::string& cmd) const
  {
    LOGLINE("Execute command: " << cmd << ".");

    // execute command and get command's result
    std::string result = exec(cmd);

    LOGLINE("Command resul: " << result << ".");
    return result;
  }

  // execute command and get its result
  std::string myservice_t::exec(const std::string& cmd) const
  {
    std::string result = "";
    FILE* pipe = nullptr;

    try
    {
      std::array<char, 128 + 1> buffer;
      FILE* pipe = popen(cmd.c_str(), "r");
      if (!pipe)
        throw std::runtime_error("popen() failed!");

      while (!feof(pipe))
      {
        if (fgets(buffer.data(), 128, pipe) != NULL)
        {
          buffer[128] = 0;
          result += buffer.data();
        }
      }

      pclose(pipe);
    }
    catch (std::exception& e)
    {
      if (pipe)
        pclose(pipe);

      result = "Execute command failed: ";
      result += e.what();
    }
    catch (...)
    {
      if (pipe)
        pclose(pipe);

      result = "Execute command failed: unexception error.";
    }

    return result;
  }

  // calculate command
  std::string myservice_t::calculate(const std::string& input) const
  {
    LOGLINE("Calculate server action.");

    std::stringstream buf;

    try 
    {
      parser_t p(input);
      double result =  expression_t::eval(p.parse());
      LOGLINE("Result: " << input << " = " << result << ".");
      buf << result;
    }
    catch (std::exception& e) 
    {
      buf << input << " : exception: " << e.what() << '\n';
    }

    return buf.str();
  }
}
