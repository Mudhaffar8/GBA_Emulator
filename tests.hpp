#include <iostream>
#include <fstream>
#include <bitset>
#include <cassert>
#include <stdexcept>
#include <sstream>
#include <string>
#include <type_traits>

#include "arm7.hpp"
#include "utils.hpp"
#include "memory.hpp"

#include "json.hpp"

using json = nlohmann::json;

class GBATests
{
public:
    template<typename T, bool print_bits = false>
    static void check_val(T val, T other_val, std::string name)
    {   
        if (val != other_val) 
        {
            // To prevent printing the ascii character instead of number for uint8_t, uint16_t etc.
            if constexpr (std::is_integral<T>::value) 
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

    static void run_test(Arm7TDMI& cpu, TestMemory& mem, std::string file_name)
    {  
        std::ifstream file("./v1/" + file_name);
        if (!file) 
        {
            std::cout << "File Not Found!\n";
            return;
        }

        std::cout << "Parsing file: " << file_name << '\n';

        json data = json::parse(file);   

        std::cout << "Running Test File: " << file_name << '\n';

        int number = 0;
        for (json::iterator it = data.begin(); it != data.end(); ++it)
        {
            ++number;
            cpu.skip_mult_instr = false;
            
            cpu.cpsr = static_cast<uint32_t>((*it)["initial"]["CPSR"]);
            cpu.handle_mode_switch(cpu.cpsr & Arm7TDMI::ProgramStatusRegsiter::Mode);

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
                    mem.write<uint32_t>(data, addr);
                    Utils::print("^ Initialized!\n");
                }
                else if (ram["size"] == 2)
                {
                    mem.write<uint16_t>(static_cast<uint16_t>(data), addr);
                    Utils::print("^ Initialized!\n");
                }
                else if (ram["size"] == 1)
                {
                    mem.write<uint8_t>(static_cast<uint8_t>(data), addr);
                    Utils::print("^ Initialized!\n");
                }
            }
            
            if (cpu.is_thumb_mode())
            {
                uint16_t opcode = static_cast<uint16_t>((*it)["opcode"]);
                cpu.thumb_execute(opcode);
            }
            else
            {
                uint32_t opcode = static_cast<uint32_t>((*it)["opcode"]);
                cpu.arm_execute(opcode);
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
                    check_val(mem.read<uint32_t>(addr), data, std::string("word @ ") + std::to_string(addr));
                else if (ram["size"] == 2)
                    check_val(mem.read<uint16_t>(addr), static_cast<uint16_t>(data), std::string("half word @ ") + std::to_string(addr));
                else if (ram["size"] == 1)
                    check_val(mem.read<uint8_t>(addr), static_cast<uint8_t>(data), std::string("byte @ ") + std::to_string(addr));
            }

            mem.clear();

            std::cout << "Passed Test #" << std::dec << number << '\n';
        }

        std::cout << "Passed All Tests: " << file_name << '\n';
        std::cout << "------------------------\n";
    }

    static void run_all_tests(Arm7TDMI& cpu, TestMemory& memory, bool run_arm = true, bool run_thumb = true)
    {  
        // Passes All ARM Tests
        if (run_arm)
        {
            GBATests::run_test(cpu, memory, "arm_b_bl.json");
            GBATests::run_test(cpu, memory, "arm_bx.json");
            GBATests::run_test(cpu, memory, "arm_data_proc_immediate.json");
            GBATests::run_test(cpu, memory, "arm_data_proc_immediate_shift.json");
            GBATests::run_test(cpu, memory, "arm_data_proc_register_shift.json");
            GBATests::run_test(cpu, memory, "arm_ldr_str_immediate_offset.json");    
            GBATests::run_test(cpu, memory, "arm_ldr_str_register_offset.json");    
            GBATests::run_test(cpu, memory, "arm_ldrh_strh.json");
            GBATests::run_test(cpu, memory, "arm_ldrsb_ldrsh.json");   
            GBATests::run_test(cpu, memory, "arm_ldm_stm.json"); 
            GBATests::run_test(cpu, memory, "arm_mul_mla.json");
            GBATests::run_test(cpu, memory, "arm_mull_mlal.json");
            GBATests::run_test(cpu, memory, "arm_mrs.json");
            GBATests::run_test(cpu, memory, "arm_msr_reg.json");  
            GBATests::run_test(cpu, memory, "arm_msr_imm.json");
            GBATests::run_test(cpu, memory, "arm_swi.json");
            GBATests::run_test(cpu, memory, "arm_swp.json");
        }

        if (run_thumb)
        {
            GBATests::run_test(cpu, memory, "thumb_add_sp_or_pc.json"); 
            GBATests::run_test(cpu, memory, "thumb_add_sub.json");
            GBATests::run_test(cpu, memory, "thumb_add_sub_sp.json");
            GBATests::run_test(cpu, memory, "thumb_b.json"); 
            GBATests::run_test(cpu, memory, "thumb_bcc.json");
            GBATests::run_test(cpu, memory, "thumb_bl_blx_prefix.json"); 
            GBATests::run_test(cpu, memory, "thumb_bl_suffix.json"); 
            GBATests::run_test(cpu, memory, "thumb_bx.json");
            GBATests::run_test(cpu, memory, "thumb_data_proc.json");
            GBATests::run_test(cpu, memory, "thumb_ldm_stm.json");
            GBATests::run_test(cpu, memory, "thumb_ldr_pc_rel.json"); 
            GBATests::run_test(cpu, memory, "thumb_ldr_str_imm_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldr_str_reg_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldr_str_sp_rel.json");
            GBATests::run_test(cpu, memory, "thumb_ldrb_strb_imm_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldrh_strh_imm_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldrh_strh_reg_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldrsb_strb_reg_offset.json");
            GBATests::run_test(cpu, memory, "thumb_ldrsh_ldrsb_reg_offset.json");
            GBATests::run_test(cpu, memory, "thumb_lsl_lsr_asr.json");
            GBATests::run_test(cpu, memory, "thumb_mov_cmp_add_sub.json");
            GBATests::run_test(cpu, memory, "thumb_push_pop.json");
            GBATests::run_test(cpu, memory, "thumb_undefined_bcc.json");
        }
    }
};    