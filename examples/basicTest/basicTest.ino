// Copyright 2020 Alan Tracey Wootton
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <ESP8266WiFi.h>

#include "mqtt5nano.h"

// Update these with values suitable for your network.

const char* ssid = "woot2";
const char* password = "fly1440nap";
const char* mqtt_server = "192.168.86.159";

const char *mqtt_user = "auser";
const char *mqtt_password = "eyJhbGciOiJFZDI1NTE5IiwidHlwIjoiSldUIn0.eyJleHAiOjE2MDkzNzI4MDAsImlzcyI6Ii85c2giLCJqdGkiOiIwZ2dBNFJIRjV0czdxOWNxTC9NK2czemMiLCJpbiI6MzIsIm91dCI6MzIsInN1Ijo0LCJjbyI6MiwidXJsIjoia25vdGZyZWUubmV0In0.43JcJsNNeCvElP8ZF9IT6G5vkhgPN85wsE8o2_6h7SKvTSsEWR0ldP5H9bP-NPymrCOYork2OeXRJKjfl9j0DQ";

WiFiClient espClient;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// the setup function runs once when you press reset or power the
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  Serial.print("hello mqtt5nano. 2");


}

// the loop function runs over and over again forever
void loop() {

  nano::slice sl;

  int tmp = sizeof(nano::mqttPacketPieces);
  int tmp2 = sizeof(nano::slice[8]);

  Serial.print(tmp);
  Serial.print(" ");
  Serial.print(tmp2);
  Serial.print(" Looping test ... atw ... \n");

  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
