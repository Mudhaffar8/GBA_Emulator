#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>

class Scheduler
{
public:
    Scheduler();

    enum class EventType
    {
        VBlankEnter,
        VBlankExit,
        HBlank,
        Timer,
        DMA
    };

    struct Event 
    {
        EventType event_type;
        uint64_t cycles;
        
        Event(EventType et, uint64_t c) : event_type(et), cycles(c) {}
        
        bool operator>(const Event& e) const { return cycles > e.cycles; }
    };  

    void add_event(EventType event_type, uint64_t event_cycles);
    void pop_event();

    Event get_next_event();

    bool next_event_pending()
    {
        return global_cycles >= event_queue.top().cycles;
    }

    void advance(uint64_t cycles)
    {
        global_cycles += cycles;
    }

    uint64_t get_global_cycles() { return global_cycles; }

    /* Debugging */
    void print_all();

private:
    // Sticking to a priority queue for now
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> event_queue{}; 
    uint64_t global_cycles = 0;
};