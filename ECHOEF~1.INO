#include <AudioHacker.h>

//Adafruit TFT Display Libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
//

//TFmini-s Lidar Libraries
#include <SoftwareSerial.h>
#include "TFMini.h"
TFMini tfmini;
//The only value that matters here is the first one, 0, Rx (Where (RX, TX) --> (WHITE, GREEN) )
SoftwareSerial SerialTFMini(0, 1);      

/* // Seems that AudioHacker and TFT Display can't be used at the same time, try contacting Nootropic Design
#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5

#else
  // For the breakout board, you can use any 2 or 3 pins.
  // These pins will also work for the 1.8" TFT shield.
  #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
#endif
*/

// For 1.44" and 1.8" TFT with ST7735 use:
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define DEBUG

unsigned int playbackBuf;
unsigned int sampleRate;
unsigned int readBuf[2];
unsigned int writeBuf;
boolean evenCycle = true;
unsigned int timer1Start;
volatile unsigned int timer1EndEven;
volatile unsigned int timer1EndOdd;
unsigned long lastDebugPrint = 0;
volatile long address = 0;
unsigned int echoDelay;
boolean echoWrapped = false;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);        // connect to the serial port
#endif
while (!Serial);            // wait for serial port to connect. Needed for native USB port only

//From TFmini-s_Reading_to_TFT_Display
uint16_t time = millis();
  
  time = millis() - time;

  Serial.println(F("Initialized TFT"));

  Serial.println(time, DEC);
  delay(500);

//Serial.println ("Initializing TFmini-s...");
  SerialTFMini.begin(TFMINI_BAUDRATE);    //Initialize the data rate for the SoftwareSerial port
  tfmini.begin(&SerialTFMini);            //Initialize the TF Mini sensor
//


  playbackBuf = 2048;
  sampleRate = DEFAULT_RECORDING_SAMPLE_RATE;
  timer1Start = UINT16_MAX - (F_CPU / sampleRate);

  AudioHacker.begin();

#ifdef DEBUG
  Serial.print("sample rate = ");
  Serial.print(sampleRate);
  Serial.print(" Hz");
  Serial.println();
#endif

/* // For TFT display
// Serial.print(F("Initializing ST77xx TFT..."));
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab

  uint16_t time = millis();
  tft.fillScreen(ST77XX_BLACK);
  time = millis() - time;

  Serial.println(F("Initialized TFT"));

  Serial.println(time, DEC);
  delay(500);
*/
}

void loop() {

  int distance = 0;
  int strength = 0; 
  
getTFminiData(&distance, &strength);
    while (!distance){
      getTFminiData(&distance, &strength);
      if (distance){ 
      // echoDelay is number of memory slots back into the past to read for the echo.
      // must be a factor of 3 because we pack 2 samples into each 3-byte sequence of SRAM.

    }

  } //end while(!distance)

delay(300);

//ORIGNAL AUDIOHACKER EchoEffect
// echoDelay is number of memory slots back into the past to read for the echo.
// must be a factor of 3 because we pack 2 samples into each 3-byte sequence of SRAM.
// echoDelay = analogRead(0) * 30; 
//^^^analogRead(0) was changed to distance

// According to article  Delay = D_daf - 2x/v
// D_daf is set to 0.2 sec which is 200ms
// x[meters] = distance between speaker(target) and the device
// v = 331.5 + 0.61(t)    however 331.5 is in meters/s, so for meters / ms would multiply 100, hence seen below 33150
// t = air temperature in Celcius. (around 20*C for room temperature)
// orignal --> 
// Therefore: echoDelay = (0.2 - (   (2*distance) /(331.5 + (0.61*20) ) )   ); 

echoDelay =  -45.55947 * 0.2 - (   (2*(distance) /(331.5 + (0.61*20)) )   )  * 30; // NEW echoDelay
//Explanation
// Known value for analogRead is 0->256, default is *30 = 7680 (MAX)
// EchoEffect program has failsafe values set to 2048 if echoDelay < 0
// within default program, echoDelay = analogDelay(0) * 30 ; maintain * 30 for sampling 3-byte sequence SRAM
// Therefore, solve for a X  value where X * 0.2 - (   (2*(distance) /(331.5 + (0.61*20)) )   )  * 30 = 7680
// X = -45.55947

#ifdef DEBUG
  if ((millis() - lastDebugPrint) >= 1000) {
    lastDebugPrint = millis();

/* // for TFT display
    tft.setRotation(1);
    tft.setTextColor(ST7735_GREEN, ST7735_BLACK); // before the tft.print.
    tft.setTextSize(2);
    tft.setCursor(0,0);
*/

    //each 3 echoDelay2 is 2 samples.  
    unsigned int delayMillis = (float)(((float)((echoDelay * 2) / 3)) / (float)(sampleRate/1000.0));

/* Failed reading of Serial data from AudioHacker to TFTdisplay
    tft.print("echo delay = ");
    tft.println(delayMillis);
    tft.println(" ms    even cycles remaining = ");
    tft.println(UINT16_MAX - timer1EndEven);
    tft.println("   odd cycles remaining = ");
    tft.println(UINT16_MAX - timer1EndOdd);
    tft.println();
*/

//Original AudioHacker Serial.print
    Serial.print("echo delay = ");
    Serial.print(delayMillis);
    Serial.print(" ms    even cycles remaining = ");
    Serial.print(UINT16_MAX - timer1EndEven);
    Serial.print("   odd cycles remaining = ");
    Serial.print(UINT16_MAX - timer1EndOdd);
    Serial.println();


  }
}
#endif

ISR(TIMER1_OVF_vect) {
  TCNT1 = timer1Start;
  unsigned int signal;
  unsigned int echo;
  int mix;

  AudioHacker.writeDAC(playbackBuf);

  // Read ADC
  signal = AudioHacker.readADC();


  if (evenCycle) {
    long echoAddress = address - echoDelay;
    if (echoAddress < 0) {
      echoAddress += MAX_ADDR;
    }
    AudioHacker.readSRAMPacked(0, echoAddress, readBuf);
    if ((!echoWrapped) && (echoAddress > address)) {
      // avoid reading from unwritten memory
      echo = 2048;
      readBuf[1] = 2048;
    } else {
      echo = readBuf[0];
    }
  } else {
    echo = readBuf[1];
  }
  if (echoDelay == 0) {
    echo = 2048;
  }

  if (evenCycle) {
    writeBuf = signal;
  } else {
    AudioHacker.writeSRAMPacked(0, address, writeBuf, signal);
    address += 3;
    if (address > MAX_ADDR) {
      address = 0;
      echoWrapped = true;
    }
  }


  mix = signal-2048;
  echo = echo >> 1; // attenuate echo
  mix += (echo - 1024); // since we are dividing echo by 2, decrement by 1024
  if (mix < -2048) {
    mix = -2048;
  } else {
    if (mix > 2047) {
      mix = 2047;
    }
  }
  playbackBuf = mix + 2048;


#ifdef DEBUG
  if (evenCycle) {
    timer1EndEven = TCNT1;
  } else {
    timer1EndOdd = TCNT1;
  }
#endif
  evenCycle = !evenCycle;
}



void getTFminiData(int* distance, int* strength)
{
  static char i = 0;
  char j = 0;
  int checksum = 0;
  static int rx[9];
  if (SerialTFMini.available())
  {
    rx[i] = SerialTFMini.read();
    if (rx[0] != 0x59)
    {
      i = 0;
    }
    else if (i == 1 && rx[1] != 0x59)
    {
      i = 0;
    }
    else if (i == 8)
    {
      for (j = 0; j < 8; j++)
      {
        checksum += rx[j];
      }
      if (rx[8] == (checksum % 256))
      {
        *distance = rx[2] + rx[3] * 256;
        *strength = rx[4] + rx[5] * 256;
      }
      i = 0;
    }
    else
    {
      i++;
    }
  }
}
