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
#define TIMEOUT_RECV_NOSIGNAL  (50000)
#define TIMEOUT_RECV           (5000000)
#define TIMEOUT_SEND           (2000000)

//========================================================
// Define
//========================================================
#define STATE_NONE             (-1)
#define STATE_OK               (0)
#define STATE_TIMEOUT          (1)
#define STATE_OVERFLOW         (2)
#define DATA_SIZE              (800)
#define MSEC_IN_DEFAULT        (11)
#define MSEC_OUT_DEFAULT       (10)

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
unsigned short recData[DATA_SIZE];

WebServer server(80);

String recvSignal(){
  
  unsigned char pre_value = HIGH;
  unsigned char now_value = HIGH;
  // true -> 1 , false -> 0
  unsigned char wait_flag = 1;
  signed char state = STATE_NONE;
  unsigned long pre_us = micros();
  unsigned long now_us = 0;
  unsigned long index = 0;
  unsigned long i = 0;

  String result = "";
  
  while(state == STATE_NONE){
    now_value = digitalRead(IR_IN);
    if(pre_value != now_value){
      now_us = micros();
      if(!wait_flag){
        recData[index++] = now_us - pre_us;
      }
      wait_flag = 0;
      pre_value = now_value;
      pre_us = now_us;
    }
    
    if(wait_flag){
      if((micros() - pre_us) > TIMEOUT_RECV){
        state = STATE_TIMEOUT;
        break;
      }
    } else {
      if((micros() - pre_us) > TIMEOUT_RECV_NOSIGNAL){
        state = STATE_OK;
        break;
      }
    }
  }
  
  if(state == STATE_OK){
    for(i = 0; i < index; i++){
      result += (String) recData[i];
      result += ",";
    }
    result += "0,";
  } else {
    result = "NG";
  }
  return result;
}

int receiveSignals(String signals, int msec_after_in, int msec_after_out) {
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
  turn(data, index + 1, msec_after_in, msec_after_out);
  return (index + 1);
}

void turn(int data[], int num_data, int msec_after_in, int msec_after_out) {
  unsigned short time = 0;
  signed long us = 0;

   for(int count = 0; count < num_data; count++){
    time = data[count];
    us = micros();
    do {
      digitalWrite(IR_OUT, !(count&1));
      delayMicroseconds(msec_after_in);
      digitalWrite(IR_OUT, 0);
      delayMicroseconds(msec_after_out);
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
    int signalsCount = 1;
    if (server.method() == HTTP_POST) { // POSTメソッドでアクセスされた場合
      body = server.arg("plain"); // server.arg("plain")でリクエストボディが取れる
      JSONVar json;
      json = JSON.parse(body);
      if(json.hasOwnProperty("signals")) {
        if (json.hasOwnProperty("count")){
          signalsCount = (String((const char*)json["count"])).toInt();
          result = signalsCount;
        }
        int msec_after_in = MSEC_IN_DEFAULT;
        int msec_after_out = MSEC_OUT_DEFAULT;
        if (json.hasOwnProperty("msecin")){
          msec_after_in = (String((const char*)json["msecin"])).toInt();
        }
        if (json.hasOwnProperty("msecout")){
          msec_after_out = (String((const char*)json["msecout"])).toInt();
        }
        for(int i = 0; i < signalsCount; i++){
          receiveSignals(String((const char*)json["signals"]), msec_after_in, msec_after_out);
          if (signalsCount > 1) delay(100);
        }
      }
    }
    server.send(200, "text/plain", (String) result + "\n"); // 値をクライアントに返す
  });

  server.on("/record", HTTP_ANY, [](){
    String result = "";
    if (server.method() == HTTP_POST) { // POSTメソッドでアクセスされた場合
      body = server.arg("plain"); // server.arg("plain")でリクエストボディが取れる
      JSONVar json;
      json = JSON.parse(body);
      if(json.hasOwnProperty("mode")) {
        if (String((const char*)json["mode"]) == "record") {
          result = recvSignal();
        } 
      }
    }
    server.send(200, "text/plain", result + "\n"); // 値をクライアントに返す
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