#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <string>

#include "wifi_util.h"

#define TRIG 12
#define ECHO 13
#define LED 4

WiFiClient client;

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

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                         type = "sketch";
                       else // U_FS
                         type = "filesystem";

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       Serial.println("Start updating " + type);
                     });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
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

  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  digitalWrite(LED, HIGH);
}
/*
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
}*/

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
  checkDate();
  //sendSocket();
}
int m = 0;

void checkDate()
{
  //Serial.println(millis() - m);
  if ((long)(millis()) - m > 900)
  {
    m = millis();
    time_t t;
    struct tm *tm;
    t = time(NULL);
    tm = localtime(&t);

    Serial.printf("%d:%d\n\r", tm->tm_min, tm->tm_sec);
    if (/*(tm->tm_min % 10) == 0 && */ tm->tm_sec == 0)
    {
      Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec);
      sendSocket();
      m = m + 2000;
    }
  }
}

void sendSocket()
{
  if (client.connect("192.168.0.3", 10000))
  {
    double m = measureAv();
    unsigned short mS = round(m * 100);
    unsigned char buf[2];
    for (int i = 0; i < 2; i++)
    {
      buf[i] = (mS >> (i * 8)) & 0xFF;
    }
    client.write(buf, 2);
    client.stop();
  }
  else
  {
    Serial.println("Connection failed");
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
    /*Serial.print("Distance:");
    Serial.print(distance);
    Serial.println(" cm");*/
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
