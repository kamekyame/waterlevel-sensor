#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>
#include <ArduinoOTA.h>
#include <string>

#include "setting.h"
#include "wifi_util.h"

#define TRIG 12
#define ECHO 13
#define LED 4

ESP8266WebServer server(8880);

double measureData[3 * 60 * 24] = {0};
int dataIndex = 0;

void setup()
{
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);

  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);

  Serial.begin(115200);
  Serial.println("Hello waterlevel sensor! v1.1");

  //printEspStatus();
  connectWifi();

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_FS
      type = "filesystem";

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setupServer();

  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  digitalWrite(LED, HIGH);
}

void handleWebhook()
{
  server.send(200);
  if (server.method() != HTTP_POST)
    return;
  String body = server.arg("plain");
  JSONVar doc = JSON.parse(body);
  String type = (const char *)doc["events"][0]["type"];
  Serial.print("Event Type: ");
  Serial.println(type);
  if (type != "message")
    return;
  String messageType = (const char *)doc["events"][0]["message"]["type"];
  Serial.print("Message Type: ");
  Serial.println(messageType);
  if (messageType != "text")
    return;
  String text = (const char *)doc["events"][0]["message"]["text"];
  String replyToken = (const char *)doc["events"][0]["replyToken"];
  if (text != "水位")
    return;
  replyMessage(replyToken);
}

void handleGet()
{
  double dis = measureAv();
  server.send(200, "text/plain", String("現在の水位は") + dis + "cmです。");
}

void handleGetAll()
{
  String text = "";
  for (int i = 0; i < 6 * 60 * 24; i++)
  {
    text += String(measureData[i]) + "\r\n";
  }
}

void setupServer()
{
  server.on("/", handleWebhook);
  server.on("/get", handleGet);
  server.on("/getAll", handleGetAll);
  server.begin();
  Serial.println("HTTP server started");
  Serial.println();
}

void printEspStatus(void)
{
  delay(100);
  Serial.println();

  Serial.println("-----ESP-WROOM-02 ( ESP8266 ) Chip Infomation -----");
  Serial.println();

  Serial.print("Core Version = ");
  Serial.println(ESP.getCoreVersion());

  Serial.print("CPU Frequency = ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  Serial.print("ChipID = ");
  Serial.println(ESP.getChipId(), HEX); //MACアドレスの下位３バイト

  Serial.print("Flash ID = ");
  Serial.println(ESP.getFlashChipId(), HEX);

  Serial.print("SDK version = ");
  Serial.println(ESP.getSdkVersion());

  Serial.print("Boot version = ");
  Serial.println(ESP.getBootVersion());

  Serial.print("Boot Mode = ");
  Serial.println(ESP.getBootMode());

  Serial.print("Flash Chip IDE Size = ");
  Serial.print(ESP.getFlashChipSize()); //Arduino IDE 設定の Flash Size になる
  Serial.println(" byte");

  Serial.print("Flash Chip Real Size = ");
  Serial.print(ESP.getFlashChipRealSize()); //ESP-WROOM-32 内蔵最大 Flash Size になる
  Serial.println(" byte");

  Serial.print("Flash Frequency = ");
  Serial.print(ESP.getFlashChipSpeed());
  Serial.println(" Hz");

  String mode_str;
  switch (ESP.getFlashChipMode())
  {
  case 0:
    mode_str = "QIO";
    break;
  case 1:
    mode_str = "QOUT";
    break;
  case 2:
    mode_str = "DIO";
    break;
  case 3:
    mode_str = "DOUT";
    break;
  case 4:
    mode_str = "Unknown";
    break;
  default:
    break;
  }
  Serial.println("Flash Chip Mode = " + mode_str);

  Serial.print("Free Heap Size = ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("Free Sketch Size = ");
  Serial.println(ESP.getFreeSketchSpace());

  Serial.print("Sketch Size = ");
  Serial.println(ESP.getSketchSize());

  Serial.println();

  uint8_t mac0[6];
  WiFi.macAddress(mac0);
  Serial.printf("WiFi StationAP Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac0[0], mac0[1], mac0[2], mac0[3], mac0[4], mac0[5]);
}

void loop()
{
  ArduinoOTA.handle();
  server.handleClient();

  checkDate();
}
int m = 0;

void checkDate()
{
  if (millis() - m > 2000)
  {
    m = millis();
    time_t t;
    struct tm *tm;
    //static const char *wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
    t = time(NULL);
    tm = localtime(&t);

    if (tm->tm_min == 0 && tm->tm_sec == 0)
    {
      Serial.println("0分0秒!");
      double dis = measureAv();
      pushMessage();
    }
    if (tm->tm_sec % 20 == 0)
    {
      dataIndex = floor(tm->tm_sec / 20) + tm->tm_min * 6 + tm->tm_hour * 6 * 60;
      Serial.print("index: ");
      Serial.println(dataIndex);
      measureData[dataIndex] = measureAv();
    }
    //  return 1;
    /*Serial.printf("ESP8266/Arduino ver%s :  %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
                __STR(ARDUINO_ESP8266_GIT_DESC),
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                wd[tm->tm_wday],
                tm->tm_hour, tm->tm_min, tm->tm_sec);*/
  }
}

double measure()
{
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); //超音波を出力
  delayMicroseconds(10);    //
  digitalWrite(TRIG, LOW);
  double duration = pulseIn(ECHO, HIGH); //センサからの入力
  double distance = 0;
  if (duration > 0)
  {
    //duration = duration / 2;                   //往復距離を半分にする
    distance = duration * 340 * 100 / 1000000 / 2; // 音速を340m/sに設定
    Serial.print("Distance:");
    Serial.print(distance);
    Serial.println(" cm");
  }
  return distance;
}

double measureAv()
{
  double distances = 0;
  for (int i = 0; i < 5; i++)
  {
    distances += measure();
  }
  distances /= 5;
  Serial.printf("水位: %lfcm\r\n", distances);
  return distances;
}

String getWaterlevelText()
{
  return String("現在の水位は") + measureAv() + "cmです。";
}

void pushMessage()
{
  postLineMessageApi("broadcast", "{\"messages\":[{\"type\":\"text\",\"text\":\"" + getWaterlevelText() + "\"}]}");
}

void replyMessage(String replyToken)
{
  postLineMessageApi("reply", "{\"replyToken\": \"" + replyToken + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + getWaterlevelText() + "\"}]}");
}

void postLineMessageApi(const char *type, String content)
{
  Serial.println("connecting to Line Messaging API");
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.line.me", 443))
  {
    Serial.println("HTTPS connection failed");
    return;
  }

  String str = String("POST /v2/bot/message/") + type + " HTTP/1.1\r\n" +
               "Host: api.line.me\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n" +
               "Authorization: Bearer " + CHANNEL_SECRET + "\r\n" +
               "Content-type: application/json\r\n" +
               "Content-Length: " + content.length() + "\r\n\r\n" +
               content + "\r\n";

  client.print(str);
  //Serial.println(str);

  Serial.println("sended");
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
    {
      //Serial.println("headers received");
      break;
    }
  }
  while (client.available())
  {
    char c = client.read();
    Serial.write(c);
  }

  client.stop();

  Serial.println("closing connection");
}
