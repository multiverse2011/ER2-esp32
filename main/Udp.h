#ifndef _Udp_h
#define _Udp_h

#include "Arduino.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define UDP_TX_PACKET_MAX_SIZE 500

class Udp
{
  private:
    // wifiの設定

    WiFiUDP wifi_udp;
    int local_port = 10000; // ポート番号
    char packet_buffer[UDP_TX_PACKET_MAX_SIZE] = {};

  public:
    Udp();
    void setup_udp(char ssid[], char password[], IPAddress local_ip, IPAddress gateway, IPAddress subnet);
    int recieve_packet();
    void send_data(char text[]);
    String get_packet_buffer();
    void clear_packet_buffer();
    void disconnect();
};

#endif
