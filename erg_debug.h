unsigned int erg_sim[] = {
1673	,
1882	,
2003	,
999999
};

int erg_sim_ptr = 0;


// #define ILI9488_DRIVER     // WARNING: Do not connect ILI9488 display SDO to MISO if other devices share the SPI bus (TFT SDO does NOT tristate when CS is high)

// // ##################################################################################
// //
// // Section 2. Define the pins that are used to interface with the display here
// //
// // ##################################################################################

// // If a backlight control signal is available then define the TFT_BL pin in Section 2
// // below. The backlight will be turned ON when tft.begin() is called, but the library
// // needs to know if the LEDs are ON with the pin HIGH or LOW. If the LEDs are to be
// // driven with a PWM signal or turned OFF/ON then this must be handled by the user
// // sketch. e.g. with digitalWrite(TFT_BL, LOW);

//  #define TFT_BL   32            // LED back-light control pin
//  #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)

// //## EDIT THE PIN NUMBERS IN THE LINES FOLLOWING TO SUIT YOUR ESP32 SETUP   ######

// // For ESP32 Dev board (only tested with ILI9341 display)
// // The hardware SPI can be mapped to any pins

// #define TFT_MISO 19
// #define TFT_MOSI 23
// #define TFT_SCLK 18
// #define TFT_CS   15  // Chip select control pin
// #define TFT_DC    2  // Data Command control pin
// #define TFT_RST   4  // Reset pin (could connect to RST pin)
// //#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

// #define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen

// // ##################################################################################
// //
// // Section 3. Define the fonts that are to be used here
// //
// // ##################################################################################

// // Comment out the #defines below with // to stop that font being loaded
// // The ESP8366 and ESP32 have plenty of memory so commenting out fonts is not
// // normally necessary. If all fonts are loaded the extra FLASH space required is
// // about 17Kbytes. To save FLASH space only enable the fonts you need!

// #define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
// //#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
// #define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// // Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// // this will save ~20kbytes of FLASH
// #define SMOOTH_FONT

// #define SPI_FREQUENCY 40000000
