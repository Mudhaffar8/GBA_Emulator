#include "dma.hpp"

DMA::DMA(Memory& _memory, Scheduler& _scheduler) :
    memory(_memory),
    scheduler(_scheduler),
    dma0_src_addr(memory),
    dma1_src_addr(memory),
    dma2_src_addr(memory),
    dma3_src_addr(memory),
    dma0_dst_addr(memory),
    dma1_dst_addr(memory),
    dma2_dst_addr(memory),
    dma3_dst_addr(memory),
    dma0_word_count(memory),
    dma1_word_count(memory),
    dma2_word_count(memory),
    dma3_word_count(memory),
    dma0_control(memory),
    dma1_control(memory),
    dma2_control(memory),
    dma3_control(memory)
{}

void DMA::handle_dma_event(Scheduler::EventType dma_event)
{
    switch(dma_event)
    {
        case Scheduler::EventType::DMA0: 
            transfer(dma0_src_addr, dma0_dst_addr, dma0_control, dma0_word_count); 
            
            if (dma0_control & DMAControl::IRQOnEnd) 
                Interrupts::request_interrupt(memory, InterruptType::DMA0);
            break;
            
        case Scheduler::EventType::DMA1: 
            transfer(dma1_src_addr, dma1_dst_addr, dma1_control, dma1_word_count); 
            
            if (dma1_control & DMAControl::IRQOnEnd) 
                Interrupts::request_interrupt(memory, InterruptType::DMA1);
            break;
        case Scheduler::EventType::DMA2: 
            transfer(dma2_src_addr, dma2_dst_addr, dma2_control, dma2_word_count); 
            
            if (dma2_control & DMAControl::IRQOnEnd) 
                Interrupts::request_interrupt(memory, InterruptType::DMA2);
            break;
        case Scheduler::EventType::DMA3: 
            transfer(dma3_src_addr, dma3_dst_addr, dma3_control, dma3_word_count); 
            
            if (dma3_control & DMAControl::IRQOnEnd) 
                Interrupts::request_interrupt(memory, InterruptType::DMA3);
            break;

        default: 
            std::runtime_error("Invalid DMA Event: " + std::to_string(static_cast<int>(dma_event))); 
            break;
    }
}

template<uint32_t T, uint32_t U, uint32_t V>
void DMA::transfer(Io32<T> src_addr, Io32<U> dst_addr, Io16<V> dma_control, int chunk_transfer_amount)
{
    int dst_addr_control = Utils::get_bits(dma_control, 5, 7);
    int src_addr_control = Utils::get_bits(dma_control, 7, 9);
    bool dma_repeat = Utils::is_bit_set(dma_control, 9);
    bool transfer_32bit = Utils::is_bit_set(dma_control, 10);
    bool irq_on_end = Utils::is_bit_set(dma_control, 14);

    int transfer_amount = transfer_32bit ? 4 : 2;
    uint32_t old_dst_addr = dst_addr;

    int dst_transfer_offset{};
    switch(dst_addr_control)
    {
        case DestinationAdjustment::IncrementAfter: 
        case DestinationAdjustment::IncrementReload:
            dst_transfer_offset = transfer_amount;
            break;
        case DestinationAdjustment::DecrementAfter: 
            dst_transfer_offset = -transfer_amount; 
            break;
        case DestinationAdjustment::None: 
            dst_transfer_offset = 0;
            break;
    }

    int src_transfer_offset{};
    switch(dst_addr_control)
    {
        case SourceAdjustment::IncrementAfter: 
            src_transfer_offset = transfer_amount;
            break;
        case SourceAdjustment::DecrementAfter: 
            src_transfer_offset = -transfer_amount; 
            break;
        case SourceAdjustment::None:
            src_transfer_offset = 0;
            break;

        case SourceAdjustment::Forbidden: break;
    }

    for (int i = 0; i < chunk_transfer_amount; ++i)
    {
        if (transfer_32bit)
        {
            uint32_t word = memory.read<uint32_t>(src_addr);
            memory.write<uint32_t>(dst_addr, word);
        }
        else
        {
            uint16_t half_word = memory.read<uint16_t>(src_addr);
            memory.write<uint16_t>(dst_addr, word);
        }

        dst_addr += dst_transfer_offset;
        src_addr += src_transfer_offset;
    }

    // When accessing OAM (7000000h) or OBJ VRAM (6010000h) by HBlank Timing, 
    // then the “H-Blank Interval Free” bit in DISPCNT register must be set.
    if (dst_addr_control == DestinationAdjustment::IncrementReload) dst_addr = old_dst_addr;
}