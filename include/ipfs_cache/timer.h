#pragma once

#include <event2/event_struct.h>
#include <chrono>

namespace ipfs_cache {

class Timer {
public:
    using Clock = std::chrono::steady_clock;

public:
    Timer(struct event_base*);

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void start(Clock::duration, std::function<void()>);

    bool is_running() const;

    void stop();

    ~Timer();

private:
    static void call(evutil_socket_t, short, void *);

private:
    struct event_base* _evbase;
    event* _event;
    std::function<void()> _cb;
};

inline
Timer::Timer(struct event_base* evbase)
    : _evbase(evbase)
{ }

inline
bool Timer::is_running() const
{
    return _event != nullptr;
}

inline
void Timer::stop()
{
    if (!_event) return;
    evtimer_del(_event);
    event_free(_event);
    _event = nullptr;
    _cb = nullptr;
}

inline
void Timer::start(Clock::duration duration, std::function<void()> cb)
{
    if (is_running()) {
        stop();
    }

    using namespace std::chrono;

    auto timeout_ms = duration_cast<milliseconds>(duration).count();

    struct timeval timeout;

    timeout.tv_sec  = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    _cb = std::move(cb);

    _event = event_new(_evbase, -1, 0, Timer::call, this);

    evtimer_add(_event, &timeout);
}

inline
void Timer::call(evutil_socket_t, short, void * v)
{
    auto self = (Timer*) v;
    auto cb = std::move(self->_cb);
    self->stop();
    cb();
}

inline
Timer::~Timer()
{
    stop();
}

} // ipfs_cache namespace
