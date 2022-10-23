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

int receiveSignals(String signals) {
  int index = 0;
  // arraySizeのロジックを追加すると-1が返ってしまうため削除した
  // https://gangannikki.hatenadiary.jp/entry/2019/01/24/154015
  // 動くはずなんだが....
  // int arraySize = (sizeof(signals)/sizeof(signals[0]));  
  unsigned long signalslength = signals.length();

  // 配列を初期化するために長さをはかる
  for (int i = 0; i < signalslength; i++) {
    char tmp = signals.charAt(i);
    if ( tmp == ',' ) {
      index++;
    }
  }

  String dst[index + 1] = {"\0"};
  index = 0; // 再度oにする

  for (int i = 0; i < signalslength; i++) {
    char tmp = signals.charAt(i);
    if ( tmp == ',' ) {
      index++;
      // if ( index > (arraySize - 1)) return -1;
    }
    else dst[index] += tmp;
  }

  int data[index + 1] = {-1};
  for(int i = 0; i <= index; i++) {
    data[i] = dst[i].toInt();
  }
  turn(data, index + 1);
  return (index + 1);
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
    int result = 0;
    if (server.method() == HTTP_POST) { // POSTメソッドでアクセスされた場合
      body = server.arg("plain"); // server.arg("plain")でリクエストボディが取れる
      JSONVar json;
      json = JSON.parse(body);
      if(json.hasOwnProperty("signals")) {
        result = receiveSignals(String((const char*)json["signals"]));
      }
    }
    server.send(200, "text/plain", (String) result + "\n"); // 値をクライアントに返す
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