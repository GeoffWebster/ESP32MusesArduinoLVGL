/*Rev 1 - amended mas6116 construct  so that initial state has MUTE pin LOW (muted)*/

#include "mas6116.h"
#include <Arduino.h>
#include <SPI.h>

mas6116::mas6116(int mutePin, int mas_CS)
{
	SPI.begin();
	_mas_CS = mas_CS;
	_mutePin = mutePin;
	pinMode(_mutePin, OUTPUT);
	digitalWrite(_mutePin, LOW);
	pinMode(_mas_CS, OUTPUT);
	digitalWrite(_mas_CS, HIGH);
}

void mas6116::mas6116Mute(unsigned char value)
{
	digitalWrite(_mutePin, value);
}

void mas6116::mas6116Write(unsigned char mas6116Reg, unsigned char value)
{
	// mas6116 datasheet recommends setting "don't care" bits to HIGH
	unsigned char address = (mas6116Reg & mas6116RegMask) | ~(mas6116RegMask | mas6116ReadBit);
	//set MAS6116 SPI frequency, then MAS6116 CS pin low to select the vol chip
	begin_mas6116_read_write();
	//  send in the address and value via SPI
	SPI.transfer(address);
	SPI.transfer(value);
	// Reset the SPI frequency, then MAS CS pin high to deselect the vol chip
	end_mas6116_read_write();
	
}

unsigned char mas6116::mas6116Read(unsigned char mas6116Reg)
{
	// mas6116 datasheet recommends setting "don't care" bits to HIGH
	unsigned char address = (mas6116Reg & mas6116RegMask) | ~mas6116RegMask | mas6116ReadBit;
	unsigned char result = 0;
	//set MAS6116 SPI frequency, then MAS6116 CS pin low to select the vol chip
	begin_mas6116_read_write();
	SPI.transfer(address);
	// Read value via SPI (any value can be written when reading)
	result = SPI.transfer(0);
	//Reset SPI frequency, then MAS6116 CS pin high to deselect the vol chip
	end_mas6116_read_write();
	return result;
}

/***************************************************************************************
** Function name:           begin_mas6116_read_write
** Description:             Start transaction and select volume controller
***************************************************************************************/
// The volume controller has a low SPI clock rate
inline void mas6116::begin_mas6116_read_write(void){
	SPI.setFrequency(SPI_MAS6116_FREQUENCY);
	MAS_CS_L;
}

/***************************************************************************************
** Function name:           end_mas6116_read_write
** Description:             End transaction and deselect mas6116 controller
***************************************************************************************/
inline void mas6116::end_mas6116_read_write(void){
	SPI.setFrequency(SPI_FREQUENCY);
	MAS_CS_H;
}
