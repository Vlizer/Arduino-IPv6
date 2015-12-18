#include "net.h"

#include "ethernet.h"
#include "Arduino.h"

#include "enc28j60.h"
#include "IPv6.h"

#include "debug.h"

uint8_t* Ethernet::buffer;
uint8_t* Ethernet::MAC;

void Ethernet::initialize(uint8_t* buffer, uint8_t* MAC) {
	Ethernet::buffer = buffer;
	Ethernet::MAC = MAC;
	IPv6::initialize(buffer);
}

void Ethernet::cp_mac(uint8_t *to, const uint8_t *from) {
	memcpy(to, from, 6);
}

static void setMACs(const uint8_t* destMAC) {
	Ethernet::cp_mac(Ethernet::buffer + ETH_DST_MAC, destMAC);
	Ethernet::cp_mac(Ethernet::buffer + ETH_SRC_MAC, Ethernet::MAC);
}

static void setTypeLen(uint16_t typelen) {
	Ethernet::buffer[ETH_TYPE_L_P] = (uint8_t) (typelen >> 0);
	Ethernet::buffer[ETH_TYPE_H_P] = (uint8_t) (typelen >> 8);
}

static uint32_t crc_table[] = {
	0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
	0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
	0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
	0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
};

static uint32_t generateCRC(uint16_t length) {
	uint16_t n;
	uint32_t crc = 0;

	for (n = 0; n < length; n++) {
		crc = (crc >> 4) ^ crc_table[(crc ^ (Ethernet::buffer[n] >> 0)) & 0x0F];
		crc = (crc >> 4) ^ crc_table[(crc ^ (Ethernet::buffer[n] >> 4)) & 0x0F];
	}

	return crc;
}

static bool verifyCRC(uint16_t length) {
	uint32_t crc_gen = generateCRC(length);
	uint32_t crc_field = 0;

	crc_field |= Ethernet::buffer[length+3];
	crc_field <<= 8;
	crc_field |= Ethernet::buffer[length+2];
	crc_field <<= 8;
	crc_field |= Ethernet::buffer[length+1];
	crc_field <<= 8;
	crc_field |= Ethernet::buffer[length];

	return crc_gen == crc_field;
}

static void setCRC(uint16_t length) {
	uint32_t crc = generateCRC(length);

	Ethernet::buffer[length]   = crc >> 0;
	Ethernet::buffer[length+1] = crc >> 8;
	Ethernet::buffer[length+2] = crc >> 16;
	Ethernet::buffer[length+3] = crc >> 24;
}

uint16_t Ethernet::packetPrepare(const uint8_t* dest_mac, uint16_t typelen) {
	setMACs(dest_mac);
	setTypeLen(typelen);
	return ETH_HEADER_LEN;
}

void Ethernet::packetSend(uint16_t length) {
	setCRC(length);
	ENC28J60::packetSend(length+ETH_FOOTER_LEN);
}

void Ethernet::packetProcess(uint16_t length) {
	uint16_t typelen;
	typelen = Ethernet::getTypeLen();

#ifdef DEBUG_ETH
	uint8_t* mac = Ethernet::getSrcMAC();

	Serial.print(F("Src MAC: "));
	print_mac_to_serial(mac);
	Serial.println();
	Serial.print(F("Type/length: "));
	Serial.println(typelen, HEX);
	Serial.println(length);
	Serial.println(verifyCRC(length-4) ? F("Verified") : F("Dropped"));
#endif

	if (!verifyCRC(length-4)) return;

	switch (typelen) {
	case ETHTYPE_IPv6_V:
		IPv6::packetProcess(ETH_HEADER_LEN, length - (ETH_HEADER_LEN + ETH_FOOTER_LEN));
		break;
	}
}

uint8_t* Ethernet::getSrcMAC() {
	return Ethernet::buffer + ETH_SRC_MAC;
}

uint16_t Ethernet::getTypeLen() {
	uint16_t out = 0;
	out |= ((uint16_t) Ethernet::buffer[ETH_TYPE_H_P]) << 8;
	out |= ((uint16_t) Ethernet::buffer[ETH_TYPE_L_P]) << 0;
	return out;
}
