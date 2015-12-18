#ifndef ETH_H
#define ETH_H

#ifndef __PROG_TYPES_COMPAT__
  #define __PROG_TYPES_COMPAT__
#endif

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#define WRITE_RESULT size_t
#define WRITE_RETURN return 1;
#else
#include <WProgram.h> // Arduino 0022
#define WRITE_RESULT void
#define WRITE_RETURN
#endif

#include <avr/pgmspace.h>


class Ethernet {
public:
	static uint8_t* buffer;
	static uint8_t* MAC;
	static void initialize(uint8_t* buffer, uint8_t* MAC);
	static uint16_t packetPrepare(const uint8_t* dest_mac, uint16_t typelen);
	static void packetSend(uint16_t length);
	static void packetProcess(uint16_t length);

	static uint8_t* getSrcMAC();

	static uint16_t getTypeLen();

	static void cp_mac(uint8_t *to, const uint8_t *from);
};

#endif
