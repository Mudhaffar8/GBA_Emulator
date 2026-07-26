#pragma once

#include <cstdint>
#include <vector>

class Scheduler
{
public:
    Scheduler();

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
    std::vector<Event> event_queue;
    uint64_t cycles_elapsed = 0;
};