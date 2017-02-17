// color picker & webinterface code by (C) Jean-Marie Bussat - Creative Commons Attribution 4.0 International Public License
// wifi indicator code by c5e3
// special thanks to adafruit & the esp8266 community for the great libraries!
#include "ESP8266WiFi.h"
#include "html_header.h"
#include <Adafruit_NeoPixel.h>
#include <ESP8266WebServer.h>

#define PIXEL_PIN    D5
#define PIXEL_COUNT  1

#define cfact 2.2

ESP8266WebServer server(80);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ400);

const char* password = "";
const char* ssid     = "BlinkenESP";
unsigned char red=0, green=0, blue=0;
String form;

// function prototypes
uint32_t Wheel(byte);
void wificount_ISR(void);
void handle_root();
void handle_color();

// global variables
int n = 0, hrssicol = 0, lrssicol = 0;

void setup() {
  pinMode(PIXEL_PIN, OUTPUT);
  digitalWrite(PIXEL_PIN, LOW);
  pinMode(16, OUTPUT); //red led
  digitalWrite(16, HIGH); //turn off

  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(10, 0, 0, 1), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, password, 13, 0);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  strip.begin();
  strip.show();
  attachInterrupt(digitalPinToInterrupt(0), wificount_ISR, FALLING);

  server.on("/", handle_info);
  server.on("/color", handle_color);
  server.begin();
  
  delay(100);
}

void loop() {
  server.handleClient();
  // wifi indicator mode when no colors set by webinterface or set to black
  if( red == 0 && green == 0 && blue == 0 ){
    // number of networks
    n = WiFi.scanNetworks();
  
    // variables for highest and lowest rssi value
    int hrssi = -120;
    int lrssi = 0;
  
    // temp. variable for sum of rssi values
    int m = 0;
  
    if (n == 0) {
      Serial.println("no networks found");
      for ( int i = 0; i < 5; i++ ) {
        strip.setPixelColor(0, 127, 0, 0);
        strip.show();
        delayMicroseconds(200000);
        strip.setPixelColor(0, 0);
        strip.show();
        delayMicroseconds(200000);
      }
    } else {
      Serial.println("-------------------------");
      for ( int i = 0; i < n; i++ ) {
        m += WiFi.RSSI(i);
  
        // finding highest value
        if ( WiFi.RSSI(i) > hrssi) {
          hrssi = WiFi.RSSI(i);
        }
  
        // finding lowest value
        if ( WiFi.RSSI(i) < lrssi) {
          lrssi = WiFi.RSSI(i);
        }
  
        // infos via serial
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        ESP.wdtFeed();
      }
  
      // calulating color value by shifting average rssi up by 100 and multiplying by a factor for spreading out the range
      int rssicol = ( (m / n) + 100 ) * cfact;
  
      // infos via serial
      Serial.print("high RSSI: ");
      Serial.println(hrssi);
      Serial.print("high RSSI: ");
      Serial.println(lrssi);
  
      // set color values for ISR
      hrssicol = ( hrssi + 100 ) * cfact;
      lrssicol = ( lrssi + 100 ) * cfact;
  
      // infos via serial
      Serial.println("-------------------------");
      Serial.print("average RSSI: ");
      Serial.println(m / n);
      Serial.print("color: ");
      Serial.println(rssicol);
      Serial.print("high RSSI color: ");
      Serial.println(hrssicol);
      Serial.print("low RSSI color: ");
      Serial.println(lrssicol);
  
      // set color finally
      strip.setPixelColor(0, Wheel(rssicol) );
      strip.show();
  
      Serial.println("");
    }
  } else {
    // color picker mode as soon as set by webinterface
    for (int i = 0; i < PIXEL_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(red, green, blue));
      strip.show();
    }
    delay(500);
  }
}

void wificount_ISR() {
  Serial.println("ISR BEGIN");
  if( red == 0 && green == 0 && blue == 0 ){
    digitalWrite(16, LOW);
  
    // turn off LED
    strip.setPixelColor(0, 0);
    strip.show();
  
    delayMicroseconds(500000);
  
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      // blink n times, where n is number of networks
      for ( int i = 0; i < n; i++ ) {
        strip.setPixelColor(0, 127, 127, 127);
        strip.show();
        delayMicroseconds(150000);
        strip.setPixelColor(0, 0);
        strip.show();
        delayMicroseconds(150000);
        ESP.wdtFeed();
      }
  
      delayMicroseconds(800000);
  
      // show color for highest value
      strip.setPixelColor(0, Wheel(hrssicol) );
      strip.show();
      delayMicroseconds(1000000);
      strip.setPixelColor(0, 0);
      strip.show();
      delayMicroseconds(200000);
  
      // show color for lowest value
      strip.setPixelColor(0, Wheel(lrssicol) );
      strip.show();
      delayMicroseconds(1000000);
      strip.setPixelColor(0, 0);
      strip.show();
      delayMicroseconds(200000);
    }
  } else {
    Serial.println("color picker mode");
  }
  Serial.println("ISR END");
  digitalWrite(16, HIGH);
}

void handle_color() {
  // Strings to strore the client output
  String RMsg;
  String GMsg;
  String BMsg;

  // Parse client output
  RMsg = server.arg("R");
  GMsg = server.arg("G");
  BMsg = server.arg("B");

  // Convert to number to pass to Neopixel library
  red = RMsg.toInt();
  green = GMsg.toInt();
  blue = BMsg.toInt();

  // Update the form and send it back to the client
  form = "<html>";
  form = form + html_header;
  form = form + "</html>";
  // Send the form
  server.send(200, "text/html", form);
}

void handle_info() {
  server.send(200, "text/html", "<html><h1>ESP8266 WS2812 color picker<br />set color to black for scanning mode<br /><a href='/color'>START</a></h1></html>");
  delay(100);
}

// function from adafruit example
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
