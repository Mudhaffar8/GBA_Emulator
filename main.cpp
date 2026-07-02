#include <iostream>
#include <bitset>

#include "utils.hpp"

int main()
{
    int offset = Utils::sign_extend32(static_cast<int8_t>(-34), 0, 8);
    std::cout << offset << '\n';

    int bit_range = Utils::get_bits(0b10110101, 3, 8);
    std::cout << std::bitset<5>(bit_range) << '\n';

    int zero_bit = Utils::is_bit_set(0b10110101, 3);
    std::cout << zero_bit << '\n';

    int one_bit = Utils::is_bit_set(0b10110101, 4);
    std::cout << one_bit << '\n';

    return 0;
}