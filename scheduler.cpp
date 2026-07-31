#include "scheduler.hpp"

#include "utils.hpp"
#include <algorithm>

Scheduler::Scheduler()
{}

void Scheduler::add_event(EventType event_type, uint64_t event_cycles)
{
    uint64_t cycles_until_event = global_cycles + event_cycles;
    event_queue.emplace(event_type, cycles_until_event);
}

void Scheduler::pop_event()
{
    if (event_queue.empty())
    {
        std::cout << "Empty Event Queue\n";
        throw std::runtime_error("Empty Scheduler");
    }
    event_queue.pop();
}

Scheduler::Event Scheduler::get_next_event()
{
    if (event_queue.empty())
    {
        std::cout << "Empty Event Queue\n";
        throw std::runtime_error("Empty Scheduler");
    }
    return event_queue.top();
}

void Scheduler::print_all()
{
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> copy;

    while (!event_queue.empty())
    {
        Event event = event_queue.top();
        std::cout << "[Event ID: " 
            << static_cast<int>(event.event_type)
            << ", Cycles: " << event.cycles << "]\n";
        copy.emplace(event);
        event_queue.pop();
    }
    event_queue = std::move(copy);
    std::cout << "----------------\n";
}