#pragma once

#include "memory.hpp"
#include "memory_regions.hpp"
#include "io_register.hpp"
#include "interrupts.hpp"
#include "scheduler.hpp"

class DMA
{
public: 
    DMA(Memory& memory, Scheduler& scheduler);

    void handle_dma_event(Scheduler::EventType dma_event);

private:
    enum DMAControl
    {
        Unused = 0x1F,
        DstAddrControl = (0b11 << 5),
        SrcAddrControl = (0b11 << 7),
        DMARepeat = (1 << 9),
        DMATransferType = (1 << 10),
        GamePakDRQ = (1 << 11),
        DMAStartTiming = (0b11 << 12),
        IRQOnEnd = (1 << 14),
        DMAEnable = (1 << 15)
    };

    enum DestinationAdjustment
    {
        IncrementAfter = (0b00 << 5),
        DecrementAfter = (0b01 << 5),
        None = (0b10 << 5),
        IncrementReload = (0b11 << 5)
    };

    enum SourceAdjustment
    {
        IncrementAfter = (0b00 << 7),
        DecrementAfter = (0b01 << 7),
        None = (0b10 << 7),
        Forbidden = (0b11 << 7)
    };

    enum TimingMode 
    {
        Now = (0b00 << 12),
        OnVBlank = (0b01 << 12),
        OnHBlank = (0b10 << 12),
        OnRefresh = (0b11 << 12)
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

    template<uint32_t T, uint32_t U, uint32_t V>
    void transfer(Io32<T> src_addr, Io32<U> dst_addr, Io16<V> dma_control, int chunk_transfer_amount);
};