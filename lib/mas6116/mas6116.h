#ifndef MAS6116_H
#define MAS6116_H

#include <Arduino.h>
#include <SPI.h>

#define SPI_MAS6116_FREQUENCY 8000000 //Spec max is 10000000

// mas6116 register addresses
// Bits 6..3 contain the actual register address
// Bits 7, 1, and 0 are "don't care", set to 0 here
// Bit 2 is 1 for read and 0 for write
const int mas6116RegRight = 0x0E << 3;  // R1 - right channel gain (read/write)
const int mas6116RegLeft = 0x0D << 3;   // R2 - left channel gain (read/write)
const int mas6116RegRef = 0x0C << 3;    // R3 - peak detector reference (read/write)
const int mas6116RegStatus = 0x0B << 3; // R4 - peak detector status (read/write)
const int mas6116RegBoth = 0x09 << 3;   // R5 - both channels gain (write only)
const int mas6116RegMask = 0x0F << 3;   // mask for register address
const int mas6116ReadBit = 0x01 << 2;   // set bit 2 in the address to 1 for read, to 0 for write

class mas6116
{
private:
    int _mutePin;
    int _mas_CS;
#define MAS_CS_L                    \
    GPIO.out_w1tc = (1 << _mas_CS); \
    GPIO.out_w1tc = (1 << _mas_CS)
#define MAS_CS_H GPIO.out_w1ts = (1 << _mas_CS)  //;GPIO.out_w1ts = (1 << _mas_CS)
    bool locked, inTransaction, lockTransaction; // SPI transaction and mutex lock flags

    inline void begin_mas6116_read_write(void);
    inline void end_mas6116_read_write(void);

public:
    int mutePin;
    int mas_CS;
    mas6116(int mutePin, int mas_CS);

    void mas6116Write(unsigned char mas6116Reg, unsigned char value);
    void mas6116Mute(unsigned char);
    unsigned char mas6116Read(unsigned char mas6116Reg);
};

#endif
