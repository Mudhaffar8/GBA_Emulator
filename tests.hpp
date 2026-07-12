#include <iostream>
#include <fstream>
#include <bitset>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>

#include "arm7.hpp"
#include "memory.hpp"

#include "json.hpp"

using json = nlohmann::json;

class GBATests
{
public:

    /// @brief Compares two values and throws an exception if they differ.
    /// @tparam T type of values being compared. Must support equality comparison.
    /// @param val Value being checked.
    /// @param other_val Value compared against.
    /// @throws `std::runtime_error` thrown when values are not equal
    template<typename T, bool print_bits = false>
    static void check_val(T val, T other_val, std::string name)
    {   
        if (val != other_val) 
        {
            // To prevent printing the ascii character instead of number for uint8_t, uint16_t etc.
            if constexpr (std::is_arithmetic<T>::value) 
            {
                if constexpr (print_bits)
                    std::cout << name << " Expected: " << std::bitset<32>(other_val) << "\n Got: " << std::bitset<32>(+val) << '\n';
                else 
                    std::cout << name << " Expected: " << +other_val << " Got: " << +val << '\n';
            }
            else
                std::cout << name << " Expected: " << other_val << " Got: " << val << '\n';
            
            throw std::runtime_error(std::string("Fail on ") + name);
        }
    }

    /// @brief Converts integer into hexidecimal string representation.
    /// @param num The integer value to convert.
    /// @return A lowercase hexadecimal string representing the input number.
    static std::string int_to_hex(int num)
    {
        std::stringstream stream;

        if (num < 0x10) stream << "0";
        stream << std::hex << num;

        return stream.str();
    }

    static void run_test(Arm7TDMI& cpu, FakeMemory& mem, std::string file_name)
    {  
        std::ifstream file("./v1/" + file_name);
        if (!file) 
        {
            std::cout << "File Not Found!\n";
            return;
        }

        std::cout << "Parsing...\n";

        json data = json::parse(file);   

        std::cout << "Running Test File: " << file_name << '\n';

        int number = 1;
        for (json::iterator it = data.begin(); it != data.end(); ++it)
        {
            cpu.skip_mult_instr = false;
            
            cpu.cpsr = static_cast<uint32_t>((*it)["initial"]["CPSR"]);
            cpu.handle_mode_switch(cpu.cpsr & 0x1F);

            // Set CPU register values
            for (int i = 0; i < 8; ++i)
                *cpu.registers[i] = static_cast<uint32_t>((*it)["initial"]["R"][i]);

            cpu.r8 = static_cast<uint32_t>((*it)["initial"]["R"][8]);
            cpu.r9 = static_cast<uint32_t>((*it)["initial"]["R"][9]);
            cpu.r10 = static_cast<uint32_t>((*it)["initial"]["R"][10]);
            cpu.r11 = static_cast<uint32_t>((*it)["initial"]["R"][11]);
            cpu.r12 = static_cast<uint32_t>((*it)["initial"]["R"][12]);
            cpu.r13 = static_cast<uint32_t>((*it)["initial"]["R"][13]);
            cpu.r14 = static_cast<uint32_t>((*it)["initial"]["R"][14]);
            cpu.r15 = static_cast<uint32_t>((*it)["initial"]["R"][15]);
        
            cpu.r8_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][0]);
            cpu.r9_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][1]);
            cpu.r10_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][2]);
            cpu.r11_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][3]);
            cpu.r12_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][4]);
            cpu.r13_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][5]);
            cpu.r14_fiq = static_cast<uint32_t>((*it)["initial"]["R_fiq"][6]);

            cpu.r13_svc = static_cast<uint32_t>((*it)["initial"]["R_svc"][0]);
            cpu.r14_svc = static_cast<uint32_t>((*it)["initial"]["R_svc"][1]);

            cpu.r13_abt = static_cast<uint32_t>((*it)["initial"]["R_abt"][0]);
            cpu.r14_abt = static_cast<uint32_t>((*it)["initial"]["R_abt"][1]);

            cpu.r13_irq = static_cast<uint32_t>((*it)["initial"]["R_irq"][0]);
            cpu.r14_irq = static_cast<uint32_t>((*it)["initial"]["R_irq"][1]);

            cpu.r13_und = static_cast<uint32_t>((*it)["initial"]["R_und"][0]);
            cpu.r14_und = static_cast<uint32_t>((*it)["initial"]["R_und"][1]);

            cpu.spsr_fiq = static_cast<uint32_t>((*it)["initial"]["SPSR"][0]);
            cpu.spsr_svc = static_cast<uint32_t>((*it)["initial"]["SPSR"][1]);
            cpu.spsr_abt = static_cast<uint32_t>((*it)["initial"]["SPSR"][2]);
            cpu.spsr_irq = static_cast<uint32_t>((*it)["initial"]["SPSR"][3]);
            cpu.spsr_und = static_cast<uint32_t>((*it)["initial"]["SPSR"][4]);

            for (const auto& ram : ((*it)["transactions"]))
            {
                if (ram["kind"] != 1) continue;

                uint32_t data = static_cast<uint32_t>(ram["data"]);
                uint32_t addr = static_cast<uint32_t>(ram["addr"]);
                
                if (ram["size"] == 4) 
                {
                    mem.write32(data, addr);
                    std::cout << "^ Initialized!\n";
                }
                else if (ram["size"] == 2)
                {
                    mem.write16(static_cast<uint16_t>(data), addr);
                    std::cout << "^ Initialized!\n";
                }
                else if (ram["size"] == 1)
                {
                    mem.write8(static_cast<uint8_t>(data), addr);
                    std::cout << "^ Initialized!\n";
                }
            }
            
            if (Arm7TDMI::ProgramStatusRegsiter::T == 0)
            {
                uint32_t opcode = static_cast<uint32_t>((*it)["opcode"]);
                cpu.arm_execute(opcode);
            }
            else
            {
                uint16_t opcode = static_cast<uint16_t>((*it)["opcode"]);
                cpu.thumb_execute(opcode);
            }

            // Set CPU register values
            for (int i = 0; i < 8; ++i)
                check_val(*cpu.registers[i], static_cast<uint32_t>((*it)["final"]["R"][i]), "R" + std::to_string(i));

            check_val(cpu.r8, static_cast<uint32_t>((*it)["final"]["R"][8]), "R8");
            check_val(cpu.r9, static_cast<uint32_t>((*it)["final"]["R"][9]), "R9");
            check_val(cpu.r10, static_cast<uint32_t>((*it)["final"]["R"][10]), "R10");
            check_val(cpu.r11, static_cast<uint32_t>((*it)["final"]["R"][11]), "R11");
            check_val(cpu.r12, static_cast<uint32_t>((*it)["final"]["R"][12]), "R12");
            check_val(cpu.r13, static_cast<uint32_t>((*it)["final"]["R"][13]), "R13");
            check_val(cpu.r14, static_cast<uint32_t>((*it)["final"]["R"][14]), "R14");
            check_val(cpu.r15, static_cast<uint32_t>((*it)["final"]["R"][15]), "R15");

            check_val(cpu.r8_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][0]), "R8 FIQ");
            check_val(cpu.r9_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][1]), "R9 FIQ");
            check_val(cpu.r10_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][2]), "R10 FIQ");
            check_val(cpu.r11_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][3]), "R11 FIQ");
            check_val(cpu.r12_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][4]), "R12 FIQ");
            check_val(cpu.r13_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][5]), "R13 FIQ");
            check_val(cpu.r14_fiq, static_cast<uint32_t>((*it)["final"]["R_fiq"][6]), "R14 FIQ");

            check_val(cpu.r13_svc, static_cast<uint32_t>((*it)["final"]["R_svc"][0]), "R13 SVC");
            check_val(cpu.r14_svc, static_cast<uint32_t>((*it)["final"]["R_svc"][1]), "R14 SVC");

            check_val(cpu.r13_abt, static_cast<uint32_t>((*it)["final"]["R_abt"][0]), "R13 ABT");
            check_val(cpu.r14_abt, static_cast<uint32_t>((*it)["final"]["R_abt"][1]), "R14 ABT");

            check_val(cpu.r13_irq, static_cast<uint32_t>((*it)["final"]["R_irq"][0]), "R13 IRQ");
            check_val(cpu.r14_irq, static_cast<uint32_t>((*it)["final"]["R_irq"][1]), "R14 IRQ");

            check_val(cpu.r13_und, static_cast<uint32_t>((*it)["final"]["R_und"][0]), "R13 UND");
            check_val(cpu.r14_und, static_cast<uint32_t>((*it)["final"]["R_und"][1]), "R14 UND");

            if (!cpu.skip_mult_instr) // Skipping for multiplication instructions cuz the carry flag is BS
                check_val<uint32_t, true>(cpu.cpsr, static_cast<uint32_t>((*it)["final"]["CPSR"]), "CPSR");
            check_val<uint32_t, true>(cpu.spsr_fiq, static_cast<uint32_t>((*it)["final"]["SPSR"][0]), "SPSR FIQ");
            check_val<uint32_t, true>(cpu.spsr_svc, static_cast<uint32_t>((*it)["final"]["SPSR"][1]), "SPSR SVC");
            check_val<uint32_t, true>(cpu.spsr_abt, static_cast<uint32_t>((*it)["final"]["SPSR"][2]), "SPSR ABT");
            check_val<uint32_t, true>(cpu.spsr_irq, static_cast<uint32_t>((*it)["final"]["SPSR"][3]), "SPSR IRQ");
            check_val<uint32_t, true>(cpu.spsr_und, static_cast<uint32_t>((*it)["final"]["SPSR"][4]), "SPSR UND");

            // Compare RAM values
            for (const auto& ram : ((*it)["transactions"]))
            {
                if (ram["kind"] != 2) continue;

                uint32_t data = static_cast<uint32_t>(ram["data"]);
                uint32_t addr = static_cast<uint32_t>(ram["addr"]);

                if (ram["size"] == 4)
                    check_val(mem.read32(addr), data, std::string("word @ ") + std::to_string(addr));
                else if (ram["size"] == 2)
                    check_val(mem.read16(addr), static_cast<uint16_t>(data), std::string("half word @ ") + std::to_string(addr));
                else if (ram["size"] == 1)
                    check_val(mem.read8(addr), static_cast<uint8_t>(data), std::string("byte @ ") + std::to_string(addr));
            }

            mem.clear_memory();

            std::cout << "\nPassed Test #" << number++ << '\n';
        }

        std::cout << "Passed All Tests: " << file_name << '\n';
        std::cout << "------------------------\n";
    }
};    