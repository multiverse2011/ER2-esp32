#include "Udp.h"
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>

Udp::Udp(){}

void Udp::setup_udp(char ssid[], char password[], IPAddress local_ip, IPAddress gateway, IPAddress subnet)
{
  WiFi.config(local_ip, gateway, subnet);
  delay(100);

  WiFi.begin(ssid, password);
  delay(100);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println(" connected");
  Serial.println(WiFi.localIP());

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.println("Starting UDP");
  wifi_udp.begin(local_port); // UDP通信の開始(引数はポート番号)
  Serial.print("Local port: ");
  Serial.println(local_port);
}

int Udp::recieve_packet()
{
  int packet_size = wifi_udp.parsePacket();
  
  if (packet_size)
  {
    //UDP情報の表示
    Serial.print("Received packet of size ");
    Serial.println(packet_size);

    // 実際にパケットを読む
    clear_packet_buffer();
    wifi_udp.read(packet_buffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packet_buffer);
  }

  return packet_size;
}

String Udp::get_packet_buffer()
{
  String packetStr = String(packet_buffer);  //substringなど扱いたいのでString型にキャスト
  return packetStr;
}

void Udp::clear_packet_buffer()
{
  memset(packet_buffer ,'\0', sizeof(packet_buffer));
}

void Udp::send_data(char text[])
{ 
  Serial.println("send...");

  wifi_udp.beginPacket(wifi_udp.remoteIP(), local_port);
  wifi_udp.printf(text);
  wifi_udp.endPacket();
}

void Udp::disconnect() {
  WiFi.disconnect();
}