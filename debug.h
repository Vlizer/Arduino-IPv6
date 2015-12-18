#ifndef DEBUG_H
#define DEBUG_H

//#define D_ETH
//#define D_IP
//#define D_ICMP
//#define D_NDP

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022
#endif
void print_ip_to_serial(const uint8_t* ip);

void print_mac_to_serial(const uint8_t* mac);

#endif
