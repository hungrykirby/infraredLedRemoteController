#include <WiFi.h>
#include <WebServer.h>
// https://github.com/arduino-libraries/Arduino_JSON
#include <Arduino_JSON.h>

//========================================================
// Config
//========================================================

#define SERIAL_BPS             (57600)   /* Serial bps */
#define IR_IN                  (5)       /* Input      */
#define IR_OUT                 (16)      /* Output     */

//========================================================
// Define
//========================================================

#define DATA_SIZE              (800)

//========================================================
// Program
//========================================================

/* 
こういう感じに関数を env.ino に定義して使う

char* wifi_ssid() {
  return "****";
}

char* wifi_password() {
  return "****";
}

*/

String body = "Initial String";

WebServer server(80);

void toWhite() {
  int data[] = {3575,1653,478,392,502,370,473,1262,503,1238,478,393,501,1238,505,364,501,372,500,371,500,1241,497,370,476,392,500,1239,502,367,499,1241,478,392,501,1237,501,370,476,395,499,1238,503,1236,478,1260,504,371,478,390,474,395,475,1266,498,371,499,1238,476,395,502,369,500,371,475,1263,478,1261,501,1238,503,370,499,370,475,1262,479,1260,477,395,500,1235,504};
  int num_data = sizeof(data)/sizeof(data[0]);
  turn(data, num_data);
}

void toOrange() {
  int data[] = {3547,1682,500,369,471,399,472,1265,473,1270,473,394,471,1271,471,399,444,426,447,422,447,1291,448,424,447,420,472,1269,449,420,472,1269,471,398,474,1264,449,424,472,397,473,1265,449,1289,448,1292,472,399,446,423,447,1294,448,1291,447,423,446,1293,448,422,448,421,473,398,470,1268,449,423,446,1291,448,424,446,424,449,1288,448,1292,470,401,470,1267,471};
  int num_data = sizeof(data)/sizeof(data[0]);
  turn(data, num_data);
}

void turn(int data[], int num_data) {
  unsigned short time = 0;
  signed long us = 0;

   for(int count = 0; count < num_data; count++){
    time = data[count];
    us = micros();
    do {
      digitalWrite(IR_OUT, !(count&1));
      delayMicroseconds(11);
      digitalWrite(IR_OUT, 0);
      delayMicroseconds(10);
    } while ((signed long)(us + time - micros()) > 0);
  }
}

void setup() {
  char* ssid = wifi_ssid();
  char* password = wifi_password();

  Serial.begin(SERIAL_BPS);

  pinMode(IR_IN, INPUT);
  pinMode(IR_OUT, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  server.on("/control", HTTP_ANY, [](){
    if (server.method() == HTTP_POST) { // POSTメソッドでアクセスされた場合
      body = server.arg("plain"); // server.arg("plain")でリクエストボディが取れる
      JSONVar json;
      json = JSON.parse(body);
      if(json.hasOwnProperty("color")) {
        if( String((const char*)json["color"]) == "white") {
          toWhite();
        } else if( String((const char*)json["color"]) == "orange") {
          toOrange();
        }
      }
      Serial.println((const char*)json["color"]);
    }
    server.send(200, "text/plain", body); // 値をクライアントに返す
  });

  // 登録されてないパスにアクセスがあった場合
  server.onNotFound([](){
    server.send(404, "text/plain", "Not Found."); // 404を返す
  });

  server.begin();
}

void loop() {
  server.handleClient();
}