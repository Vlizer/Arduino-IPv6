#ifndef ICMPv6_H
#define ICMPv6_H

#include <Arduino.h>

struct ICMPv6_header {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint32_t reserved;
	uint8_t body[];
};

class ICMPv6 {
public:
	static uint8_t* buffer;
	static uint8_t* address;
	static void initialize(uint8_t* buffer);

	static void packetProcess(uint16_t offset, uint16_t length);
	
	static void handlePingRequest(struct ICMPv6_header *header);

};

#endif
