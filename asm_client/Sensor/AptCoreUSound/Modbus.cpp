//
// Copyright (c) 2019 AptCore Limited
// MIT License (see LICENSE file)
//

#include "Modbus.h"

#include "../../Utils/Log.h"
#include "../../Utils/Utils.h"

#include <string.h>
#include <chrono>
#include <thread>

#ifdef __unix__
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#ifdef TARGET_RPI
#include <wiringPi.h>
#else
void digitalWrite( int pin, int value ) {};
#endif

#define RS485_TX_ENABLE_PIN 4

// Table of CRC values for high-order byte
static const uint8_t Modbus_table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

// Table of CRC values for low-order byte
static const uint8_t Modbus_table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

ModbusComms::ModbusComms( HWSerial s, ModbusState mbus )
{
    serial = s;
    Serial_Init();

    state = mbus;
    state.byte_period = (float)(10 * 1.0 / serial.baud);
}

// The Modbus CRC calculation is readily and openly available from on-line sources.
uint16_t ModbusComms::Calc_CRC( uint8_t *data, uint16_t length )
{
    uint8_t crc_hi = 0xFF; // high CRC byte initialized
    uint8_t crc_lo = 0xFF; // low CRC byte initialized
    unsigned int i; // will index into CRC lookup

    // Pass through message buffer
    while (length--)
    {
        i = crc_hi ^ *data++;
        crc_hi = Modbus_table_crc_hi[i] ^ crc_lo;
        crc_lo = Modbus_table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}

// Send a data-packet. These follow an AptCore defined format, and are not
// standard Modbus function codes.
ModbusErrno ModbusComms::SendADU( int function, uint8_t *data, int length )
{
    adu[0] = state.slave_id;
    adu[1] = function;
    memcpy( adu + 2, data, length );

    uint16_t crc = Calc_CRC( adu, length + 2 );
    adu[length + 2] = crc >> 8;
    adu[length + 3] = crc & 0x00FF;

    // Send the message
    Serial_Write( adu, length + 4 );

    // Busy wait for the data to be transmitted (sleep can be unpredictable)
    // Add 0.5 byte period to allow for a small delay before transmission
    double start = Get_Time_Monotonic();
    while (Get_Time_Monotonic() - start < state.byte_period * (length + 4.5));

    return MODBUS_ERR_NO_ERROR;
}

// Receive a Modbus response from the USound board.
ModbusErrno ModbusComms::ReceiveADU( int function, uint8_t **data, int *length )
{
    int len = 1, num_bytes = 0;

    *length = 0;

    if (Serial_Read( state.timeout, adu, sizeof( adu ), num_bytes )
        != SERIAL_ERR_NO_ERROR)
    {
        LOG( ERROR ) << "Error on serial read";
    }

    if (num_bytes == 0) { return MODBUS_ERR_RX_TIMEOUT; }

    // Wait for remaining bytes
    while (len > 0)
    {
        // Wait 1ms from the last received byte to create a gap before transmitting again
        if (Serial_Read( 1000, adu + num_bytes, sizeof( adu ) - num_bytes, len )
            != SERIAL_ERR_NO_ERROR)
        {
            return MODBUS_ERR_SERIAL_BASE;
        }
        num_bytes += len;
    }

    // Check the CRC - CRC of the whole message (including CRC) will be 0 for valid CRC
    if (Calc_CRC( adu, num_bytes ) != 0x0000)
    {
        LOG( ERROR ) << "CRC Failure";
        std::this_thread::sleep_for( std::chrono::milliseconds( state.timeout / 1000 ) );
#ifdef __unix__
        tcflush( serial.fd, TCIFLUSH );
#endif
        return MODBUS_ERR_RX_ERROR;
    }

    // Check funcion code is correct
    if (adu[1] == (function | 0x80))
    {
        LOG( ERROR ) << "Modbus received error code " << function;
        return MODBUS_ERR_RX_ERROR;
    }
    else if (adu[1] != function)
    {
        LOG( ERROR ) << "Modbus received incorrect function code " << adu[1] << "expected " << function;
        return MODBUS_ERR_RX_ERROR;
    }

    *data = adu + 2; // Point at the start of data.
    *length = num_bytes - 4; // Address, Func and CRC.

    return MODBUS_ERR_NO_ERROR;
}

SerialErrno ModbusComms::Serial_Init()
{
    LOG( INFO ) << "RaspPi Serial Initialisation";
    LOG( INFO ) << "Device " << serial.device;
    LOG( INFO ) << "Baud " << serial.baud;

#ifdef __unix__
    struct termios tio;
    speed_t baud_define;

    // Open modem device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    serial.fd = open( serial.device.c_str(), O_RDWR | O_NOCTTY );
    if (serial.fd < 0)
    {
        LOG( INFO ) << "Open Failure. Device: " << serial.device.c_str();
        perror( "Open Failure." );
        return SERIAL_ERR_OPENING_CONNECTION;
    }

    // Convert the integer baud rate to a defined value
    switch (serial.baud)
    {
    case   1200: baud_define = B1200; break;
    case   2400: baud_define = B2400; break;
    case   4800: baud_define = B4800; break;
    case   9600: baud_define = B9600; break;
    case  19200: baud_define = B19200; break;
    case  38400: baud_define = B38400; break;
    case  57600: baud_define = B57600; break;
    case 115200: baud_define = B115200; break;
    case 230400: baud_define = B230400; break;
    case 460800: baud_define = B460800; break;
    default: return SERIAL_ERR_INVALID_BAUD;
    }

    // Specified baud, 8 bit, no parity, 1 stop, ignore modem control, enable read
    bzero( &tio, sizeof( tio ) );
    tio.c_cflag = baud_define | CS8 | CLOCAL | CREAD;
    tcsetattr( serial.fd, TCSAFLUSH, &tio );

    // Flush the serial port.
    tcflush( serial.fd, TCIFLUSH );

    LOG( INFO ) << "Opened serial fd: " << serial.fd;
#else
    // Set up a fd for test purposes.
    serial.fd = 12;
#endif
    return SERIAL_ERR_NO_ERROR;
}

SerialErrno ModbusComms::Serial_Write( const void *data, int length )
{
#ifdef __unix__
    if (write( serial.fd, data, length ) != length)
    {
        LOG( ERROR ) << "Write Failed";
        return SERIAL_ERR_WRITE_ERROR;
    }
#endif
    return SERIAL_ERR_NO_ERROR;
}

SerialErrno ModbusComms::Serial_Read( int timeout, void *data, int max_length, int &length )
{
#ifdef __unix__
    fd_set readfds;
    struct timeval timeout_struct;
    int waiting_data;

    length = 0;
    // See if there is data waiting to be read
    timeout_struct.tv_sec = timeout / 1000000;
    timeout_struct.tv_usec = timeout % 1000000;

    FD_ZERO( &readfds );
    FD_SET( serial.fd, &readfds );

    waiting_data = select( serial.fd + 1, &readfds, NULL, NULL, &timeout_struct );
    // See if there is any data available
    if (waiting_data < 0)
    {
        return SERIAL_ERR_READ_ERROR;
    }
    else if (waiting_data > 0)
    {
        // Read the available data
        length = read( serial.fd, data, max_length );
        if (length == -1) return SERIAL_ERR_READ_ERROR;
    }
#else
    LOG( INFO ) << "Serial Read";
#endif
    return SERIAL_ERR_NO_ERROR;
}
