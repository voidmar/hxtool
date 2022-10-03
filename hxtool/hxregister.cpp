#include "serial.h"
#include "packet.h"
#include "hxregister.h"
#include <assert.h>
#include <stdio.h>

void WritePacketVerify(SerialPort& port, Packet& packet)
{
    WritePacket(port, packet);

    Packet response;
    if (!ReadPacket(port, response))
    {
        printf("failed to read packet from device\n");
        exit(1);
    }

    assert(response.payload[0] == PAYLOAD_I2C_WRITE_ACK || (response.payload[0] == 'M' && response.payload[1] == 'O'));
}

void HX16318_SendConnectPacket(SerialPort& port)
{
    Packet packet;
    CreateI2CWritePacket(packet, 0, 0x58, 0x79, 0x5a);
    WritePacketVerify(port, packet);
}

void HX16318_SendShutdownPacket(SerialPort& port)
{
    Packet packet;
    CreateI2CWritePacket(packet, 0, 0x58, 0x59, 0x7a);
    WritePacketVerify(port, packet);
}

bool HX7318_WriteRegister(SerialPort& port, uint8_t dev, uint8_t reg, uint8_t value)
{
    Packet packet;
    CreateI2CWritePacket(packet, 0, dev, reg, value);
    WritePacketVerify(port, packet);

    return true;
}

bool HX7318_ReadRegister(SerialPort& port, uint8_t dev, uint8_t reg, uint8_t* value)
{
    Packet packet;
    CreateI2CReadPacket(packet, 0, dev, reg);
    WritePacket(port, packet);

    Packet response;
    if (!ReadPacket(port, response))
    {
        printf("failed to read packet from device\n");
        return false;
    }

    if (response.payload[0] != PAYLOAD_I2C_READ)
        return false;

    if (response.payload_length != 3)
        return false;

    auto payload = (I2CReadResponsePayload*)response.payload;
    *value = payload->value;
    return true;
}
