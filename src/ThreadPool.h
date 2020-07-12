#ifndef __CW_THREAD_POOL_H_
#define __CW_THREAD_POOL_H_

#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <iostream>
#include <list>
#include <functional>

namespace cW {
typedef std::function<void(void)> Predicate;
struct Thread {
  private:
    std::thread                           thread;
    std::atomic<bool>                     idle         = false;
    std::atomic<bool>                     shouldCancel = false;
    bool                                  running      = false;
    std::condition_variable               cv;
    std::mutex                            cv_mtx;
    std::queue<Predicate>                 defers;
    std::mutex                            defer_mtx;
    std::chrono::steady_clock::time_point last_active;
    std::chrono::seconds                  idleTimeout = std::chrono::seconds(20);
    std::atomic<unsigned int>             defer_count;

  public:
    unsigned int deferCount() { return defer_count; }
    bool         isIdle() { return idle; }
    bool         joinIfIdle()
    {
        if (idle && last_active + idleTimeout <= std::chrono::steady_clock::now()) {
            cancelAndJoin();
            return true;
        }
        return false;
    }
    void cancelAndJoin()
    {
        if (running) {
            shouldCancel = true;
            cv.notify_one();
            if (thread.joinable()) thread.join();
            running = false;
        }
    }
    void defer(Predicate&& predicate)
    {
        defer_count++;
        {
            std::scoped_lock lock(defer_mtx);
            defers.push(std::move(predicate));
        }
        if (!running) run();
        if (idle) {
            shouldCancel = false;
            idle         = false;
            cv.notify_one();
        }
    }

    ~Thread() { cancelAndJoin(); }

  private:
    void run()
    {
        running = true;
        std::cout << "Started executing loop." << std::endl;
        thread = std::thread(
            [](std::atomic<bool>&                     shouldCancel,
               std::atomic<bool>&                     idle,
               std::queue<Predicate>&                 defers,
               std::mutex&                            defer_mtx,
               std::atomic<unsigned int>&             defer_count,
               std::condition_variable&               cv,
               std::mutex&                            cv_mtx,
               std::chrono::steady_clock::time_point& last_active) {
                std::cout << "Inside loop." << std::endl;
                while (true) {
                    bool empty;
                    {
                        std::cout << "Checking if empty." << std::endl;
                        std::scoped_lock lock(defer_mtx);
                        empty = defers.empty();
                        std::cout << "Empty " << empty << std::endl;
                    }
                    if (empty) {
                        std::unique_lock cv_lck(cv_mtx);
                        idle = true;
                        cv.wait(cv_lck);
                        if (shouldCancel) break;
                    }
                    else
                        idle = false;
                    {
                        std::unique_lock lock(defer_mtx, std::defer_lock);
                        lock.lock();
                        const auto& defer = defers.front();
                        lock.unlock();
                        defer();
                        lock.lock();
                        defers.pop();
                        defer_count--;
                        last_active = std::chrono::steady_clock::now();
                        lock.unlock();
                    }
                }
            },
            std::ref(shouldCancel),
            std::ref(idle),
            std::ref(defers),
            std::ref(defer_mtx),
            std::ref(defer_count),
            std::ref(cv),
            std::ref(cv_mtx),
            std::ref(last_active));
    }
};

struct ThreadPool {
    std::list<std::unique_ptr<Thread>> threads;

    void clearIdleThreads()
    {
        for (auto itr = threads.begin(); itr != threads.end(); ++itr)
            if ((*itr)->joinIfIdle()) threads.erase(itr);
    };

    void launch(Predicate&& predicate)
    {
        // for (const auto& thread : threads) {
        //     if (thread->isIdle()) {
        //         thread->defer(std::move(predicate));
        //         return;
        //     }
        // }
        auto thread = std::make_unique<Thread>();
        thread->defer(std::move(predicate));
        // threads.push_back(std::move(thread));
    }
    void defer(Predicate&& predicate)
    {
        if (threads.empty())
            launch(std::move(predicate));
        else {
            auto min_itr         = threads.begin();
            auto itr             = ++min_itr;
            auto min_defer_count = (*min_itr)->deferCount();
            for (; itr != threads.end(); ++itr) {
                unsigned int defer_count;
                if ((defer_count = (*itr)->deferCount()) < min_defer_count) {
                    min_itr         = itr;
                    min_defer_count = defer_count;
                }
            }
            (*min_itr)->defer(std::move(predicate));
        }
    }
};
}; // namespace cW

#endif