#include <ESP8266WiFi.h>
#include "setting.h"

void connectWifi()
{
  const char *ssid = SSID;
  const char *password = PSK;
  delay(100);
  Serial.printf("connecting to %s...\r\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi connection failed");
    Serial.println("ESP8266 restart after 10 seconds...");
    delay(10000);
    Serial.println("restart");
  }
  Serial.println("WiFi connected");
  Serial.println("changing to static IP address...");
  WiFi.config(IPAddress(192, 168, 0, 30), WiFi.gatewayIP(), WiFi.subnetMask());
  delay(100);
  Serial.println("changed to static IP address");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}