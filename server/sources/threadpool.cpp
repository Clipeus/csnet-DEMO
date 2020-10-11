#include <sstream>
#include "threadpool.h"
#include "logger.h"

namespace csnet
{

  using namespace shared;

  // the constructor just launches some amount of workers
  thread_pool_t::thread_pool_t(size_t threads) : _stop(false)
  {
    for (size_t i = 0; i < threads; ++i)
    {
      // declare thread function
      _workers.emplace_back
      (
        [this]
      {
        LOGLINE("Thread " << std::this_thread::get_id() << " is started.");

        for (;;)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->_queue_mutex);
            this->_condition.wait(lock, [this]
            {
              return this->_stop || !this->_tasks.empty();
            });

            // continue work?
            if (this->_stop && this->_tasks.empty())
              break;

            // get next task
            task = std::move(this->_tasks.front());
            this->_tasks.pop();
          }

          LOGLINE("Thread " << std::this_thread::get_id() << " executes a task.");

          task(); // execute a task
        }

        LOGLINE("Thread " << std::this_thread::get_id() << " is finished.");
      }
      );
    }
  }

  // the destructor joins all threads
  thread_pool_t::~thread_pool_t()
  {
    wait();
  }

  // wait with joining
  void thread_pool_t::wait()
  {
    close(true, false);
  }

  // close pool with or w/o joining
  void thread_pool_t::close(bool wait, bool clear_tasks)
  {
    {
      // lock the code
      std::unique_lock<std::mutex> lock(_queue_mutex);
      if (_workers.size() == 0)
        return;

      LOGLINE("closing pool.");
      LOGLINE("tasks count " << _tasks.size() << ".");

      _stop = true; // tell to all threads to exit

      if (clear_tasks)
      {
        LOGLINE("empty tasks.");

        // empty task queue, does need to continue task process
        while (!_tasks.empty())
          _tasks.pop();
      }
    }

    _condition.notify_all(); // notify all threads about changings

    if (wait)
    {
      LOGLINE("waiting for closing.");

      // waiting for a closing all threads
      for (std::thread& worker : _workers)
        worker.join();

      LOGLINE("pool is closed.");
    }
    else
    {
      // not waiting for a closing all threads
      for (std::thread& worker : _workers)
        worker.detach();
    }

    // clear threads pool
    _workers.clear();
  }

}
