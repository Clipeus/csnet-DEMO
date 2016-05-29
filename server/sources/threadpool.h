#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace csnet
{

// thread pool manager class
class thread_pool_t
{
public:
    // the constructor just launches some amount of workers
    explicit thread_pool_t(size_t threads);
    // the destructor joins all threads
    ~thread_pool_t();
    
    // wait with joining
    void wait();
    
    // close pool with or w/o joining
    void close(bool wait = true, bool clear_tasks = false);
    
    // add new work item to the pool
    template<class F, class... Args>
    decltype(auto) enqueue(F&& f, Args&&... args) //-> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()> >(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            
        // promise object
        std::future<return_type> res = task->get_future();
        {
            // lock the code
            std::unique_lock<std::mutex> lock(_queue_mutex);

            // don't allow enqueueing after stopping the pool
            if(_stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            // add task to the end of the queue
            _tasks.emplace([task](){ (*task)(); });
        }
        _condition.notify_one(); // notify only one thread about changings
        return res;
    }
    
private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> _workers;
    // the task queue
    std::queue<std::function<void()> > _tasks;
    // synchronization
    std::mutex _queue_mutex;
    std::condition_variable _condition;
    bool _stop;
};

}

