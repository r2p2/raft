#pragma once

#include <chrono>

class Timeout
{
public:
    class EventHandler
    {
	public:
        virtual ~EventHandler() = default;

        virtual void timeout() = 0;
    };

    virtual ~Timeout() = default;

    virtual void handler(EventHandler* handler) = 0;
    virtual void reset(std::chrono::milliseconds const& duration) = 0;
};
