#include "arm7dissassembler.hpp"

std::string Arm7Dissassembler::arm_branch_dissassemble(uint32_t opcode) { return ""; } 

std::string Arm7Dissassembler::arm_branch_and_exchange_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_block_data_transfer_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_coprocessor_data_operation_dissassemble(uint32_t opcode) { return "UND"; }

std::string Arm7Dissassembler::arm_coprocessor_data_transfer_dissassemble(uint32_t opcode) { return "UND"; }

std::string Arm7Dissassembler::arm_coprocessor_register_transfer_dissassemble(uint32_t opcode) { return "UND"; }

std::string Arm7Dissassembler::arm_data_processing_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_halfword_data_transfer_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_multiply_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_multiply_long_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_psr_transfer_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_software_interrupt_dissassemble(uint32_t opcode) 
{ 
    int value24 = Utils::get_bits(opcode, 0, 24);

    return "SWI " + std::to_string(value24);
}

std::string Arm7Dissassembler::arm_single_data_swap_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_single_data_transfer_dissassemble(uint32_t opcode) { return ""; }

std::string Arm7Dissassembler::arm_undefined_dissassemble(uint32_t opcode) 
{ 
    return "UND"; 
}
