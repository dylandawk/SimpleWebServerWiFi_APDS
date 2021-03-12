#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_JSON.h>
#include <Arduino_APDS9960.h>
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);
JSONVar sensorData;


int proximity = 0;
int _gesture = -1;
int r = 0, g = 0, b = 0;
unsigned long lastUpdate = 0;
bool isClient;

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(9, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
    while (true); // Stop forever
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
  updateJSON();
}

void updateJSON() {
  sensorData["r"] = r;
  sensorData["g"] = g;
  sensorData["b"] = b;
  sensorData["distance"] = proximity;
  sensorData["gesture"] = _gesture;
}

void loop() {
  checkWifi();
  if (!isClient)checkAPDS();
}

void checkWifi() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    isClient = true;
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println();

            // the content of the HTTP response follows the header:
            //  client.print("Click <a href=\"/H\">here</a> turn the LED on pin 9 on<br>");
            //  client.print("Click <a href=\"/L\">here</a> turn the LED on pin 9 off<br>");
            client.print(JSON.stringify(sensorData));

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // close the connection:
    client.stop();
    isClient = false;
    Serial.println("client disonnected");
  }
}

void checkAPDS() {
  // Check if a proximity reading is available.
  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }

  // check if a color reading is available
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b);
  }

  // check if a gesture reading is available
  if (APDS.gestureAvailable()) {
    int gesture = APDS.readGesture();
    switch (gesture) {
      case GESTURE_UP:
        Serial.println("Detected UP gesture");
        _gesture = GESTURE_UP;
        break;

      case GESTURE_DOWN:
        Serial.println("Detected DOWN gesture");
        _gesture = GESTURE_DOWN;
        break;

      case GESTURE_LEFT:
        Serial.println("Detected LEFT gesture");
        _gesture = GESTURE_LEFT;
        break;

      case GESTURE_RIGHT:
        Serial.println("Detected RIGHT gesture");
        _gesture = GESTURE_RIGHT;
        break;

      default:
        break;
    }
  }
  updateJSON();
  // Print updates every 100ms
  //  if (millis() - lastUpdate > 1000) {
  //    lastUpdate = millis();
  //    Serial.print("PR=");
  //    Serial.print(proximity);
  //    Serial.print(" rgb=");
  //    Serial.print(r);
  //    Serial.print(",");
  //    Serial.print(g);
  //    Serial.print(",");
  //    Serial.println(b);
  //  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
