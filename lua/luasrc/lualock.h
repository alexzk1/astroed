#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <mutex>
#include <exception>

class LuaLocker
{
private:
    const std::thread::id lock_is_free;
    std::atomic<std::thread::id> lock_owner;
    std::atomic<int> lock_count;

    std::recursive_mutex mutex; //hmm, it seem everthing is proper so far, will use other fields to control validness
public:
    LuaLocker():
        lock_is_free(),
        lock_owner(lock_is_free),
        lock_count(0),
        mutex()
    {
    }

    void lock()
    {
        mutex.lock();
        lock_owner = std::this_thread::get_id();
        ++lock_count;
        return;
    }

    void unlock()
    {
        if (lock_owner != std::this_thread::get_id())
            throw std::runtime_error("Unlocking VM by wrong thread (not an owner).");

        auto v = lock_count.fetch_sub(1);
        if (v < 1)
            throw std::runtime_error("Unlocking VM which is already unlocked.");

        if(v == 1)
        {
            lock_owner.store(lock_is_free, std::memory_order_release);
        }
        mutex.unlock();
    }

    bool try_lock()
    {
        if (mutex.try_lock())
        {
            lock_owner = std::this_thread::get_id();
            ++lock_count;
            return true;
        }
        return false;
    }

    int count() const
    {
        return lock_count;
    }
};

typedef std::lock_guard<LuaLocker> a_lua_lock_t;

