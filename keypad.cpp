#include "keypad.hpp"

#include "utils.hpp"

Keypad::Keypad(Memory& _memory) : 
    memory(_memory),
    keypad_input(_memory),
    keypad_control(_memory)
{}

void Keypad::handle_inputs(const bool* state)
{
    set_key(KeypadInput::ButtonA, state[GBAInput::BUTTON_A]);
    set_key(KeypadInput::ButtonB, state[GBAInput::BUTTON_B]);
    set_key(KeypadInput::Select, state[GBAInput::BUTTON_SELECT]);
    set_key(KeypadInput::Start, state[GBAInput::BUTTON_START]);
    set_key(KeypadInput::Right, state[GBAInput::DPAD_RIGHT]);
    set_key(KeypadInput::Left, state[GBAInput::DPAD_LEFT]);
    set_key(KeypadInput::Up, state[GBAInput::DPAD_UP]);
    set_key(KeypadInput::Down, state[GBAInput::DPAD_DOWN]);
    set_key(KeypadInput::ButtonR, state[GBAInput::BUTTON_R]);
    set_key(KeypadInput::ButtonL, state[GBAInput::BUTTON_L]);

    bool keypad_irq_enable = Utils::is_bit_set(keypad_control, KeypadInput::ButtonIRQEnable);
    if (keypad_irq_enable)
    {
        bool keypad_logical_and = Utils::is_bit_set(keypad_control, KeypadInput::ButtonIRQCondition);
        
        if (keypad_logical_and && (keypad_input & keypad_control) == keypad_control)
            GBAInterrupts::enable_interrupt(memory, Interrupts::Keypad);
        else if ((keypad_input & keypad_control & 0x3FFF) != 0)
            GBAInterrupts::enable_interrupt(memory,  Interrupts::Keypad);
    }
}


// Note that, rather unconventionally for the Game Boy (Advance),
// a button being pressed is seen as the corresponding bit 
// being 0, not 1.
void Keypad::set_key(KeypadInput input_bit, bool cond)
{
    keypad_input = (cond) ? 
        keypad_input & ~input_bit : 
        keypad_input | input_bit;
}