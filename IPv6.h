#ifndef IPv6_H
#define IPv6_H

#include <Arduino.h>

struct IPv6_header {
	uint32_t first4octets;
	uint16_t payload_length;
	uint8_t next_header;
	uint8_t hop_limit;
	uint8_t src_ip[16];
	uint8_t dst_ip[16];
};

class IPv6 {
public:

	static uint8_t* buffer;
	static uint16_t* address;
	static struct IPv6_header* header;

	static void initialize(uint8_t* buffer);
	static void packetProcess(uint16_t offset, uint16_t length);
	static uint16_t generateChecksum(uint16_t correction);
	static uint16_t packetPrepare(uint8_t *dst_ip, uint8_t next_header, uint16_t length);
	static void packetSend(uint16_t length);

	static bool filter(uint8_t *ip);
	static void cp_ip(uint8_t *to, const uint8_t *from);
	static int cmp_ip(uint8_t *ip1, uint8_t *ip2);
};

#endif
