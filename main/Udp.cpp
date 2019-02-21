#include "Udp.h"
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>

Udp::Udp(){}

void Udp::setup_udp(char ssid[], char password[], IPAddress local_ip, IPAddress gateway, IPAddress subnet)
{
  WiFi.config(local_ip, gateway, subnet);
  delay(100);

  //WiFi.begin(ssid, password);
  WiFi.begin(ssid);
  delay(100);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  //Serial.println(WiFi.status());
  Serial.println(" connected");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  wifi_udp.begin(local_port); // UDP通信の開始(引数はポート番号)
  Serial.print("Local port: ");
  Serial.println(local_port);
}

void Udp::recieve_packet()
{
  int packet_size = wifi_udp.parsePacket();
  
  if (packet_size)
  {
    //UDP情報の表示
    Serial.print("Received packet of size ");
    Serial.println(packet_size);

    // 実際にパケットを読む
    wifi_udp.read(packet_buffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packet_buffer);
  }
}

String Udp::get_packet_buffer()
{
  String packetStr = String(packet_buffer);
  return packetStr;
}

void Udp::clear_packet_buffer()
{
  int packetBuffer_length = strlen(packet_buffer);
  memset(packet_buffer ,'\0',packetBuffer_length);
}

void Udp::send_data(IPAddress remote_ip, char text[])
{ 
  Serial.println("send...");
  
  wifi_udp.beginPacket(remote_ip, local_port);
  wifi_udp.printf(text);
  wifi_udp.endPacket();
}
void Udp::disconnect() {
  WiFi.disconnect();
}

IPAddress Udp::get_remote_ip() {
  return wifi_udp.remoteIP();
}
