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
#define SD_FileName "alti2.csv"


// Altimeter settings
#define XMAX  96
#define YMAX  64

int rotatingBuffer[XMAX];
unsigned char rotatingBufferIndex=0 ;
int screen=-1;
unsigned long loopTick;
unsigned int loopDuration = 1000;
unsigned char clignote; 

void display_Altitude(double altitude);              // Print Altitude on tinyScreen
void display_Temperature(double temperature);        // Print temperature on tinyScreen
void display_Time(void);                             // Print temperature on tinyScreen
void store_data(double altitude, double temperature, char * filename); // Store Altitude and temperature on SD CARD





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
    delay(3000);
  } else {
    SerialUSB.println("SD CARD OK !");
  }

  setTime(0, 0, 0, 16, 2, 2016);
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

    case 0: // Altitude
      display_Altitude(bmp.readAltitude(P0));
      store_data(bmp.readAltitude(P0), bmp.readTemperature(), SD_FileName);
      loopDuration = 1000;
    break;

    case 1: // Temperature
      display_Temperature(bmp.readTemperature());
      loopDuration = 1000;
    break;

    case 2: // Time
      display_Time();
      loopDuration = 1000;
    break;


    #define SETTING_HOUR 3
    #define SETTING_MINUTE 4
    #define SETTING_SECOND 5
    case 3: // Time
    case 4: // Time
    case 5: // Time
      display_Time();
      loopDuration = 500;
    break;

    default:
       screen = 0;
    break;
  }

}







//--------------------------------------------------------------------------------------------------------------------------------
void display_Altitude(double altitude) {
   display.clearScreen();
   display.setFont(liberationSans_12ptFontInfo);   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(44,0);
   display.print(String(altitude));
   display.setFont(thinPixel7_10ptFontInfo);
   display.setCursor(90,6);
   display.print("m");
 
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
   display.setFont(  liberationSans_12ptFontInfo  );   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(44,0);
   display.print(String(temperature));
   display.setFont(thinPixel7_10ptFontInfo);
   display.setCursor(90,6);
   display.print("c");
   display.drawRect(88,6,2,2,TSRectangleNoFill,TS_8b_White);
}



//--------------------------------------------------------------------------------------------------------------------------------
void display_Time() {
    char buffer[3];

    display.clearScreen();
    display.setFont(  liberationSans_12ptFontInfo  );   
    display.fontColor(TS_8b_White,TS_8b_Black);
    display.setCursor(0,0);

    if ((clignote%2) && (screen == SETTING_HOUR))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", hour());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print(':');

    if ((clignote%2) && (screen == SETTING_MINUTE))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", minute());
    display.print(buffer);

    display.fontColor(TS_8b_White,TS_8b_Black);
    display.print(':');

    if ((clignote%2) && (screen == SETTING_SECOND))  display.fontColor(TS_8b_Black, TS_8b_White);
    sprintf(buffer, "%02d", second());
    display.print(buffer);
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


