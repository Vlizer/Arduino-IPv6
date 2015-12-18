#include "net.h"

#include "ICMPv6.h"
#include "Arduino.h"

#include "IPv6.h"
#include "ndp.h"

uint8_t* ICMPv6::buffer;

void ICMPv6::initialize(uint8_t* buffer) {
	ICMPv6::buffer = buffer;
}

void ICMPv6::packetProcess(uint16_t offset, uint16_t length) {
	struct ICMPv6_header *header = (struct ICMPv6_header*) (ICMPv6::buffer+offset);
#ifdef D_ICMP
	Serial.print("ICMPv6 Type: ");
	Serial.println(header->type);

	Serial.print("Code: ");
	Serial.println(header->code);

	Serial.print("Type: ");
	Serial.println(header->type);

	Serial.print("Checksum:           0x");
	Serial.println(SWAP_16_H_L(header->checksum), 16);

	Serial.print("Generated checksum: 0x");
	Serial.println(IPv6::generateChecksum(header->checksum), 16);
#endif
	switch (header->type) {
	// Neighbour Advertisment
	case ICMPv6_NBR_ADVERT:
		NDP::handleAdvertisment(header);
		break;

	// Neighbour Solicitation
	case ICMPv6_NBR_SOLICIT:
		NDP::handleSolicitation(header);
		break;

	// Ping Request
	case ICMPv6_PING_REQUEST:
		ICMPv6::handlePingRequest(header);
		break;
	}

}

void ICMPv6::handlePingRequest(struct ICMPv6_header *header) {
	header->checksum = 0;
	header->type = ICMPv6_PING_REPLY;

	uint16_t tmp = IPv6::header->payload_length;
	uint16_t offset = IPv6::packetPrepare(IPv6::header->src_ip, ICMPv6_NEXT_HEADER, SWAP_16_H_L(tmp));
	offset += SWAP_16_H_L(tmp);

	tmp = IPv6::generateChecksum(0);
	header->checksum = SWAP_16_H_L(tmp);

	IPv6::packetSend(offset);
}

