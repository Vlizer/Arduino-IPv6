#ifndef NET_H
#define NET_H

// ******* ETH *******
#define ETH_HEADER_LEN 14
#define ETH_FOOTER_LEN 4
// values of certain bytes:
#define ETHTYPE_IPv6_H_V 0x86
#define ETHTYPE_IPv6_L_V 0xdd
#define ETHTYPE_IPv6_V 0x86dd
// byte positions in the ethernet frame:
//
// Ethernet type field (2bytes):
#define ETH_TYPE_H_P 12
#define ETH_TYPE_L_P 13
//
#define ETH_DST_MAC 0
#define ETH_SRC_MAC 6

// ******* IPv6 ********
#define IPv6_HEADER_LEN 40

#define IPv6_VERSION 0
#define IPv6_VERSION_VAL 0x06

#define IPv6_PAYLOAD 4
#define IPv6_NEXT 6
#define IPv6_HOP_LIMIT 7
#define IPv6_SRC_ADDR 8
#define IPv6_DST_ADDR 24

// ******** ICMPv6 **********
#define ICMPv6_NEXT_HEADER 0x3a

#define ICMPv6_PING_REQUEST 128
#define ICMPv6_PING_REPLY 129
#define ICMPv6_NBR_SOLICIT 135
#define ICMPv6_NBR_ADVERT 136

#define SWAP_16_H_L(val) ((uint16_t) (val) >> 8 | (uint16_t) (val) << 8)
#define SWAP_32(val) (((val) >> 24) | (((val) & 0xFF0000) >> 8) | (((val) & 0xFF00) << 8) | ((val) << 24))
#define BIG_ENDIAN_JOIN(val1, val2) ((uint16_t) (val1) << 8 | (uint16_t) (val2))

#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#endif
