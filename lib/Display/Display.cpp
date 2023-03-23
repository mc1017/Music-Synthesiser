#include <Arduino.h>
#include <U8g2lib.h>
#include <Keyboard.h>
#include <ES_CAN.h>

// Define boarders
const int X_START = 2;
const int Y_START = 0;
const int X_END = 127;
const int Y_END = 31;
const int FONT_HEIGHT = 8;
const int FONT_WIDTH = 6;

void displayHandshake()
{
    u8g2.clearBuffer(); // clear the internal memory

    u8g2.setFont(u8g2_font_open_iconic_www_4x_t);

    u8g2.drawGlyph((X_END - X_START) / 2 - 16, 32, 79);

    u8g2.sendBuffer();
}

void displayOctave(uint8_t octave, bool drumEnable)
{

    u8g2.drawButtonUTF8(39, 19, U8G2_BTN_INV, 0, 0, 0, "Oct");

    if (drumEnable)
    {
        u8g2.setCursor(37, 31);
        u8g2.print("Drum");
    }
    else
    {
        u8g2.setCursor(45, 31);
        u8g2.print(octave + 3);
    }
}

void displayWaveform(uint8_t waveform)
{

    u8g2.drawButtonUTF8(4, 19, U8G2_BTN_INV, 0, 0, 0, "Wav");
    u8g2.setCursor(4, 31);

    switch (waveform)
    {
    case 0:
        u8g2.print("SAW");
        break;
    case 1:
        u8g2.print("TRI");
        break;
    case 2:
        u8g2.print("SQU");
        break;
    case 3:
        u8g2.print("SIN");
        break;
    }
}

void displayMode(uint8_t mode)
{

    u8g2.drawButtonUTF8(74, 19, U8G2_BTN_INV, 0, 0, 0, "Mod");

    u8g2.setCursor(74, 31);

    switch (mode)
    {
    case 0:
        u8g2.print("NOR");
        break;
    case 1:
        u8g2.setCursor(66, 31);
        u8g2.print("DRUM");
        break;
    case 2:
        u8g2.print("LFO");
        break;
    }
}

void displayVolume(uint8_t volume)
{

    u8g2.drawButtonUTF8(109, 19, U8G2_BTN_INV, 0, 0, 0, "Vol");

    u8g2.setCursor(115, 31);
    u8g2.print(volume);
}

void displayTXRX(bool transmitter, bool multipleModule, uint8_t position)
{
    u8g2.setCursor(120, 8);
    u8g2.print(position);

    if (!multipleModule)
    {
        u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
        u8g2.drawGlyph(110, 8, 69);
        u8g2.setFont(u8g2_font_t0_11_tf);
        return;
    }

    u8g2.setFont(u8g2_font_open_iconic_gui_1x_t);

    if (transmitter)
    {
        u8g2.drawGlyph(110, 8, 65);
    }
    else
    {
        u8g2.drawGlyph(110, 8, 64);
    }

    u8g2.setFont(u8g2_font_t0_11_tf);
}

void displayKeys(uint8_t keyArray0, uint8_t keyArray1, uint8_t keyArray2)
{
    uint8_t activatedKeys = 0;

    uint16_t keys = ((uint16_t)keyArray2 & 0x0F) << 8 |
                    ((uint16_t)keyArray1 & 0x0F) << 4 |
                    ((uint16_t)keyArray0 & 0x0F);

    for (uint8_t i = 0; i < 12; i++)
    {
        if (activatedKeys > 7)
            //  Allow maximum 7 keys to display at once
            return;
        // Get single bit at position i
        uint32_t bit = (keys & (1 << i)) << i;

        if (!bit)
        {
            u8g2.setCursor(20 + activatedKeys * 12, 8);

            switch (i + 1)
            {
            case 1:
                u8g2.print("C");
                break;
            case 2:
                u8g2.print("C");
                u8g2.drawGlyph(25 + activatedKeys * 12, 8, 35);
                break;
            case 3:
                u8g2.print("D");
                break;
            case 4:
                u8g2.print("D");
                u8g2.drawGlyph(25 + activatedKeys * 12, 8, 35);
                break;
            case 5:
                u8g2.print("E");
                break;
            case 6:
                u8g2.print("F");
                break;
            case 7:
                u8g2.print("F");
                u8g2.drawGlyph(25 + activatedKeys * 12, 8, 35);
                break;
            case 8:
                u8g2.print("G");
                break;
            case 9:
                u8g2.print("G");
                u8g2.drawGlyph(25 + activatedKeys * 12, 8, 35);
                break;
            case 10:
                u8g2.print("A");
                break;
            case 11:
                u8g2.print("A");
                u8g2.drawGlyph(25 + activatedKeys * 12, 8, 35);
                break;
            case 12:
                u8g2.print("B");
                break;
            default:
                u8g2.print("-");
                break;
            }

            activatedKeys++;
        }
    }
}