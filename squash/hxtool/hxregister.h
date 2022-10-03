#pragma once
#include <stdint.h>

struct SerialPort;

void WritePacketVerify(SerialPort& port, Packet& packet);

void HX16318_SendConnectPacket(SerialPort& port);

void HX16318_SendShutdownPacket(SerialPort& port);

bool HX7318_WriteRegister(SerialPort& port, uint8_t dev, uint8_t reg, uint8_t value);

bool HX7318_ReadRegister(SerialPort& port, uint8_t dev, uint8_t reg, uint8_t* value);


struct HXRegister
{
    uint8_t id = 0;
    uint8_t value = 0;
    uint8_t initial = 0;
    uint8_t default = 0;
    bool dirty = false;

    HXRegister()
    { }

    explicit HXRegister(uint8_t id) : id(id)
    { }

    //HXRegister(const HXRegister& that) = delete;
    HXRegister& operator = (const HXRegister& that) = delete;

    HXRegister& operator = (uint8_t new_value)
    {
        if (value != new_value)
        {
            value = new_value;
            dirty = true;
        }
        return *this;
    }

    operator uint8_t() const { return value; }

    void read_from_device(struct SerialPort& port, uint8_t dev)
    {
        HX7318_ReadRegister(port, dev, id, &value);
    }

    void write_to_device(struct SerialPort& port, uint8_t dev)
    {
        HX7318_WriteRegister(port, dev, id, value);
    }
};

