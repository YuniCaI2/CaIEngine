#pragma once

#include<thread>
#include<vector>
#include<functional>
#include<queue>
#include<atomic>
#include<mutex>
#include<future>
#include "../Logger.h"

class ThreadPool {
public:
    static ThreadPool& GetInstance() {
        static ThreadPool instance(8);
        return instance;
    }

    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>{ 
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if(stop.load()){
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([task](){ (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    uint32_t Size(){
        return workers.size();
    }

    uint32_t PendingSize(){
        std::lock_guard<std::mutex> lock(queueMutex);
        return tasks.size();
    }

    ~ThreadPool(){
        stop.store(true);
        condition.notify_all();
        for(std::thread &worker : workers){
            if(worker.joinable()){
                worker.join();
            }
        }
    }

    
private:
    ThreadPool(uint32_t numThreads){
        if(numThreads > std::thread::hardware_concurrency()){
            LOG_WARNING("The numThreads: {} exceed the hardware concurrency, set to hardware concurrency", numThreads);
            numThreads = std::thread::hardware_concurrency();
        }
        for(uint32_t i = 0; i < numThreads; i++){
            workers.emplace_back([this]{
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);

                        this->condition.wait(lock, [this]{ return this->stop.load() || !this->tasks.empty(); });

                        if(this->stop.load() && this->tasks.empty()){
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::atomic<bool> stop{false};
    std::mutex queueMutex; //保证队列的线程安全
    std::condition_variable condition;
};