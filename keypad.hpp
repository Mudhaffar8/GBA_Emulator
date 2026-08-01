#pragma once 

#include "memory.hpp"
#include "memory_regions.hpp"
#include "io_register.hpp"
#include "interrupts.hpp"

#include <iostream>
#include <SDL3/SDL_keyboard.h>


namespace GBAInput
{
    constexpr int DPAD_UP = SDL_SCANCODE_W;
    constexpr int DPAD_DOWN = SDL_SCANCODE_S;
    constexpr int DPAD_LEFT = SDL_SCANCODE_A;
    constexpr int DPAD_RIGHT = SDL_SCANCODE_D;
    constexpr int BUTTON_B = SDL_SCANCODE_J;
    constexpr int BUTTON_A = SDL_SCANCODE_K;
    constexpr int BUTTON_L = SDL_SCANCODE_U;
    constexpr int BUTTON_R = SDL_SCANCODE_I;
    constexpr int BUTTON_SELECT = SDL_SCANCODE_SPACE;
    constexpr int BUTTON_START = SDL_SCANCODE_RETURN;
}

class Keypad
{
public:
    Keypad(Memory& _mmu);

    enum KeypadInput
    {
        ButtonA = (1 << 0),
        ButtonB = (1 << 1),
        Select = (1 << 2),
        Start = (1 << 3),
        Right = (1 << 4),
        Left = (1 << 5),
        Up = (1 << 6),
        Down = (1 << 7),
        ButtonR = (1 << 8),
        ButtonL = (1 << 9),
        ButtonIRQEnable = (1 << 14),
        ButtonIRQCondition = (1 << 15)
    };

    /* Input Handling */
    void handle_inputs(const bool* keyboard);
    void set_key(KeypadInput input_bit, bool cond);

private:
    Memory& memory;
    
    Io16<GBAIO::KEYINPUT> keypad_input;
    Io16<GBAIO::KEYCNT> keypad_control;
};