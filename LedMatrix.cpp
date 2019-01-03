#include <SPI.h>
#include "LedMatrix.h"
#include "cp437font.h"

/**
 * Heavily influenced by the code and the blog posts from https://github.com/nickgammon/MAX7219_Dot_Matrix
 */
LedMatrix::LedMatrix(byte numberOfDevices, byte slaveSelectPin) 
{
    myNumberOfDevices = numberOfDevices;
    mySlaveSelectPin = slaveSelectPin;
    cols = new byte[numberOfDevices * 8];
    spitransfer = new uint16_t[numberOfDevices];
}

/**
 *  numberOfDevices: how many modules are daisy changed togehter
 *  slaveSelectPin: which pin is controlling the CS/SS pin of the first module?
 */
void LedMatrix::init() 
{
    pinMode(mySlaveSelectPin, OUTPUT);

    //Clear anything that has been clocked in accidentally during startup...
    digitalWrite(mySlaveSelectPin,LOW);
    delay(20);
    digitalWrite(mySlaveSelectPin,HIGH);

    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    //SPI.setClockDivider(SPI_CLOCK_DIV128);
    for(byte device = 0; device < myNumberOfDevices; device++) 
    {
        sendByte (device, MAX7219_REG_SCANLIMIT, 7);   // show all 8 digits
        sendByte (device, MAX7219_REG_DECODEMODE, 0);  // using an led matrix (not digits)
        sendByte (device, MAX7219_REG_DISPLAYTEST, 0); // no display test
        sendByte (device, MAX7219_REG_INTENSITY, 0);   // character intensity: range: 0 to 15
        sendByte (device, MAX7219_REG_SHUTDOWN, 1);    // not in shutdown mode (ie. start it up)
    }
}

void LedMatrix::sendByte (const byte device, const byte reg, const byte data) 
{
    for(int i=0;i<myNumberOfDevices;i++) 
        spitransfer[i] = 0;

    // put our device data into the array
    spitransfer[device] = reg << 8 | data;

    // enable the line
    digitalWrite(mySlaveSelectPin,LOW);
    // now shift out the data
    for(int i=0;i<myNumberOfDevices;i++) 
    {
        SPI.transfer16(spitransfer[i]);
    }
    digitalWrite (mySlaveSelectPin, HIGH);
}

void LedMatrix::sendAllBytes (void) 
{
    for (byte col = 0; col < 8; col++) 
    {
        // enable the line
        digitalWrite(mySlaveSelectPin,LOW);
        for(int i=0;i<myNumberOfDevices;i++) 
        {
            spitransfer[i] = (col + 1) << 8 | (cols[(i*8) + col]);
            SPI.transfer16(spitransfer[i]);
        }
        digitalWrite (mySlaveSelectPin, HIGH);
    }   
}

void LedMatrix::sendByte (const byte reg, const byte data) 
{
    for(byte device = 0; device < myNumberOfDevices; device++) 
    {
        sendByte(device, reg, data);
    }
}

void LedMatrix::setIntensity(const byte intensity) 
{
    sendByte(MAX7219_REG_INTENSITY, intensity);
}

void LedMatrix::setCharWidth(byte charWidth) 
{
    myCharWidth = charWidth;
}

void LedMatrix::setTextAlignment(byte textAlignment) 
{
    myTextAlignment = textAlignment;
    calculateTextAlignmentOffset();
}

void LedMatrix::calculateTextAlignmentOffset() 
{
    switch(myTextAlignment) 
    {
        case TEXT_ALIGN_LEFT:
            myTextAlignmentOffset = 0;
            break;
        case TEXT_ALIGN_LEFT_END:
            myTextAlignmentOffset = myNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT:
            myTextAlignmentOffset = myText.length() * myCharWidth - myNumberOfDevices * 8;
            break;
        case TEXT_ALIGN_RIGHT_END:
            myTextAlignmentOffset = - myText.length() * myCharWidth;
            break;
    }
    
}

void LedMatrix::clear() 
{
    for (byte col = 0; col < myNumberOfDevices * 8; col++) 
    {
        cols[col] = 0;
    }
}

void LedMatrix::commit() 
{
    sendAllBytes();
}

void LedMatrix::setText(String text) 
{
    myText = text;
    myTextOffset = 0;
    calculateTextAlignmentOffset();
}

void LedMatrix::setNextText(String nextText) 
{
    myNextText = nextText;
}

void LedMatrix::scrollTextRight() 
{
    myTextOffset = (myTextOffset + 1) % ((int)myText.length() * myCharWidth - 5);
}

void LedMatrix::scrollTextLeft() 
{
    myTextOffset = (myTextOffset - 1) % ((int)myText.length() * myCharWidth + myNumberOfDevices * 8);
    if (myTextOffset == 0 && myNextText.length() > 0) 
    {
        myText = myNextText;
        myNextText = "";
        calculateTextAlignmentOffset();
    }
}

void LedMatrix::oscillateText() 
{
    int maxColumns = (int)myText.length() * myCharWidth;
    int maxDisplayColumns = myNumberOfDevices * 8;
    if (maxDisplayColumns > maxColumns) 
    {
        return;
    }
    if (myTextOffset - maxDisplayColumns == -maxColumns) 
    {
        increment = 1;
    }
    if (myTextOffset == 0) 
    {
        increment = -1;
    }
    myTextOffset += increment;
}

void LedMatrix::drawText() 
{
    char letter;
    int position = 0;
    unsigned char coldata;
    for (int i = 0; i < (int)myText.length(); i++) 
    {
        letter = myText.charAt(i);
        for (byte col = 0; col < myCharWidth; col++) 
        {
            position = (i * myCharWidth) + col + myTextOffset + myTextAlignmentOffset;
            if ((position >= 0) && (position < (myNumberOfDevices * 8))) 
            {
                if (rotate)
                {
                    unsigned char colinfo[8];
                    coldata = 0;
                    byte ii;

                    //This is ugly... But the only alternative is a charset that is already pre-rotated....
                    for(ii=0;ii<myCharWidth;ii++)
                    {
                        colinfo[ii] = pgm_read_byte (&(cp437_font [(unsigned char)letter] [ii]));
                    }
                    for (ii=0;ii<8;ii++)
                    {
                        if(((colinfo[col] >> ii) & 0x01) == 0x01)
                        {
                                int column = ((position/8)*8)+ii;
                                if (column < 0 || column >= myNumberOfDevices * 8)
                                {
                                    return;
                                }
                                setPixel(column, 7-(position%8));
                        }
                    }
                }
                else
                {
                    coldata = pgm_read_byte (&(cp437_font [(unsigned char)letter] [col]));
                    setColumn(position, coldata);
                }

            }
        }
    }
}

void LedMatrix::setCustomChar(byte x,byte width,byte *data)
{
    int position = 0;
    unsigned char coldata;
    for (byte col = 0; col < width; col++) 
    {
        position = x + col;
        if ((position >= 0) && (position < (myNumberOfDevices * 8))) 
        {
            if (rotate)
            {
                unsigned char colinfo[width];
                coldata = 0;
                byte ii;

                //And again..This is ugly...
                for(ii=0;ii<width;ii++)
                {
                    colinfo[ii] = data[ii];
                }
                for (ii=0;ii<8;ii++)
                {
                    if(((colinfo[col] >> ii) & 0x01) == 0x01)
                    {
                            int column = ((position/8)*8)+ii;
                            if (column < 0 || column >= myNumberOfDevices * 8)
                            {
                                return;
                            }
                            setPixel(column, 7-(position%8));
                    }
                }
            }
            else
            {
                coldata = data[col];
                setColumn(position, coldata);
            }

        }
    }
}

void LedMatrix::setColumn(int column, byte value) 
{
    if (column < 0 || column >= myNumberOfDevices * 8) 
    {
        return;
    }
    cols[column] = value;
}

void LedMatrix::setPixel(byte x, byte y) 
{
    bitWrite(cols[x], y, true);
}

void LedMatrix::setRotate(bool rot)
{
    rotate = rot;
}