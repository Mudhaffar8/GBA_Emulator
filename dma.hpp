#pragma once

#include "memory.hpp"
#include "memory_regions.hpp"
#include "io_register.hpp"
#include "scheduler.hpp"

class DMA
{
public: 
    DMA(Memory& memory, Scheduler& scheduler);

private:
    enum DMAControl
    {
        NumOfTransfer = 0xF,
        ChunkSize = (1 << 0x1A), // 0 = Halfword, 1 = word
        Repeat = (1 << 0x19), // Repeat at each copy at each VBlank or HBlank
        RaiseIRQ = (1 << 0x1E), // Raise IRQ when finished
        DMAEnable = (1 << 0x1F)
    };

    enum DestinationAdjustment
    {
        IncrementAfter = (0b00 << 0x15),
        DecrementAfter = (0b01 << 0x15),
        None = (0b10 << 0x15),
        IncrementDestination = (0b11 << 0x15)
    };

    enum SourceAdjustment
    {
        IncrementAfter = (0b00 << 0x17),
        DecrementAfter = (0b01 << 0x17),
        None = (0b10 << 0x17),
        Forbidden = (0b11 << 0x17)
    };

    enum TimingMode 
    {
        Now = (0b00 << 0x17),
        OnVBlank = (0b01 << 0x17),
        OnHBlank = (0b10 << 0x17),
        OnRefresh = (0b11 << 0x17), 
    };

    Memory& memory;
    Scheduler& scheduler;

    // DMA 0-3 Source Address
    Io32<GBAIO::DMA0SAD> dma0_src_addr;
    Io32<GBAIO::DMA1SAD> dma1_src_addr;
    Io32<GBAIO::DMA2SAD> dma2_src_addr;
    Io32<GBAIO::DMA3SAD> dma3_src_addr;

    // DMA 0-3 Destination Address
    Io32<GBAIO::DMA0DAD> dma0_dst_addr;
    Io32<GBAIO::DMA1DAD> dma1_dst_addr;
    Io32<GBAIO::DMA2DAD> dma2_dst_addr;
    Io32<GBAIO::DMA3DAD> dma3_dst_addr;

    // DMA 0-3 Word Count
    Io16<GBAIO::DMA0CNT_L> dma0_word_count;
    Io16<GBAIO::DMA1CNT_L> dma1_word_count;
    Io16<GBAIO::DMA2CNT_L> dma2_word_count;
    Io16<GBAIO::DMA3CNT_L> dma3_word_count;

    // DMA 0-3 Control
    Io16<GBAIO::DMA0CNT_H> dma0_control;
    Io16<GBAIO::DMA1CNT_H> dma1_control;
    Io16<GBAIO::DMA2CNT_H> dma2_control;
    Io16<GBAIO::DMA3CNT_H> dma3_control;
};