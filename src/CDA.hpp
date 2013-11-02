#pragma once
/**
 * RC3200 VM - CDA.hpp
 * Color Display Adapter device
 *
 */

#ifndef __CDA_HPP__
#define __CDA_HPP__ 1

#include "types.hpp"
#include "ram.hpp"
#include "IDevice.hpp"

#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>

#include <cassert>


namespace vm {
namespace cda {

unsigned const VSYNC = 25; // Vertical refresh frecuency

class CDA : public IDevice {
public:

CDA() : count(0), videomode(0), textmode(true), blink(false), userfont(false), e_vsync(false), vram(this), setupr(this) {
}

virtual ~CDA() {
}

byte_t DevClass() const     {return 0x0E;}   // Graphics device
word_t Builder() const      {return 0x0000;} // Generic builder
word_t DevId() const        {return 0x0001;} // CDA standard
word_t DevVer() const       {return 0x0000;} // Ver 0 -> CDA base standard

virtual void Tick (cpu::RC3200& cpu, unsigned n=1) {
    count += n;
    if (count >= (cpu.Clock()/VSYNC)) { // V-Sync Event
        count -= cpu.Clock()/VSYNC;
        if (e_vsync)
            cpu.ThrowInterrupt(0x0000005A);
    }
}

virtual std::vector<ram::AHandler*> MemoryBlocks() const { 
    auto handlers = IDevice::MemoryBlocks(); 
    handlers.push_back((ram::AHandler*)&vram);
    handlers.push_back((ram::AHandler*)&setupr);

    return handlers;
}

protected:

/**
 * Address Handler that manages the VideoRAM
 */
class VideoRAM : public ram::AHandler {
public:
    VideoRAM (CDA* cda) : AHandler(0xFF0A0000, 0x4400), vram(NULL) {
        this->cda = cda;
        vram = new byte_t[this->size]();
    }

    virtual ~VideoRAM() {
        if (vram != NULL)
            delete[] vram;
    }

    byte_t RB (dword_t addr) {
        addr -= this->begin;
        assert(addr < this->size);
        assert(addr >= 0);

        return vram[addr];
    }

    void WB (dword_t addr, byte_t val) {
        //std::printf("CDA -> ADDR: %08Xh VAL : 0x%02X\n", addr, val);
        addr -= this->begin;
        assert(addr < this->size);
        assert(addr >= 0);
        
        vram[addr] = val;
    }

    byte_t* vram;

    CDA* cda; // Self-reference
};

/**
 * Address block that manages the SETUP register
 */
class SETUPreg : public ram::AHandler {
public:
    SETUPreg (CDA* cda) : AHandler(0xFF0ACC00, 1), reg(0) {
        this->cda = cda;
    }

    byte_t RB (dword_t addr) {
        return reg;
    }

    void WB (dword_t addr, byte_t val) {
        //std::printf("CDA -> ADDR: %08Xh VAL : 0x%08X\n", addr, val);
        reg = val;
        cda->videomode = val & 3; 
        cda->textmode = (val & 4) == 0; 
    }

    byte_t reg;
    CDA* cda; // Self-reference

};

unsigned count;         /// Cycle counter

unsigned videomode;     /// Actual video mode
bool textmode;          /// Is text mode ?
bool blink;             /// Blink attribute in textmode ?
bool userfont;          /// User Font ?
bool e_vsync;           /// Enable V-Sync interrupt ?

CDA::VideoRAM vram;
CDA::SETUPreg setupr;

};


} // End of namespace cda
} // End of namespace vm

#endif
