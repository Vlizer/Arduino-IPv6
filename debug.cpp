#include "Arduino.h"

#include "debug.h"

void print_ip_to_serial(const uint8_t *ip) {
	char sep = 0;
	int i;
	for (i = 0; i < 16; i += 2) {
		if (sep != 0) Serial.print(sep);
		Serial.print(ip[i],   16);
		Serial.print(ip[i+1], 16);
		sep = ':';
	}
}

void print_mac_to_serial(const uint8_t *mac) {
	int i;
	char sep = 0;

	for (i = 0; i < 6; i++) {
		if (sep != 0) Serial.print(sep);
		Serial.print(mac[i], HEX);
		sep = ':';
	}
}
