
#include <ESP8266WiFi.h>
#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif
#ifndef max
#define max(x,y) (((x)>(y))?(x):(y))
#endif
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <time.h>

const char SSID[]     = "WIFI_PROJ_SMI_2017";
const char PASSWORD[] = "helloworld";

// Use your own API key by signing up for a free developer account.
// http://www.wunderground.com/weather/api/
#define WU_API_KEY "a9790f2254d417c1"

#define WU_LOCATION "autoip"

// 5 minutes between update checks. The free developer account has a limit
// on the  number of calls so don't go wild.
#define DELAY_NORMAL    (5*60*1000)
// 20 minute delay between updates after an error
#define DELAY_ERROR     (20*60*1000)

#define WUNDERGROUND "api.wunderground.com"

// HTTP request
const char WUNDERGROUND_REQ[] =
    "GET /api/" WU_API_KEY "/conditions/q/" WU_LOCATION ".json HTTP/1.1\r\n"
    "User-Agent: ESP8266/0.1\r\n"
    "Accept: */*\r\n"
    "Host: " WUNDERGROUND "\r\n"
    "Connection: close\r\n"
    "\r\n";

#define WIFI_CONNECTED ( (char)0x01)
#define TCP_CONNECTED (char)0x02
#define TCP_NOT_CONNECTED (char)0x03
#define HTTP_RESPONSE_OK (char)0x04
#define HTTP_RESPONSE_NOT_OK (char)0x05
#define JSON_OK (char)0x6
#define JSON_NOT_OK (char)0x7
#define UDP_CONNECTED (char)0x8
#define UDP_NOT_CONNECTED (char)0x9
#define UDP_RESPONSE_OK (char)0x10
#define UDP_RESPONSE_NOT_OK (char)0x11


#define GET_WEATHER_UPDATE 'w'
#define GET_TIME_UPDATE 't'

void setup()
{
  // set baudrate
  Serial.begin(115200);

  Serial.println("Starting");
}

/*Weather related data !*/
static char respBuf[4096];

// sunny code 1
uint8_t sunny_length = 2;
char sunny[][20] = {
     "sunny", 
     "clear"
  };

// cloudy code 2
uint8_t cloudy_length = 3;
char cloudy[][20] = {
     "cloudy",
     "fog",
     "hazy"
  };

// pc code 3
uint8_t partly_cloudy_length = 4;
char partly_cloudy[][20] = {
     "mostlycloudy", 
     "mostlysunny",
     "partlycloudy",
     "partlysunny"
  };

// rain code 4
uint8_t rain_length = 6;
char rain[][20] = {
     "chancerain",
     "rain",
     "chancetstorms",
     "tstorms",
     "chancesleet",
     "sleet"
  };


// rain code 5
uint8_t snow_length = 4;
char snow[][20] = {
     "chancesnow",
     "snow",
     "chanceflurries",
     "flurries"
  };


uint8_t getIconCode(const char* icon)
{
  uint8_t i;

  //check sunny 
  for( i = 0; i<sunny_length; i++ )
    if(strcmp(icon, sunny[i]) == 0)
      return 1;

  //check cloudy 
  for( i = 0; i<cloudy_length; i++ )
    if(strcmp(icon, cloudy[i]) == 0)
      return 2;

  //check partly cloudy
  for( i = 0; i<partly_cloudy_length; i++ )
    if(strcmp(icon, partly_cloudy[i]) == 0)
      return 3;

  // check  rain 
  for( i = 0; i<rain_length; i++ )
    if(strcmp(icon, rain[i]) == 0)
      return 4;

  // check snow 
  for( i = 0; i<snow_length; i++ )
    if(strcmp(icon, snow[i]) == 0)
      return 5;
      
  // else unknown
  return 0;
}

bool showWeather(char *json)
{
  StaticJsonBuffer<3*1024> jsonBuffer;

  // Skip characters until first '{' found
  // Ignore chunked length, if present
  char *jsonstart = strchr(json, '{');
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.print(JSON_NOT_OK);
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
     Serial.print(JSON_NOT_OK);
    return false;
  }

  Serial.print(JSON_OK);

  // Extract weather info from parsed JSON
  Serial.print("(");
  JsonObject& current = root["current_observation"];
  const float temp_c = current["temp_c"];
  Serial.print((int8_t )temp_c);
  Serial.print(",");
  const char *icon = current["icon"];
  Serial.print(getIconCode(icon));
  Serial.print(")");
  
  return true;
}

void getWeatherUpdate()
{
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(SSID, PASSWORD);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  Serial.print(WIFI_CONNECTED);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient httpclient;
  const int httpPort = 80;
  if (!httpclient.connect(WUNDERGROUND, httpPort)) {
    Serial.print(TCP_NOT_CONNECTED);
   // delay(DELAY_ERROR);
    return;
  } else {
    Serial.print(TCP_CONNECTED);  
  }

  // This will send the http request to the server
  httpclient.print(WUNDERGROUND_REQ);
  httpclient.flush();

  // Collect http response headers and content from Weather Underground
  // HTTP headers are discarded.
  // The content is formatted in JSON and is left in respBuf.
  int respLen = 0;
  bool skip_headers = true;
  while (httpclient.connected() || httpclient.available()) {
    if (skip_headers) {
      String aLine = httpclient.readStringUntil('\n');
      //Serial.println(aLine);
      // Blank line denotes end of headers
      if (aLine.length() <= 1) {
        skip_headers = false;
      }
    }
    else {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&respBuf[respLen], sizeof(respBuf) - respLen);
      if (bytesIn > 0) {
        respLen += bytesIn;
        if (respLen > sizeof(respBuf)) respLen = sizeof(respBuf);
      }
      else if (bytesIn < 0) {
        Serial.print(HTTP_RESPONSE_NOT_OK);
      }
    }
    delay(1);
  }
  httpclient.stop();
  Serial.print(HTTP_RESPONSE_OK);

  if (respLen >= sizeof(respBuf)) {
    Serial.print(HTTP_RESPONSE_NOT_OK);
    //delay(DELAY_ERROR);
    return;
  }
  // Terminate the C string
  respBuf[respLen++] = '\0';
  //Serial.println(respBuf);

  showWeather(respBuf);
  
 }
 
/*UDP related data*/
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

unsigned int localPort = 2390;   

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void getTimeUpdate()
{
  if(WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(SSID, PASSWORD);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  Serial.print(WIFI_CONNECTED);

  // Start udp port
  udp.begin(localPort);
  
  Serial.print(UDP_CONNECTED);

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(2000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.print(UDP_RESPONSE_NOT_OK);
  }
  else {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print(UDP_RESPONSE_OK);


    time_t secsSince1970 = secsSince1900 - 2208988800 +2*3600;
    //time_t tz
    struct tm * lt;
  //  tzset();  
    lt = localtime(&secsSince1970);
    
    Serial.print("(");
    Serial.print(lt->tm_year);
    Serial.print(",");
    Serial.print(lt->tm_mon);
    Serial.print(",");
    Serial.print(lt->tm_mday);
    Serial.print(",");
    Serial.print(lt->tm_hour);
    Serial.print(",");
    Serial.print(lt->tm_min);
    Serial.print(",");
    Serial.print(lt->tm_sec);   
    Serial.print(")");
    
  }

}

void loop()
{
  char cmd;
  if(Serial.available())
  {
    cmd = Serial.read();
    if(cmd == GET_WEATHER_UPDATE)
    {
      //Serial.println("Getting weather update");
      getWeatherUpdate();
    }else if (cmd == GET_TIME_UPDATE)
    {
       //Serial.println("Getting time update");
      getTimeUpdate();
    }

   // Serial.print(cmd);
  }
  
  yield();
  
}
