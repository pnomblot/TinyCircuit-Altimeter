#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SD.h>
#include <TimeLib.h>

#include <TinyScreen.h>
TinyScreen display = TinyScreen(TinyScreenPlus);

// BMP280 definitions
Adafruit_BMP280 bmp; 
#define P0 1013.25

// SD Card 
#define SD_chipSelect 10
#define SD_FileName "alti.csv"


// Altimeter settings
#define XMAX  96
#define YMAX  64

int rotatingBuffer[XMAX];
unsigned char rotatingBufferIndex=0 ;
int screen=-1;
unsigned long loopTick;
unsigned int loopDuration = 1000;
unsigned char clignote; 
#define time_setting_tick 150
#define MAX_SECOND  60
#define MAX_MINUTE 60
#define MAX_HOUR  24
#define MAX_DAY  31
#define MAX_MONTH 12
#define MAX_YEAR  2100
  

void display_Altitude(double altitude);              // Print Altitude on tinyScreen
void display_Temperature(double temperature);        // Print temperature on tinyScreen
void display_Time(void);                             // Print temperature on tinyScreen
void store_data(double altitude, double temperature, char * filename); // Store Altitude and temperature on SD CARD
void display_Battery(int batteryLevel);
void draw_Battery(int batteryLevel);
int read_Battery(void);
unsigned updown(unsigned int val, unsigned int max);



//--------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  SerialUSB.begin(9600);
//   while (!SerialUSB); // wait for serial port to connect.
  setTime(07,42,00,9,3,2016);    //values in the order hr,min,sec,day,month,year

  Wire.begin();
  display.begin();
  display.setBrightness(10);

  display.setFlip(true);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_Blue,TS_8b_Black);

  if(!bmp.begin()){
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(1,10);
    display.print("BMP ERROR ! ");
    display.drawRect(0,0,XMAX,YMAX,TSRectangleNoFill,TS_8b_Red);
    while(1);
  }

  // Initialize curve values
  int A = (int)bmp.readAltitude(P0);
  for (int i=0; i<XMAX; i++) { 
     rotatingBuffer[i] = A;
  }

  //while (!Serial); // Wait for USB Serial to be ready 
  SerialUSB.println("Setup");

 if (!SD.begin(SD_chipSelect)) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(1,40);
    display.print("NO SD CARD ! ");
  } else {
    SerialUSB.println("SD CARD OK !");
  }

//  setTime(0, 0, 0, 16, 2, 2016);
  SerialUSB.println("Starting");
  loopTick =  millis();

}


//--------------------------------------------------------------------------------------------------------------------------------
void loop()
{
  
  while ( ( millis() - loopTick ) < loopDuration ) {
    delay(1);
    if (display.getButtons(TSButtonLowerRight)) {
      screen--;
      while(display.getButtons(TSButtonLowerRight));
      break;
    }  
    if (display.getButtons(TSButtonUpperRight)) {
      screen++;
      while(display.getButtons(TSButtonUpperRight));
      break;
    }  
  }


  loopTick =  millis();
  ++clignote;

    #define STATE_DISPLAY_ALTITUDE 0
    #define STATE_DISPLAY_TEMPERATURE 1
    #define STATE_DISPLAY_BATTERY 2
    #define STATE_DISPLAY_TIME 3
    #define STATE_SETTING_HOUR 4
    #define STATE_SETTING_MINUTE 5
    #define STATE_SETTING_SECOND 6
    #define STATE_SETTING_DAY 7
    #define STATE_SETTING_MONTH 8
    #define STATE_SETTING_YEAR 9



  switch (screen) {
    case -1: // Startup
      display.clearScreen(); 
//      display.drawBitmap(0, 0, Splash, 128, 64, WHITE);
//      display.display();
    display.setFont(  liberationSans_12ptFontInfo  );   
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setCursor(0,0);
    display.print("Patrick");

     ++screen;
     loopDuration = 1000;
  
    break;

    case STATE_DISPLAY_ALTITUDE:
      display_Altitude(bmp.readAltitude(P0));
      store_data(bmp.readAltitude(P0), bmp.readTemperature(), SD_FileName);
      draw_Battery(read_Battery());
      loopDuration = 1000;
    break;

    case STATE_DISPLAY_TEMPERATURE:
      display_Temperature(bmp.readTemperature());
      draw_Battery(read_Battery());
      loopDuration = 1000;
    break;

    case STATE_DISPLAY_BATTERY:
      display_Battery(read_Battery()); 
      draw_Battery(read_Battery());
      loopDuration = 1000;
    break;

    case STATE_DISPLAY_TIME:
      display_Time();
      draw_Battery(read_Battery());
      loopDuration = 1000;
    break;


    case STATE_SETTING_HOUR:
      setTime(updown(hour(), MAX_HOUR), minute(), second(), day(), month(), year());
      display_Time();
      loopDuration = time_setting_tick;
    break;

    case STATE_SETTING_MINUTE:
      setTime(hour(), updown(minute(), MAX_MINUTE), second(), day(), month(), year());
      display_Time();
      loopDuration = time_setting_tick;
    break;

    case STATE_SETTING_SECOND:
      setTime(hour(), minute(), updown(second(), MAX_SECOND), day(), month(), year());
      display_Time();
      loopDuration = time_setting_tick;
    break;

    case STATE_SETTING_DAY:
      setTime(hour(), minute(), second(), updown(day(), MAX_DAY), month(), year());
      display_Time();
      loopDuration = time_setting_tick;
    break;

    case STATE_SETTING_MONTH:
      setTime(hour(), minute(), second(), day(), updown(month(), MAX_MONTH), year());
      display_Time();
      loopDuration = time_setting_tick;
    break;


    case STATE_SETTING_YEAR:
      setTime(hour(), minute(), second(), day(), month(), updown(year(), MAX_YEAR));
      display_Time();
      loopDuration = time_setting_tick;
    break;

    default:
       screen = 0;
    break;
  }

}







//--------------------------------------------------------------------------------------------------------------------------------
void display_Altitude(double altitude) {
   display.clearScreen();
   display.setFont(liberationSans_10ptFontInfo);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(28,0);
   display.print(String(altitude));
 
   // store Altitude value in rotating buffer
   rotatingBuffer[rotatingBufferIndex] = int(altitude);
   rotatingBufferIndex = (rotatingBufferIndex + 1)%XMAX;
   int Ymin=int(altitude);
   int Ymax=int(altitude);
   for (int i=0; i<XMAX; i++) { 
     if (rotatingBuffer[i] > Ymax) Ymax=rotatingBuffer[i];
     if (rotatingBuffer[i] < Ymin) Ymin=rotatingBuffer[i];
   }

   Ymin = Ymin -(Ymin%10);
   Ymax = Ymax -(Ymax%10) +10;
   
   // normalize
   float coef = YMAX/(Ymax-Ymin);
   if ( coef>1) coef=1;

   // Print current lower Y value
   display.setFont(thinPixel7_10ptFontInfo);
   display.fontColor(TS_8b_Blue,TS_8b_Black);
   display.setCursor(0,52);
   display.print(Ymin);

   // Print current higher Y value (if needed)
   display.setCursor(0,10);
   display.print(Ymax);
   
   // draw curve from buffer values
   for (int i=0; i<XMAX; i++) { 
    display.drawPixel(i, (YMAX)-( coef*(rotatingBuffer[(rotatingBufferIndex+i)%XMAX]-Ymin) ), TS_8b_Blue);
   }
   display.writePixel(TS_8b_Blue);
}




//--------------------------------------------------------------------------------------------------------------------------------
void display_Temperature(double temperature) {
   display.clearScreen();
   display.setFont(liberationSansNarrow_12ptFontInfo);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(10,20);
   display.print(String(temperature));
   display.print(" c");
   display.drawRect(40,20,2,2,TSRectangleNoFill,TS_8b_White);
}




//--------------------------------------------------------------------------------------------------------------------------------
void display_Time() {
    char buffer[3];

    display.clearScreen();
    display.setFont(  liberationSans_12ptFontInfo  );   
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setCursor(0,30);


    if ((clignote%2) && (screen == STATE_SETTING_DAY))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", day());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print("/");

    if ((clignote%2) && (screen == STATE_SETTING_MONTH))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", month());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print("/");

    if ((clignote%2) && (screen == STATE_SETTING_YEAR))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", year());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setCursor(0,0);

    if ((clignote%2) && (screen == STATE_SETTING_HOUR))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", hour());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print(':');

    if ((clignote%2) && (screen == STATE_SETTING_MINUTE))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", minute());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print(':');

    if ((clignote%2) && (screen == STATE_SETTING_SECOND))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", second());
    display.print(buffer);
    display.fontColor(TS_8b_White,TS_8b_Black);
}


//--------------------------------------------------------------------------------------------------------------------------------
unsigned int updown(unsigned int val, unsigned int max) {
  if(display.getButtons(TSButtonUpperLeft)) val = (val+1)%max;
  if(display.getButtons(TSButtonLowerLeft)) val--;
  if (val<0) val = max-1;
  return (val);
}


//--------------------------------------------------------------------------------------------------------------------------------
void store_data(double altitude, double temperature, char * filename) {
  char buffer[35];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d %0.2f %0.2f", year(),month(),day(),hour(),minute(),second(), altitude, temperature);
  SerialUSB.println(buffer);

  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.println(buffer);
    file.close();
    SerialUSB.println("Altitude stored.");
  } else {
    SerialUSB.println("ERROR FILE");
  }
}



//--------------------------------------------------------------------------------------------------------------------------------
void display_Battery(int batteryLevel) {
   display.clearScreen();
   display.setFont(liberationSansNarrow_12ptFontInfo);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(10,20);
   display.print(String((float(batteryLevel)/100)));
   display.print(" V");
}


//--------------------------------------------------------------------------------------------------------------------------------
void draw_Battery(int batteryLevel) {
  uint8_t x = 70;
  uint8_t y = 3;
  uint8_t height = 5;
  uint8_t length = 20;
  uint8_t red, green;
  if (batteryLevel > 325) {
    red = 0;
    green = 63;
  } else {
    red = 63;
    green = 0;
  }
  display.drawLine(x - 1, y, x - 1, y + height, 0xFF); //left boarder
  display.drawLine(x - 1, y - 1, x + length, y - 1, 0xFF); //top border
  display.drawLine(x - 1, y + height + 1, x + length, y + height + 1, 0xFF); //bottom border
  display.drawLine(x + length, y - 1, x + length, y + height + 1, 0xFF); //right border
  display.drawLine(x + length + 1, y + 2, x + length + 1, y + height - 2, 0xFF); //right border
  for (uint8_t i = 0; i < length; i++) {
    display.drawLine(x + i, y, x + i, y + height, red, green, 0);
  } 
}


//--------------------------------------------------------------------------------------------------------------------------------
int read_Battery(void) {
  int result = 0;

  SYSCTRL->VREF.reg |= SYSCTRL_VREF_BGOUTEN;while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SAMPCTRL.bit.SAMPLEN = 0x1;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->INPUTCTRL.bit.MUXPOS = 0x19;         // Internal bandgap input
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC
  // Start conversion
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Clear the Data Ready flag
  ADC->INTFLAG.bit.RESRDY = 1;
  // Start conversion again, since The first conversion after the reference is changed must not be used.
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->SWTRIG.bit.START = 1;
  // Store the value
  while ( ADC->INTFLAG.bit.RESRDY == 0 );   // Waiting for conversion to complete
  uint32_t valueRead = ADC->RESULT.reg;
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  while (ADC->STATUS.bit.SYNCBUSY == 1);
  SYSCTRL->VREF.reg &= ~SYSCTRL_VREF_BGOUTEN;
  result = (((1100L * 1024L) / valueRead) + 5L) / 10L;
  SerialUSB.println(result);
  return(result);
}
