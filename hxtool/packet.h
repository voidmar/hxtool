#pragma once

#include <cstdint>
#include "serial.h"

struct Packet
{
    uint16_t length = 0;
    uint8_t payload_length = 0;
    uint8_t payload[255] = { 0 };
    bool valid = true;

    bool AddByte(uint8_t byte)
    {
        int idx = length++;

        if (idx == 0 && byte != 0x0b)
            return valid = false;
        else if (idx == 1 && byte != 0x0e)
            return valid = false;
        else if (idx == 2)
            payload_length = byte;
        else if (idx > 2 && idx < 3 + payload_length)
            payload[length - 4] = byte;

        if (idx == payload_length + 2)
        {
            int checksum = payload_length;
            for (int i = 0; i < payload_length - 1; ++i)
                checksum += payload[i];
            if ((checksum & 0xFF) != byte)
                return valid = false;
        }
        else if (idx == payload_length + 3 && byte != 0x0d)
            return valid = false;
        else if (idx == payload_length + 4 && byte != 0x0a)
            return valid = false;
        else if (idx == payload_length + 5 && byte != 0x0f)
            return valid = false;

        if (idx == payload_length + 5)
        {
            return false;
        }

        return true;
    }

    void Reset()
    {
        valid = true;
        length = 0;
        payload_length = 0;
    }
};

inline bool ReadPacket(SerialPort& port, Packet& packet)
{
    uint8_t buffer = 0;
    DWORD bytes_read = 0;

    do
    {
        if (ReadFile(port.handle, &buffer, 1, &bytes_read, NULL) && bytes_read)
        {
            if (!packet.AddByte(buffer))
                return packet.valid;
        }
    } while (bytes_read || packet.valid);

    return packet.valid;
}

inline void WritePacket(SerialPort& port, Packet& packet)
{
    uint8_t buffer[256 + 6];
    buffer[0] = 0x0b;
    buffer[1] = 0x0e;
    buffer[2] = packet.payload_length;
    memcpy(buffer + 3, packet.payload, packet.payload_length);
    buffer[3 + packet.payload_length] = 0x0d;
    buffer[4 + packet.payload_length] = 0x0a;
    buffer[5 + packet.payload_length] = 0x0f;

    DWORD bytes_written = 0;
    WriteFile(port.handle, buffer, packet.payload_length + 6, &bytes_written, nullptr);

//     int res = WriteFileEx(port.handle, buffer, packet.payload_length + 6, get_free_async_writer(), &finish_write_packet);
//     int gle = GetLastError();
//     DWORD event_mask = 0;
//     do
//     {
//         WaitCommEvent(port.handle, &event_mask, NULL);
//     } while (!(event_mask & EV_TXEMPTY));
}

struct PacketPayload
{
    uint8_t type_id;
};

struct I2CAppPayload : public PacketPayload
{
    uint8_t bus_id;
    uint8_t device_id;
    uint8_t register_id;
};

#define PAYLOAD_I2C_READ 0x52
struct I2CReadPayload : public I2CAppPayload
{

};

#define PAYLOAD_I2C_WRITE 0x57
struct I2CWritePayload : public I2CAppPayload
{
    uint8_t value;
};

#define PAYLOAD_I2C_READ_ACK 0x52
struct I2CReadResponsePayload : public PacketPayload
{
    uint8_t value;
};

#define PAYLOAD_I2C_WRITE_ACK 0x58
struct I2CWriteResponsePayload : public I2CAppPayload
{

};

inline void CreateI2CReadPacket(Packet& packet, uint8_t bus, uint8_t dev, uint8_t reg)
{
    packet.payload_length = sizeof(I2CReadPayload) + 1;

    auto write_payload = (I2CReadPayload*)packet.payload;
    write_payload->type_id = PAYLOAD_I2C_READ;
    write_payload->bus_id = bus;
    write_payload->device_id = dev;
    write_payload->register_id = reg;

    int checksum = packet.payload_length;
    for (int i = 0; i < packet.payload_length - 1; ++i)
        checksum += packet.payload[i];

    packet.payload[packet.payload_length - 1] = (uint8_t)(checksum & 0xFF);
}

inline void CreateI2CWritePacket(Packet& packet, uint8_t bus, uint8_t dev, uint8_t reg, uint8_t value)
{
    packet.payload_length = sizeof(I2CWritePayload) + 1;

    auto write_payload = (I2CWritePayload*)packet.payload;
    write_payload->type_id = PAYLOAD_I2C_WRITE;
    write_payload->bus_id = bus;
    write_payload->device_id = dev;
    write_payload->register_id = reg;
    write_payload->value = value;

    int checksum = packet.payload_length;
    for (int i = 0; i < packet.payload_length - 1; ++i)
        checksum += packet.payload[i];

    packet.payload[packet.payload_length - 1] = (uint8_t)(checksum & 0xFF);
}