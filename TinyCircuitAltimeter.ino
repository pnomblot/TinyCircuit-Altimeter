#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <SD.h>
#include <TimeLib.h>
#include <TinyScreen.h>
TinyScreen display = TinyScreen(TinyScreenPlus);

#include "LucidaGrande.h"
#include "LucidaGrandeBold.h"


// BMP280 definitions
Adafruit_BMP280 bmp; 
float P0=1013.25;
unsigned int samples;
unsigned int sampling=1;
float average;

// SD Card 
#define SD_chipSelect 10


// Altimeter settings
#define XMAX  96
#define YMAX  40
#define OFFSET_Y  20

#define MIN_LINE_COLOR  TS_8b_DarkGreen
#define MAX_LINE_COLOR  TS_8b_Brown

// BATTERY LEVEL 
#define BAT_X_POS  82
#define BAT_Y_POS   1
#define BAT_HEIGHT  4
#define BAT_LENGTH  9

float rotatingBuffer[XMAX];
unsigned char rotatingBufferIndex=0 ;
int screen=-1;
int brightness=10;
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


void SET_P0(void);
void SET_Sampling(void);
void display_Setting_Sampling(void); 
void Brightness(void);
void display_Altitude(float altitude);               // Print Altitude 
void draw_Altitude();                                // draw altitude graph 
void display_Temperature(float temperature);         // Print temperature 
void display_Time(void);                             // Print temperature 
void store_data(float altitude, float temperature, char * filename); // Store Altitude and temperature on SD CARD
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
  display.setBrightness(brightness);

  display.setFlip(true);
  display.clearScreen();
  display.setFont(FONT_10ptsBold);   
  display.fontColor(TS_8b_Blue,TS_8b_Black);

  if(!bmp.begin()){
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(1,10);
    display.print("BMP ERROR ! ");
    display.drawRect(0,0,XMAX,YMAX,TSRectangleNoFill,TS_8b_Red);
    while(1);
  }


  delay(200);
  average = bmp.readAltitude(P0);
  // Initialize curve values
  for (int i=0; i<XMAX; i++) { 
     rotatingBuffer[i] = average;
  }

  //while (!Serial); // Wait for USB Serial to be ready 
  SerialUSB.println("Setup");

 if (!SD.begin(SD_chipSelect)) {
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(0,40);
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
  float a;
  
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

  #define STATE_SETTING_ALTITUDE 0
  #define STATE_SETTING_SAMPLING_RATE 1
  #define STATE_DISPLAY_ALTITUDE 2
  #define STATE_DISPLAY_TEMPERATURE 3
  #define STATE_DISPLAY_BATTERY 4
  #define STATE_DISPLAY_TIME 5
  #define STATE_SETTING_HOUR 6
  #define STATE_SETTING_MINUTE 7
  #define STATE_SETTING_SECOND 8
  #define STATE_SETTING_DAY 9
  #define STATE_SETTING_MONTH 10
  #define STATE_SETTING_YEAR 11


  display.clearScreen(); 

  switch (screen) {
    case -1: // Startup
//      display.drawBitmap(0, 0, Splash, 128, 64, WHITE);
//      display.display();
      display.setFont(FONT_20ptsBold);   
      display.fontColor(TS_8b_White,TS_8b_Black);
      display.setCursor(0,0);
      display.print("Patrick");
      screen = STATE_DISPLAY_ALTITUDE;
      loopDuration = 1000;
    break;

    case STATE_SETTING_SAMPLING_RATE:
      display_Setting_Sampling();
      loopDuration = 150;
      SET_Sampling();
    break;

    case STATE_SETTING_ALTITUDE:
      display_Setting_Altitude(P0, bmp.readAltitude(P0));
      loopDuration = 150;
      SET_P0();
    break;

    case STATE_DISPLAY_ALTITUDE:      
      a = bmp.readAltitude(P0);
      display_Altitude(a);
      draw_Battery(read_Battery());
      if ( ++samples>= sampling) {
         samples = 0;
         rotatingBuffer[rotatingBufferIndex] = average/sampling;
         rotatingBufferIndex = (rotatingBufferIndex + 1)%XMAX;
         store_data(average/sampling, bmp.readTemperature());
         average = a;
      } else {
              average += a;
      }
      draw_Altitude();
      loopDuration = 1000;
      Brightness();
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
void Brightness() {
    if (display.getButtons(TSButtonLowerLeft)) {
      if (brightness>0) brightness--;
      display.setBrightness(brightness);
    }  
    
    if (display.getButtons(TSButtonUpperLeft)) {
      if (brightness<16) brightness++;
      display.setBrightness(brightness);
    }  
}

//--------------------------------------------------------------------------------------------------------------------------------
void SET_P0() {
    if (display.getButtons(TSButtonLowerLeft)) {
      if (P0>0) P0-=0.01;
    }  
    
    if (display.getButtons(TSButtonUpperLeft)) {
      if (P0<2000) P0+=0.01;
    }  
}

//--------------------------------------------------------------------------------------------------------------------------------
void SET_Sampling() {
    if (display.getButtons(TSButtonLowerLeft)) {
      if (sampling>1) sampling--;
    }  
    
    if (display.getButtons(TSButtonUpperLeft)) {
      if (sampling<60*60) sampling++;
    }  
}

//--------------------------------------------------------------------------------------------------------------------------------
void display_Altitude(float altitude) {
   display.setFont(FONT_12ptsBold);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(28,0);
   display.print(String(altitude));
}
 
//--------------------------------------------------------------------------------------------------------------------------------
void draw_Altitude() {
   int Ymin=int(rotatingBuffer[0]);
   int Ymax=Ymin;
   
   for (int i=0; i<XMAX; i++) { 
     if (rotatingBuffer[i] > Ymax) Ymax=rotatingBuffer[i];
     if (rotatingBuffer[i] < Ymin) Ymin=rotatingBuffer[i];
   }
   
   unsigned int modulo = 1;
   if ((Ymax - Ymin) >YMAX ) modulo = 10;
   if ((Ymax - Ymin) >200 ) modulo = 100;
   Ymin = Ymin -(Ymin%modulo);
   Ymax = Ymax -(Ymax%modulo) + modulo;
   
   // normalize
   float coef = YMAX/float(Ymax-Ymin);

   // Print current lower Y value
   display.setFont(FONT_6pts);
   display.fontColor(TS_8b_Blue,TS_8b_Black);
   display.setCursor(0,OFFSET_Y+YMAX-8);
   display.print(Ymin);

   // Print current higher Y value (if needed)
   display.setCursor(0,OFFSET_Y-8);
   display.print(Ymax);

   display.drawLine(0,OFFSET_Y       , XMAX,OFFSET_Y       , MAX_LINE_COLOR);
   display.drawLine(0,OFFSET_Y + YMAX, XMAX,OFFSET_Y + YMAX, MIN_LINE_COLOR);

   // draw curve from buffer values
   for (int i=0; i<XMAX; i++) { 
    display.drawPixel(i, OFFSET_Y + YMAX - (coef*(rotatingBuffer[(rotatingBufferIndex+i)%XMAX]-Ymin) ), TS_8b_Blue);
   }
   display.writePixel(TS_8b_Blue);
}




//--------------------------------------------------------------------------------------------------------------------------------
void display_Setting_Altitude(float p, float altitude) {
   display.setFont(FONT_12ptsBold);  
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(0,10);
   display.print(String(p));
   display.print(" mb");

   display.fontColor(TS_8b_Blue,TS_8b_Black);
   display.setCursor(0,40);
   display.print(String(altitude));
   display.print(" m");
 
}
//--------------------------------------------------------------------------------------------------------------------------------
void display_Setting_Sampling() {
   display.setFont(FONT_12ptsBold);  
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(0,10);
   display.print(String(sampling));
   display.print(" s");
 
}
   

//--------------------------------------------------------------------------------------------------------------------------------
void display_Temperature(float temperature) {
   display.setFont(FONT_20ptsBold);  
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(0,20);
   display.print(String(temperature)+" °c");
}




//--------------------------------------------------------------------------------------------------------------------------------
void display_Time() {
    char buffer[3];

    display.setFont(FONT_12ptsBold);   
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
void store_data(float altitude, float temperature) {
  char buffer[40];
  char filename[13];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d %0.2f %0.2f", year(),month(),day(),hour(),minute(),second(), altitude, temperature);
  sprintf(filename, "%02d%02d%04d.csv", day(),month(),year());
  SerialUSB.println(buffer);


//  File file = SD.open(filename, FILE_WRITE);
  File file = SD.open(filename, O_CREAT | O_WRITE | O_APPEND);
  if (file) {
    file.println(buffer);
    file.flush();
    file.close();
    SerialUSB.println("Altitude stored.");
  } else {
    SerialUSB.println("ERROR FILE");
  }
}



//--------------------------------------------------------------------------------------------------------------------------------
void display_Battery(int batteryLevel) {
   display.setFont(FONT_20ptsBold);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(10,20);
   display.print(String((float(batteryLevel)/100)));
   display.print(" v");
}


//--------------------------------------------------------------------------------------------------------------------------------
void draw_Battery(int batteryLevel) {
    
  display.drawRect(BAT_X_POS+BAT_LENGTH, BAT_Y_POS+1, 2, 2, TSRectangleFilled,TS_8b_White);
  display.drawRect(BAT_X_POS-1 , BAT_Y_POS-1, BAT_LENGTH+2, BAT_HEIGHT+2, TSRectangleNoFill,TS_8b_White);
  if (batteryLevel > 326) {
    display.drawRect(BAT_X_POS , BAT_Y_POS, BAT_LENGTH, BAT_HEIGHT, TSRectangleFilled,TS_8b_Green);
  } else {
    display.drawRect(BAT_X_POS , BAT_Y_POS, BAT_LENGTH, BAT_HEIGHT, TSRectangleFilled,TS_8b_Red);
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
  return(result);
}
