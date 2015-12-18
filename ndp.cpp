#include "net.h"

#include "ndp.h"
#include "Arduino.h"

#include "ICMPv6.h"
#include "IPv6.h"
#include "ethernet.h"

#include "debug.h"

static uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

struct ndp_pair NDP::pairs[NDP_CACHE_LEN];

uint8_t* NDP::getMAC(uint8_t *ip) {
	int i;
	for (i = 0; i < NDP_CACHE_LEN; i++) {
		if(IPv6::cmp_ip(ip, NDP::pairs[i].ip) == 0) {
			return NDP::pairs[i].mac;
		}
	}
	return 0;
}

void NDP::handleAdvertisment(struct ICMPv6_header *header) {
	uint8_t *src_ip = header->body;
	uint8_t *src_mac = header->body + 16 + 2;

	NDP::savePairing(src_ip, src_mac);
}

void NDP::handleSolicitation(struct ICMPv6_header *header) {
#ifdef D_NDP
	Serial.print(F("Target Address: "));
	print_ip_to_serial(header->body);
	Serial.println();

	Serial.print(F("My Address: "));
	print_ip_to_serial(NDP::IP);
	Serial.println();
#endif

	if (IPv6::cmp_ip(header->body, NDP::IP) != 0) {
		return;
	}

	NDP::sendAdvertisment(IPv6::header->src_ip, true, false);
}

void NDP::sendAdvertisment(uint8_t *dst_ip, bool solicited, bool override) {
	uint16_t offset = IPv6::packetPrepare(dst_ip, ICMPv6_NEXT_HEADER, 32);
	struct ICMPv6_header *header = (struct ICMPv6_header *) (ICMPv6::buffer + offset);
	memset(header, 0, 32);
	header->type = ICMPv6_NBR_ADVERT;
	if (solicited) {
		header->reserved |= 1 << 6;
	}
	if (override) {
		header->reserved |= 1 << 5;
	}
	IPv6::cp_ip(header->body, NDP::IP);
	// set option
	*(header->body + 16 + 0) = 2; // type
	*(header->body + 16 + 1) = 1; // length in 8-octets
	Ethernet::cp_mac(header->body + 16 + 2, Ethernet::MAC);

	uint16_t tmp = IPv6::generateChecksum(0);
	header->checksum = SWAP_16_H_L(tmp);
  ,
	IPv6::packetSend(offset + 32);
}



void NDP::savePairing(uint8_t *ip, uint8_t *mac) {
	uint8_t i = 0;
	uint32_t best = 0;
	uint8_t best_i = 0;

	while (i < NDP_CACHE_LEN) {
		if (IPv6::cmp_ip(ip, NDP::pairs[i].ip) == 0) {
			memcpy(NDP::pairs[i].mac, mac, 6);
			NDP::pairs[i].created = millis();
			break;
		}
		if (NDP::pairs[i].created == 0) {
			memcpy(NDP::pairs[i].ip, ip, 16);
			memcpy(NDP::pairs[i].mac, mac, 6);
			NDP::pairs[i].created = millis();
			break;
		}
		if (best == 0 || NDP::pairs[i].created < best) {
			best = NDP::pairs[i].created;
			best_i = i;
		}
		i++;
	}

	if (i == NDP_CACHE_LEN) {
		memcpy(NDP::pairs[best_i].ip, ip, 16);
		memcpy(NDP::pairs[best_i].mac, mac, 6);
		NDP::pairs[best_i].created = millis();
	}

#ifdef D_NDP
	for (i = 0; i < NDP_CACHE_LEN; i++) {
		if (NDP::pairs[i].created == 0) continue;
		Serial.print(NDP::pairs[i].created);
		Serial.print(F(": "));
		print_ip_to_serial(NDP::pairs[i].ip);
		Serial.print(F(" -> "));
		print_mac_to_serial(NDP::pairs[i].mac);
		Serial.println();
	}
#endif
}
