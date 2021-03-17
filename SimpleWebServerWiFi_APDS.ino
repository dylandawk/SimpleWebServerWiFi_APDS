#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include <Arduino_JSON.h>
#include <Arduino_APDS9960.h>
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

char serverAddress[] = "192.168.1.228";  // server address
int port = 3000;

// initialize WiFi connection:
WiFiClient wifi;
//MqttClient mqttClient(wifi);
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
int requestDelay = 20;


// details for MQTT client:
//char broker[] = "public.cloud.shiftr.io";
//int port = 1883;
//char topic[] = "notes";
//char clientID[] = "buttonClient";

JSONVar sensorData;


int proximity = 0;
int _gesture = -1;
int _prevGesture = -1;
int r = 0, g = 0, b = 0;
unsigned long lastUpdate = 0;
unsigned long lastGestureUpdate = 0;
bool isClient;

void setup() {
  Serial.begin(9600);      // initialize serial communication
  // wait for serial monitor to open:
  while (!Serial);

  // check for the WiFi module:
  // initialize WiFi, if not connected:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.println(SECRET_SSID);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print IP address once connected:
  Serial.print("Connected. My IP address: ");
  Serial.println(WiFi.localIP());

  //  // set the credentials for the MQTT client:
  //  mqttClient.setId(clientID);
  //  mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PASS);
  //
  //  // try to connect to the MQTT broker once you're connected to WiFi:
  //  while (!connectToBroker()) {
  //    Serial.println("attempting to connect to broker");
  //    delay(1000);
  //  }
  //  Serial.println("connected to broker");

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS9960 sensor.");
    while (true); // Stop forever
  }

  printWifiStatus();                        // you're connected now, so print out the status
  updateJSON();
}

void updateJSON() {
  sensorData["r"] = r;
  sensorData["g"] = g;
  sensorData["b"] = b;
  sensorData["distance"] = proximity;

  // refresh gesture after 2 secs
  if(millis() - lastGestureUpdate > 2000 && _gesture == _prevGesture){
    lastGestureUpdate = millis();
    _gesture = -1;
  }
  sensorData["gesture"] = _gesture;

}

void loop() {
  //checkMqtt();
  checkAPDS();
  makePOSTRequest();
}

//void checkMqtt() {
//  // if not connected to the broker, try to connect:
//  if (!mqttClient.connected()) {
//    Serial.println("reconnecting");
//    connectToBroker();
//  }
//
//  delay(1000);
//  mqttClient.beginMessage(topic);
//  // add a random number as a numeric string (print(), not write()):
//  mqttClient.print(random(16));
//  // send the message:
//  mqttClient.endMessage();
//
//}

void makePOSTRequest() {
  if (millis() - lastUpdate > requestDelay) {
    lastUpdate = millis();
    String contentType = "application/json";
    String postData = JSON.stringify(sensorData);

    client.post("/api/data", contentType, postData);

    // read the status code and body of the response
    int statusCode = client.responseStatusCode();
    String response = client.responseBody();

//    Serial.print("Status code: ");
//    Serial.println(statusCode);
//    Serial.print("Response: ");
//    Serial.println(response);
//    Serial.println(JSON.stringify(sensorData));
//    Serial.println("Wait 20 millis");
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
  //   Print updates every 100ms
  if (millis() - lastUpdate > 1000) {
    //lastUpdate = millis();
    //      Serial.print("PR=");
    //      Serial.print(proximity);
    //      Serial.print(" rgb=");
    //      Serial.print(r);
    //      Serial.print(",");
    //      Serial.print(g);
    //      Serial.print(",");
    //      Serial.println(b);
    //        Serial.println(JSON.stringify(sensorData));
  }
}

//boolean connectToBroker() {
//  // if the MQTT client is not connected:
//  if (!mqttClient.connect(broker, port)) {
//    // print out the error message:
//    Serial.print("MOTT connection failed. Error no: ");
//    Serial.println(mqttClient.connectError());
//    // return that you're not connected:
//    return false;
//  }
//  // once you're connected, you can proceed:
//  mqttClient.subscribe(topic);
//  // return that you're connected:
//  return true;
//}

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
