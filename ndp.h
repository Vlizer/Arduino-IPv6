#ifndef NDP_H
#define NDP_H

#include <Arduino.h>

#define NDP_CACHE_LEN 3

struct ndp_pair {
	uint8_t ip[16];
	uint8_t mac[6];
	uint32_t created;
};

class NDP {
public:
	static uint8_t IP[];
	static struct ndp_pair pairs[];
	static uint8_t* getMAC(uint8_t *ip);
	static void savePairing(uint8_t *ip, uint8_t *mac);

	static void handleAdvertisment(struct ICMPv6_header *header);
	static void handleSolicitation(struct ICMPv6_header *header);
	static void sendAdvertisment(uint8_t *dst_ip, bool solicited, bool override);

};

#endif
