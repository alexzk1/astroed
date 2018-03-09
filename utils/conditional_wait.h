#ifndef CONDITIONAL_WAIT_H
#define CONDITIONAL_WAIT_H
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <memory>

//this one ensures that "confirm" will not be missed IF confirm() happened before waitConfirm()
//(that is nothing about password, pass = "passing")
class confirmed_pass
{
private:
    std::unique_ptr<std::mutex>              waitMtx;
    std::unique_ptr<std::condition_variable> cv;
    bool                                     conf; //"Effective C++ 14" - no need in atomical when is guarded by mutex

public:
    using wait_lambda = std::function<bool()>;
    confirmed_pass():
        waitMtx(new std::mutex()),
        cv(new std::condition_variable()),
        conf(false)
    {
    }

    confirmed_pass(const confirmed_pass& c) = delete;
    confirmed_pass(confirmed_pass&& c) = delete;
    confirmed_pass& operator = (const confirmed_pass& c) = delete;
    confirmed_pass& operator = (confirmed_pass&& c) = delete;



    void waitConfirm()
    {
        std::unique_lock<std::mutex> lck(*waitMtx);
        if (!conf)
            cv->wait(lck, [this]()->bool{return conf;});
        conf = false;
    }

    //returns false if timeouted (and unblocks thread!), should be used in while() GUI to process events
    bool tryWaitConfirm(int millis)
    {
        std::unique_lock<std::mutex> lck(*waitMtx);
        if(!conf)
            cv->wait_for(lck, std::chrono::milliseconds(millis), [this]()->bool{return conf;});
        if (conf)
        {
            conf = false;
            return true;
        }
        return false;
    }

    //allows to do additional check of extern stop pereodicaly
    void waitConfirm(const std::atomic<bool>& isStopped)
    {
        std::unique_lock<std::mutex> lck(*waitMtx);
        if (!conf)
            cv->wait(lck, [this, &isStopped]()->bool{return conf || isStopped;});
        conf = false;
    }

    void waitConfirm(const wait_lambda& isStopped)
    {
        std::unique_lock<std::mutex> lck(*waitMtx);
        if (!conf)
            cv->wait(lck, [this, &isStopped]()->bool{return conf || isStopped();});
        conf = false;
    }

    void confirm()
    {
        std::unique_lock<std::mutex> lck(*waitMtx) ;
        conf = true;
        cv->notify_all();
    }
};

using confirmed_pass_ptr = std::shared_ptr<confirmed_pass>;

#endif // CONDITIONAL_WAIT_H
