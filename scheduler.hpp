#pragma once

#include <array>

class Scheduler
{
public:
    enum class EventType
    {
        VBlank,
        HBlank,
    };

    struct Event 
    {
        EventType event_type;
        int cycles;
    };  

private:
    std::array<Event, 12> event_queue{};
};