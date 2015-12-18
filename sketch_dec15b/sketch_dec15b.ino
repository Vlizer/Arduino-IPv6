#include <debug.h>
#include <enc28j60.h>
#include <ethernet.h>
#include <ICMPv6.h>
#include <IPv6.h>
#include <ndp.h>
#include <net.h>
byte ENC28J60::buffer[500];

uint8_t ENC28J60::MAC[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
//fe80::80ff
uint8_t NDP::IP[] = { 0xFE, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0xFF};

void setup()   {
	ENC28J60::initialize(sizeof ENC28J60::buffer, ENC28J60::MAC);
	ENC28J60::disableMulticast();
	Serial.begin(9600);
	Ethernet::initialize(ENC28J60::buffer, ENC28J60::MAC);
	randomSeed(42);
}


void loop()
{
	uint16_t len;

	do {
		len = ENC28J60::packetReceive();
	} while (len == 0);

	Ethernet::packetProcess(len);
}
