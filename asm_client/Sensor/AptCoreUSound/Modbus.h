//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#pragma once

#include <string>

typedef enum
{
    MODBUS_ERR_NO_ERROR = 0,
    MODBUS_ERR_BAD_RX_CRC,
    MODBUS_ERR_RX_ERROR,
    MODBUS_ERR_RX_TIMEOUT,
    MODBUS_ERR_SERIAL_BASE = 20
} ModbusErrno;

struct HWSerial
{
    std::string device;
    int baud;
    int fd;
};

typedef enum
{
    SERIAL_ERR_NO_ERROR = 0,
    SERIAL_ERR_INVALID_STATE,
    SERIAL_ERR_INVALID_BAUD,
    SERIAL_ERR_OPENING_CONNECTION,
    SERIAL_ERR_WRITE_ERROR,
    SERIAL_ERR_READ_ERROR
} SerialErrno;

struct ModbusState
{
    int slave_id;
    int timeout;
    int tx_enable_gpio;
    float byte_period;
};

class ModbusComms
{
public:
    ModbusComms( HWSerial s, ModbusState mbus );

    ModbusErrno Init();

    // Formats and transmits a Modbus Message onto the bus
    ModbusErrno SendADU( int function, uint8_t *data, int length );

    // Receives incoming Modbus responses.
    ModbusErrno ReceiveADU( int function, uint8_t **data, int *length );

private:
    uint16_t Calc_CRC( uint8_t *data, uint16_t length );
    SerialErrno Serial_Init();
    SerialErrno Serial_Write( const void *data, int length );
    SerialErrno Serial_Read( int timeout, void *data, int max_length, int &length );

    struct HWSerial serial;
    struct ModbusState state;
    uint8_t adu[256 + 4];       // maximum RTU message length
};
