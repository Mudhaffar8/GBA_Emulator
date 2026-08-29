#pragma once

#include "display.hpp"
#include "utils.hpp"

#include <string_view>
#include <iostream>

class Debugger
{
public:
    Debugger(Display& display);

    std::string_view error_msg;

    inline void trigger_exception(std::string_view what)
    {
        error_msg = what;
        display.get_running_status() = false;

        std::cout << "Exception Raised!\n" << "Message: " << what << '\n';
    }

    inline void do_bounds_check(uint32_t val, size_t start, size_t end, std::string name)
    {
        if (val < start || val > end)
        {
            std::string s = "Out of Bounds! " + name + " | Value: " + Utils::int_to_hex(val) + '\n';
            s += "Range Start: " + Utils::int_to_hex(start);
            s += "\nRange End: " + Utils::int_to_hex(end);
            trigger_exception("Index Out of Bounds for" + name);
        }
    }

private:
    Display& display;
};

Debugger::Debugger(Display& display) : display(display) {}