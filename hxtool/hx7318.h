#pragma once

#include "serial.h"
#include "hxregister.h"

struct HX7318RegisterTable
{
    HXRegister control_00{ 0x00 };
    HXRegister control_01{ 0x01 };
    HXRegister com_n{ 0x19 }, com_p{ 0x1A };
    HXRegister ring_n{ 0x1D }, ring_p{ 0x1E };
    HXRegister gamma_table_r[16]
    {
        HXRegister{ 0x21 }, HXRegister{ 0x27 }, HXRegister{ 0x2D }, HXRegister{ 0x33 }, HXRegister{ 0x39 }, HXRegister{ 0x3F }, HXRegister{ 0x45 }, HXRegister{ 0x4B },
        HXRegister{ 0x24 }, HXRegister{ 0x2A }, HXRegister{ 0x30 }, HXRegister{ 0x36 }, HXRegister{ 0x3C }, HXRegister{ 0x42 }, HXRegister{ 0x48 }, HXRegister{ 0x4E },
    };
    HXRegister gamma_table_g[16]
    {
        HXRegister{ 0x22 }, HXRegister{ 0x28 }, HXRegister{ 0x2E }, HXRegister{ 0x34 }, HXRegister{ 0x3A }, HXRegister{ 0x40 }, HXRegister{ 0x46 }, HXRegister{ 0x4C },
        HXRegister{ 0x25 }, HXRegister{ 0x2B }, HXRegister{ 0x31 }, HXRegister{ 0x37 }, HXRegister{ 0x3D }, HXRegister{ 0x43 }, HXRegister{ 0x49 }, HXRegister{ 0x4F },
    };
    HXRegister gamma_table_b[16]
    { 
        HXRegister{ 0x23 }, HXRegister{ 0x29 }, HXRegister{ 0x2F }, HXRegister{ 0x35 }, HXRegister{ 0x3B }, HXRegister{ 0x41 }, HXRegister{ 0x47 }, HXRegister{ 0x4D },
        HXRegister{ 0x26 }, HXRegister{ 0x2C }, HXRegister{ 0x32 }, HXRegister{ 0x38 }, HXRegister{ 0x3E }, HXRegister{ 0x44 }, HXRegister{ 0x4A }, HXRegister{ 0x50 },
    };

    void iterate_all_registers(std::function<void(HXRegister& r)> func)
    {
        func(control_00);
        func(control_01);
        func(com_n);
        func(com_p);
        func(ring_n);
        func(ring_p);
        for (int i = 0; i < 16; ++i)
        {
            func(gamma_table_r[i]);
            func(gamma_table_g[i]);
            func(gamma_table_b[i]);
        }
    }

    void make_dirty()
    {
        iterate_all_registers([](auto& r) { r.dirty |= true; });
    }

    void read_from_device(SerialPort& port, bool make_dirty = false, uint8_t dev = 0x92)
    {
        iterate_all_registers([&port, make_dirty, dev](auto& r) { r.read_from_device(port, dev); r.dirty |= make_dirty; });
    }

    void copy_to_initial()
    {
        iterate_all_registers([](auto& r) { r.initial = r.value; });
    }

    void reset_from_initial()
    {
        iterate_all_registers([](auto& r) { r.value = r.initial; });
    }

    void write_to_device(SerialPort& port, bool dirty_only = true, uint8_t dev = 0x92)
    {
        iterate_all_registers([&port, dirty_only, dev](auto& r)
        {
            if (!dirty_only || r.dirty)
            {
                r.write_to_device(port, dev); 
                r.dirty = false;
            }
        });
    }
};


