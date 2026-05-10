#include <TinyGPSPlus.h>
//#include <SoftwareSerial.h>
#include <M5Unified.h>
#include "MultipleSatellite.h"

/*
   This sample code tracks satellite elevations using TinyGPSCustom objects.

   Satellite numbers and elevations are not normally tracked by TinyGPSPlus, but 
   by using TinyGPSCustom we get around this.

   It requires the use of SoftwareSerial and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(RX) and 3(TX).
*/
static const int RXPin = 13, TXPin = 27;
static const uint32_t GPSBaud = 115200;

//static const int RXPin = 4, TXPin = 3;
//static const uint32_t GPSBaud = 4800;
static const int MAX_SATELLITES = 40;
static const int PAGE_LENGTH = 40;

#if 0
// The TinyGPSPlus object
    TinyGPSPlus gps;
    // The serial connection to the GPS device
    SoftwareSerial ss(RXPin, TXPin);
#else
    // Create an instance of MultipleSatellite, assuming we use the Serial1 seri
    MultipleSatellite gps(Serial1, GPSBaud, SERIAL_8N1, RXPin, TXPin);
#endif



TinyGPSCustom totalGPGSVMessages(gps, "GPGSV", 1); // $GPGSV sentence, first element
TinyGPSCustom messageNumber(gps, "GPGSV", 2);      // $GPGSV sentence, second element
TinyGPSCustom satNumber[4]; // to be initialized later
TinyGPSCustom elevation[4];
bool anyChanges = false;
unsigned long linecount = 0;

struct
{
  int elevation;
  bool active;
} sats[MAX_SATELLITES];

void setup()
{

  m5::M5Unified::config_t cfg = M5.config();
    //std::cout << "Type of cfg: " << typeid(cfg).name() << std::endl;

    // Set the items you want to configure. Omit the following two lines if
    cfg.serial_baudrate = 115200;
    cfg.output_power = true;
    cfg.output_power = true;

    M5.begin(cfg);

    M5.Power.setExtOutput(false);  // reset gps
    delay(1000);
    M5.Power.setExtOutput(true);    // restart gps

  Serial.begin(115200);
  gps.begin();

  Serial.println(F("SatElevTracker.ino"));
  Serial.println(F("Displays GPS satellite elevations as they change"));
  Serial.print(F("Testing TinyGPSPlus library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();

  String version = gps.getGNSSVersion();
  Serial.printf("GNSS SW=%s\r\n", version.c_str());
  delay(1000);
  // Set satellite mode
  gps.setSatelliteMode(SATELLITE_MODE_GPS);

  gps.setSystemBootMode(BOOT_FACTORY_START);
 
  // Initialize all the uninitialized TinyGPSCustom objects
  for (int i=0; i<4; ++i)
  {
    satNumber[i].begin(gps, "GPGSV", 4 + 4 * i); // offsets 4, 8, 12, 16
    elevation[i].begin(gps, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
  }
}

void loop()
{
  // Dispatch incoming characters
  // if (gps.available() > 0)
  gps.updateGPS();

  static uint16_t satCount;
  uint16_t satCountNow = gps.satellites.value();

  if (satCount != satCountNow)
  {
    Serial.printf("sat count = %d\n", satCountNow);
    satCount = satCountNow;
  }

  static double keepLat, keepLng;
  static uint32_t loopCtr = 0;

  double nowLat, nowLng;
  loopCtr++;

  if (gps.location.isUpdated()) {
      nowLat = gps.location.lat();
      nowLng = gps.location.lng();

      if (nowLat != keepLat || nowLng != keepLng)
      {
          Serial.printf("%8d %10.8f %10.8f \n", loopCtr, nowLat, nowLng);
      }
  }

  delay(1000);

  gps.updateGPS();
  {
    gps.encode(gps.read());
   
    if (totalGPGSVMessages.isUpdated())
    {
      for (int i=0; i<4; ++i)
      {
        int no = atoi(satNumber[i].value());
        if (no >= 1 && no <= MAX_SATELLITES)
        {
          int elev = atoi(elevation[i].value());
          sats[no-1].active = true;
          if (sats[no-1].elevation != elev)
          {
            sats[no-1].elevation = elev;
            anyChanges = true;
          }
        }
      }
      
      int totalMessages = atoi(totalGPGSVMessages.value());
      int currentMessage = atoi(messageNumber.value());
      if (totalMessages == currentMessage && anyChanges)
      {
        if (linecount++ % PAGE_LENGTH == 0)
          printHeader();
        TimePrint();
        for (int i=0; i<MAX_SATELLITES; ++i)
        {
          Serial.print(F(" "));
          if (sats[i].active)
            IntPrint(sats[i].elevation, 2);
          else
            Serial.print(F("   "));
          sats[i].active = false;
        }
        Serial.println();
        anyChanges = false;
      }
    }
  }
}

void IntPrint(int n, int len)
{
  int digs = n < 0 ? 2 : 1;
  for (int i=10; i<=abs(n); i*=10)
    ++digs;
  while (digs++ < len)
    Serial.print(F(" "));
  Serial.print(n);
  Serial.print(F(" "));
}

void TimePrint()
{
  if (gps.time.isUpdated())
  {
    if (gps.time.hour() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F(" "));
  }
  else
  {
    Serial.print(F("(unknown)"));
  }
}

void printHeader()
{
  Serial.println();
  Serial.print(F("Time     "));
  for (int i=0; i<MAX_SATELLITES; ++i)
  {
    Serial.print(F(" "));
    IntPrint(i+1, 2);
  }
  Serial.println();
  Serial.print(F("---------"));
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.print(F("----"));
  Serial.println();
}
