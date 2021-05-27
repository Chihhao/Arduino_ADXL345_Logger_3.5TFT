
//#define ENABLE_SD
//#define ENABLE_TOUCH
#define ENABLE_MXIC_FLASH
//#define ENABLE_SERIAL

#include <Adafruit_ADXL345_U.h>
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); //assign ID to our accelerometer

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;       // hard-wired for UNO shields anyway.
uint8_t Orientation = 3;    //PORTRAIT

#ifdef ENABLE_MXIC_FLASH
  #include <SPIMemory.h>
  #define CAPA 134217728   //Flash容量128M
  #define TIME_INTERVAL 200  //0.2秒
  const int ARRSZ=32;  
  SPIFlash flash;  
  uint32_t gAdr=0;
  unsigned long lastWriteTime = 0;  
#endif

#ifdef ENABLE_TOUCH
  #include <TouchScreen.h>
  const int TS_LEFT=900,TS_RT=175,TS_TOP=950,TS_BOT=160;
  char *name = "Please Calibrate.";  //edit name of shield  
  const int XP=6,XM=A2,YP=A1,YM=7; //ID=0x9341
  TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
  TSPoint tp;
  #define MINPRESSURE 100
  #define MAXPRESSURE 1000
#endif

#ifdef ENABLE_SD
  #include <SPI.h>
  #include <SD.h>
  const int chipSelect = 10;
#endif

#define TFTLCD_BLACK   0x0000
#define TFTLCD_BLUE    0x001F
#define TFTLCD_RED     0xF800
#define TFTLCD_GREEN   0x07E0
#define TFTLCD_CYAN    0x07FF
#define TFTLCD_MAGENTA 0xF81F
#define TFTLCD_YELLOW  0xFFE0
#define TFTLCD_WHITE   0xFFFF

int x_pos; //position along the graph x axis
float y_pos_x; //current graph y axis position of X value
float y_pos_x_old = 160; //old y axis position of X value
float y_pos_y; //current graph y axis position of Y value
float y_pos_y_old = 160; //old y axis position of Y value
float y_pos_z; //current graph y axis position of Z value
float y_pos_z_old = 160; //old y axis position of Z value
byte x_scale = 3; //scale of graph x axis, controlled by touchscreen buttons
byte y_scale = 3;

void setup(void){  
   
#ifdef ENABLE_SERIAL
    Serial.begin(115200);
    Serial.println("start");
#endif
 
    tft.reset();
    tft.begin(tft.readID());
    tft.setRotation(Orientation);
    tft.fillScreen(TFTLCD_BLACK);
    tft.invertDisplay(0);

    if(!accel.begin()){ //begin accel, check if it's working
      tft.setCursor(0, 0); tft.setTextColor(TFTLCD_WHITE);
      tft.print("No ADXL345 detected");
      while(1);
    }
    
#ifdef ENABLE_MXIC_FLASH
    Serial.println("enable Mxic Nor Flash");
    flash.begin(MB(128));
    gAdr = findIdxOfFlash();
#endif 

#ifdef ENABLE_SD
    Serial.print("Initializing SD card...");  
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      while (1);
    }
    Serial.println("card initialized.");
#endif 
    
    //accel.setRange(ADXL345_RANGE_16_G);  
    accel.setRange(ADXL345_RANGE_8_G);  
    //accel.setRange(ADXL345_RANGE_2_G); //set resolution of accelerometer
 
    tftDrawGraphObjects(); //draw graph objects
    tftDrawColorKey();
    tftDrawXScaleButtons();
    tftDrawYScaleButtons();
}

void loop(){
  for (x_pos = (11 + x_scale); x_pos <= tft.width(); x_pos += x_scale){    
    uint16_t tsXpos, tsYpos;
        
#ifdef ENABLE_TOUCH
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    TSPoint p = ts.getPoint(); //get touch point!
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE){
      tsXpos = map(p.y, TS_BOT, TS_TOP, 0, tft.width());
      tsYpos = map(p.x, TS_LEFT, TS_RT, 0, tft.height());
      
      //!!!!!!!! X SCALE BUTTONS !!!!!!!!
      if ( (tsXpos >= 360 && tsXpos <= 380) && (tsYpos >= 0 && tsYpos <= 20) ){ //if x "-" button pressed!
         if (x_scale > 1){      
          x_scale --; //decrement x scale
          delay(70);
          break; //restart 'for' loop
         }
      }
      if ( (tsXpos >= 389 && tsXpos <= 409) && (tsYpos >= 0 && tsYpos <= 20) ){ //if x "+" button pressed!
        if (x_scale < 6){
          x_scale ++; //increment x scale
          delay(70);          
          break; //restart 'for' loop
        }
      }
      //!!!!!!!! Y SCALE BUTTONS !!!!!!!!
      if ( (tsXpos >= 426 && tsXpos <= 446) && (tsYpos >= 0 && tsYpos <= 20) ){ //if y "-" button pressed!
        if (y_scale > 1){
          y_scale --; //decrement x scale
          delay(70);
          break; //restart 'for' loop
        }
      }
      if ( (tsXpos >= 455 && tsXpos <= 475) && (tsYpos >= 0 && tsYpos <= 20) ){ //if y "+" button pressed!
        if (y_scale < 6){
          y_scale ++; //increment x scale
          delay(70);
          break; //restart 'for' loop
        }
      } 
    }
#endif

    //get accel data
    sensors_event_t event;
    accel.getEvent(&event);
    //store accel data, convert
    y_pos_x = ((-event.acceleration.x * (y_scale * 3)) + 160); //values to use when displaying on LCD, these are floats, for more precision!
    y_pos_y = ((-event.acceleration.y * (y_scale * 3)) + 160); // 160 is axis, so value is displacement from axis, scaled for better visisility!
    y_pos_z = ((-event.acceleration.z * (y_scale * 3)) + 160);
   
    //------------------------LCD DISPLAY CODE----------------------\\

    //display accel data to LCD
    tft.setTextColor(TFTLCD_WHITE); tft.setTextSize(1);
    tft.setCursor(380, 32); 
    tft.print("X New: "); tft.print(event.acceleration.x);
    tft.setCursor(380, 40);
    tft.print("Y New: "); tft.print(event.acceleration.y);
    tft.setCursor(380, 48);
    tft.print("Z New: "); tft.print(event.acceleration.z);
   
    tft.drawFastHLine(10, 160, 470, TFTLCD_WHITE); // x axis

#ifdef ENABLE_SD
    // Write to SD Card
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (dataFile) {
      String dataString = "X:" + String(event.acceleration.x) + 
                          ", Y:" + String(event.acceleration.y) + 
                          ", Z:" + String(event.acceleration.z);
      dataFile.println(dataString);
      dataFile.close();
    }
#endif

#ifdef ENABLE_MXIC_FLASH
    if(millis() - lastWriteTime > TIME_INTERVAL || millis() < lastWriteTime){
      lastWriteTime = millis();
      char str_FLASH[ARRSZ+1]; 
      String dataString = "X:" + String(event.acceleration.x) + 
                          ", Y:" + String(event.acceleration.y) + 
                          ", Z:" + String(event.acceleration.z) + "\n";
      dataString.toCharArray(str_FLASH, ARRSZ);
      writeToFlash(str_FLASH);
    }
#endif
    
    //CODE FOR PLOTTING CONTINUOUS LINES!!!!!!!!!!!!
    if(x_pos==11){
      y_pos_x_old = y_pos_x; //set old y pos values to current y pos values 
      y_pos_y_old = y_pos_y;
      y_pos_z_old = y_pos_z;
    }    
    //Plot "X" value
    tft.drawLine(x_pos - x_scale, y_pos_x_old, x_pos, y_pos_x, TFTLCD_GREEN);
    //Plot "Y" value
    tft.drawLine(x_pos - x_scale, y_pos_y_old, x_pos, y_pos_y, TFTLCD_RED);
    //Plot "Z" value
    tft.drawLine(x_pos - x_scale, y_pos_z_old, x_pos, y_pos_z, TFTLCD_BLUE);
 
    if ((x_pos >= 410) && (x_pos <= 480)){
      tft.fillRect(x_pos+1, 56, 10, 224, TFTLCD_BLACK); 
    }
    else if ((x_pos >= 350) && (x_pos < 410)){
      tft.fillRect(x_pos+1, 56, 10, 264, TFTLCD_BLACK); 
    }
    else{
      tft.fillRect(x_pos+1, 0, 10, 320, TFTLCD_BLACK); 
    }

    y_pos_x_old = y_pos_x; //set old y pos values to current y pos values 
    y_pos_y_old = y_pos_y;
    y_pos_z_old = y_pos_z;
    
    tft.fillRect(416, 32, 100, 24, TFTLCD_BLACK); //clear displayed accel data from LCD
    
    //delay(50);   
  }
  
  tft.fillRect(350, 0, 130, 56, TFTLCD_BLACK); //erase XY buttons and any lines behind them
  tft.fillRect(400, 280, 80, 40, TFTLCD_BLACK); //erase color key and any stray lines behind them
  
  tftDrawXScaleButtons();
  tftDrawYScaleButtons();
  tftDrawColorKey();
  tftDrawGraphObjects();
}

void tftDrawXScaleButtons(){
  //draw touch controls to change x axis resolution
  tft.fillRect(360, 0, 20, 20, TFTLCD_WHITE); //draw "-" box
  tft.fillRect(389, 0, 20, 20, TFTLCD_WHITE); //draw "+" box
  tft.setCursor(365, 3); tft.setTextColor(TFTLCD_BLUE); 
  tft.setTextSize(2); tft.print("-"); //print "-"
  tft.setCursor(394, 3); tft.print("+"); 
  tft.drawFastVLine(384, 0, 20, TFTLCD_WHITE);
  tft.setCursor(360, 21); tft.setTextColor(TFTLCD_WHITE); 
  tft.setTextSize(1); tft.print("X Scale:"); tft.print(x_scale);
}

void tftDrawYScaleButtons(){
  //draw touch controls to change y axis resolution
  tft.fillRect(426, 0, 20, 20, TFTLCD_WHITE); //draw "-" box
  tft.fillRect(455, 0, 20, 20, TFTLCD_WHITE); //draw "+" box
  tft.setCursor(431, 3); tft.setTextColor(TFTLCD_RED); 
  tft.setTextSize(2); tft.print("-"); //print "-"
  tft.setCursor(460, 3); tft.print("+");
  tft.drawFastVLine(450, 0, 20, TFTLCD_WHITE);  
  tft.setCursor(426, 21); tft.setTextColor(TFTLCD_WHITE); 
  tft.setTextSize(1); tft.print("Y Scale:"); tft.print(y_scale);
}

void tftDrawColorKey(){
  //Display color key
  tft.setTextSize(1); tft.setTextColor(TFTLCD_WHITE);
  tft.fillRect(420, 286, 15, 8, TFTLCD_GREEN); 
  tft.setCursor(436, 286); tft.print(" - X");
  tft.fillRect(420, 294, 15, 8, TFTLCD_RED); 
  tft.setCursor(436, 294); tft.print(" - Y");
  tft.fillRect(420, 302, 15, 8, TFTLCD_BLUE); 
  tft.setCursor(436, 302); tft.print(" - Z");  
}

void tftDrawGraphObjects(){
  //draw the graph objects
  tft.fillRect(11, 5, x_scale+1, 155, TFTLCD_BLACK);
  tft.fillRect(11, 161, x_scale+1, 159, TFTLCD_BLACK);
  tft.drawFastVLine(10, 5, 310, TFTLCD_WHITE); // y axis
  tft.drawFastHLine(10, 160, 470, TFTLCD_WHITE); // x axis
  tft.setTextColor(TFTLCD_YELLOW); tft.setTextSize(1); // set parameters for y axis labels
  tft.setCursor(3, 156); tft.print("0");  // "0" at center of ya axis
  tft.setCursor(3, 6); tft.print("+"); // "+' at top of y axis
  tft.setCursor(3, 308); tft.print("-"); // "-" at bottom of y axis

}

#ifdef ENABLE_MXIC_FLASH
uint32_t findIdxOfFlash(){
  unsigned long adr=0;
  bool gotIndex=false;
  
  for(adr=0; adr<CAPA; adr+=ARRSZ){
    if(flash.readByte(adr)==0xFF){
      gotIndex=true;
      mySerialPrint(F("find index of flash: "));
      myPrintHex(adr); mySerialPrintln("");      
      break;
    }
  }
  if(gotIndex==false){
    mySerialPrintln("gotIndex==false");
    adr=0;    
  }
  return adr;  
}

void myPrintHex(uint32_t inputInt32){
  if      (inputInt32>0x0FFFFFFF){    mySerialPrint("0x");        }
  else if (inputInt32>0x00FFFFFF){    mySerialPrint("0x0");       }
  else if (inputInt32>0x000FFFFF){    mySerialPrint("0x00");      }
  else if (inputInt32>0x0000FFFF){    mySerialPrint("0x000");     }
  else if (inputInt32>0x00000FFF){    mySerialPrint("0x0000");    }
  else if (inputInt32>0x000000FF){    mySerialPrint("0x00000");   }
  else if (inputInt32>0x0000000F){    mySerialPrint("0x000000");  }
  else {                              mySerialPrint("0x0000000"); }  
  mySerialPrint(String(inputInt32, HEX));
}

void erase4K(unsigned long addr){
  unsigned long BEGIN = micros();
  myPrintHex(addr);
  if (flash.eraseSector(addr)) {
    mySerialPrintln(F("                    erase 4KB"));
  }
  else {
    mySerialPrintln(F("                    Erasing sector failed"));
  } 
  unsigned long END = micros();
  mySerialPrint("erase4K() time through (us): "); mySerialPrintln(END-BEGIN);
  //delay(10);
}

void writeToFlash(char str[ARRSZ+1]){
  //mySerialPrintln(micros()); 
  if(gAdr>(CAPA-ARRSZ)){
    //mySerialPrint("                    "); 
    myPrintHex(gAdr);
    mySerialPrintln(" reset gAdr=0"); 
    gAdr=0; 
  }
  if(gAdr%4096==0){ erase4K(gAdr); }

  if (flash.writeCharArray(gAdr, str, ARRSZ, true)) {
    //mySerialPrint("                    "); 
    myPrintHex(gAdr);
    mySerialPrint(F(" W: ")); mySerialPrintln(str);
    
    //讀回來看看    
    /*char textAdr[ARRSZ];
    myPrintHex(gAdr);    
    if (flash.readCharArray(gAdr, textAdr, ARRSZ)) {
      mySerialPrint(F(" R: ")); mySerialPrintln(textAdr); 
    }     
    else{
      mySerialPrintln("read fail!"); 
    }*/
    
    gAdr+=ARRSZ;     
  }
  //mySerialPrintln(micros()); 
}
#endif

template <class T> void mySerialPrint(T str){
#ifdef ENABLE_SERIAL
   Serial.print(str);
#endif
}
template <class T> void mySerialPrintln(T str){
#ifdef ENABLE_SERIAL
   Serial.println(str);
#endif
}
