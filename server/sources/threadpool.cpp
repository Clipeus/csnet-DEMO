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
                std::stringstream id;
                id << std::this_thread::get_id();
                logger_t::instance()->logout("Thread #%s is started.\n", id.str().c_str());
                
                for(;;)
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

                    logger_t::instance()->logout("Thread #%s executes a task.\n", id.str().c_str());

                    task(); // execute a task
                }
                
                logger_t::instance()->logout("Thread #%s is finished.\n", id.str().c_str());
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
        
        logger_t::instance()->logout("closing pool.\n");
        logger_t::instance()->logout("tasks count %d.\n", _tasks.size());

        _stop = true; // tell to all threads to exit
        
        if (clear_tasks)
        {
            logger_t::instance()->logout("empty tasks.\n");
            
            // empty task queue, does need to continue task process
            while (!_tasks.empty())
                _tasks.pop();
        }
    }
    
    _condition.notify_all(); // notify all threads about changings
    
    if (wait)
    {
        logger_t::instance()->logout("waiting for closing.\n");
        
        // waiting for a closing all threads
        for(std::thread &worker: _workers)
            worker.join();
        
        logger_t::instance()->logout("pool is closed.\n");
    }
    else
    {
        // not waiting for a closing all threads
        for(std::thread &worker: _workers)
            worker.detach();
    }
    
    // clear threads pool
    _workers.clear();
}

}
