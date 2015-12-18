#ifndef TCP_H
#define TCP_H

#include <Arduino.h>

struct TCP_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint32_t seq_num;
	uint32_t ack_num;
	uint8_t data_offset; // highest 4 bits only, rest reserved
	uint8_t control_bits; // except for two highest bits which are reserved
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_pointer;
	uint8_t options[];
};

struct TCP_status {
	int state;
	uint8_t ip[16];
	uint16_t port;
	uint16_t local_port;
	uint32_t local_num;
	uint32_t remote_num;
	void (*handler)(struct TCP_handler_args*);
};

struct TCP_status_node {
	struct TCP_status node;
	struct TCP_status_node* next;
};

struct TCP_handler_args {
	struct TCP_status* status;
	uint8_t* data;
	uint16_t length;
};

struct TCP_handler {
	uint16_t port;
	void (*handler)(struct TCP_handler_args*);
	struct TCP_handler* next;
};

class TCP {
public:

	static uint8_t* buffer;
	static struct TCP_header* header;
	static void initialize(uint8_t* buffer);

	static void packetProcess(uint8_t* src_ip, uint16_t offset, uint16_t length);

	static void send(TCP_status* status, uint8_t* data, uint16_t length);

	static void registerHandler(uint16_t port, void (*handler)(struct TCP_handler_args* args));
};

#endif
