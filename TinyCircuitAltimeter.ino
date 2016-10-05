#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

#include <TinyScreen.h>
TinyScreen display = TinyScreen(TinyScreenPlus);

Adafruit_BMP280 bmp; 
#define P0 1013.25

#define XMAX  96
#define YMAX  64

int rotatingBuffer[XMAX];
unsigned char rotatingBufferIndex=0 ;


void setup()
{
  display.begin();
  display.setFlip(true);
  //setBrightness(brightness);//sets main current level, valid levels are 0-15
  display.setBrightness(10);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  display.fontColor(TS_8b_Blue,TS_8b_Black);


  if(!bmp.begin()){
    display.fontColor(TS_8b_Red,TS_8b_Black);
    display.setCursor(1,10);
    display.print("BMP ERROR ! ");
    while(1);
  }

  // Initialize curve values
  int A = (int)bmp.readAltitude(P0);
  for (int i=0; i<XMAX; i++) { 
     rotatingBuffer[i] = A;
  }
}


void loop()
{
   double A = bmp.readAltitude(P0);

   // Print Altitude
   display.clearScreen();
   display.setFont(  liberationSans_12ptFontInfo  );   
   display.fontColor(TS_8b_White,TS_8b_Black);
   display.setCursor(44,0);
   display.print(String(A));

   display.setFont(thinPixel7_10ptFontInfo);
   display.setCursor(90,6);
   display.print("m");

   // Print Temp
   display.fontColor(TS_8b_Yellow,TS_8b_Black);
   display.setCursor(60,16);
   display.print(String(bmp.readTemperature()));
   display.setCursor(92,16);
   display.print("c");
   display.drawRect(90,17,2,2,TSRectangleNoFill,TS_8b_Yellow);

 
   // store Altitude
   rotatingBuffer[rotatingBufferIndex] = int(A);
   rotatingBufferIndex = (rotatingBufferIndex + 1)%XMAX;
   int Ymin=int(A);
   int Ymax=int(A);
   for (int i=0; i<XMAX; i++) { 
     if (rotatingBuffer[i] > Ymax) Ymax=rotatingBuffer[i];
     if (rotatingBuffer[i] < Ymin) Ymin=rotatingBuffer[i];
    }

   // normalize
   float coef = YMAX/(Ymax-Ymin);
   SerialUSB.print("coef = ");SerialUSB.println(coef);
   SerialUSB.print("Ymax = ");SerialUSB.println(Ymax);
   SerialUSB.print("Ymin = ");SerialUSB.println(Ymin);
   if ( coef>1) coef=1;

   // Print current lower Y value
   display.setFont(thinPixel7_10ptFontInfo);
   display.fontColor(TS_8b_Green,TS_8b_Black);

   display.setCursor(0,54);
   display.print(Ymin);

   // Print current higher Y value (if needed)
   display.setCursor(0,10);
   if ( coef<1) display.print(Ymax);
   
   // draw curve
   for (int i=0; i<XMAX; i++) { 
     display.drawPixel(i, (50)-( coef*(rotatingBuffer[(rotatingBufferIndex+i)%XMAX]-Ymin) ), TS_8b_Blue);
    }
   display.writePixel(TS_8b_Blue);

   // wait a liitle now :-)
   delay(1000);
}
