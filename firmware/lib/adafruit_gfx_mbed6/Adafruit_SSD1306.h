/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

/*
 *  Modified by Neal Horman 7/14/2012 for use in mbed
 */

#ifndef _ADAFRUIT_SSD1306_H_
#define _ADAFRUIT_SSD1306_H_

#include "mbed.h"
#include "Adafruit_GFX.h"

#include <vector>
#include <algorithm>

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

/** The pure base class for the SSD1306 display driver.
 *
 * You should derive from this for a new transport interface type,
 * such as the SPI and I2C drivers.
 */
class Adafruit_SSD1306 : public Adafruit_GFX
{
public:
	explicit Adafruit_SSD1306(PinName RST, uint8_t rawHeight = 32, uint8_t rawWidth = 128)
		: Adafruit_GFX(rawWidth,rawHeight)
		, rst(RST,false)
	{
		buffer.resize(rawHeight * rawWidth / 8);
	};

	void begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC);
	
	// These must be implemented in the derived transport driver
	virtual void command(uint8_t c) = 0;
	virtual void data(uint8_t c) = 0;
	void drawPixel(int16_t x, int16_t y, uint16_t color) override;

	/// Clear the display buffer    
	void clearDisplay();
	void invertDisplay(bool i) override;

	/// Cause the display to be updated with the buffer content.
	void display();
	/// Fill the buffer with the AdaFruit splash screen.
	virtual void splash();
    
protected:
	virtual void sendDisplayBuffer() = 0;
	mbed::DigitalOut rst;

	// the memory buffer for the LCD
	std::vector<uint8_t> buffer;
};


/** This is the SPI SSD1306 display driver transport class
 *
 */
class Adafruit_SSD1306_Spi : public Adafruit_SSD1306
{
public:
	/** Create a SSD1306 SPI transport display driver instance with the specified DC, RST, and CS pins, as well as the display dimentions
	 *
	 * Required parameters
	 * @param spi - a reference to an initialized SPI object
	 * @param DC (Data/Command) pin name
	 * @param RST (Reset) pin name
	 * @param CS (Chip Select) pin name
	 *
	 * Optional parameters
	 * @param rawHeight - the vertical number of pixels for the display, defaults to 32
	 * @param rawWidth - the horizonal number of pixels for the display, defaults to 128
	 */
	Adafruit_SSD1306_Spi(mbed::SPI &spi, PinName DC, PinName RST, PinName CS, uint8_t rawHieght = 32, uint8_t rawWidth = 128)
	    : Adafruit_SSD1306(RST, rawHieght, rawWidth)
	    , cs(CS,true)
	    , dc(DC,false)
	    , mspi(spi)
	    {
		    begin();
		    splash();
		    display();
	    };

	void command(uint8_t c) override
	{
	    cs = 1;
	    dc = 0;
	    cs = 0;
	    mspi.write(c);
	    cs = 1;
	};

	void data(uint8_t c) override
	{
	    cs = 1;
	    dc = 1;
	    cs = 0;
	    mspi.write(c);
	    cs = 1;
	};

protected:
	void sendDisplayBuffer() override
	{
		cs = 1;
		dc = 1;
		cs = 0;

		for(unsigned char i : buffer)
			mspi.write(i);

		if(height() == 32)
		{
			for(uint16_t i=0, q=buffer.size(); i<q; i++)
				mspi.write(0);
		}

		cs = 1;
	};

	mbed::DigitalOut cs, dc;
	mbed::SPI &mspi;
};

#endif