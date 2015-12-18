#include "net.h"

#include "tcp.h"
#include "Arduino.h"

#include "IPv6.h"
#include "ndp.h"

#include "debug.h"

uint8_t* TCP::buffer;
struct TCP_header* TCP::header;

void TCP::initialize(uint8_t* buffer) {
	TCP::buffer = buffer;
}

struct TCP_status_node* head = 0;

struct TCP_status* getStatus(uint8_t* ip, uint16_t port) {
	struct TCP_status_node* curr = head;
	while (curr != 0) {
		if (curr->node.port == port && IPv6::cmp_ip(curr->node.ip, ip) == 0) {
			return (struct TCP_status*) curr;
		}
		curr = curr->next;
	}
	return 0;
}

struct TCP_status* addStatus(uint8_t* ip, uint16_t port, uint16_t local_port) {
	if (getStatus(ip, port) != 0) {
		return 0;
	}

	struct TCP_status_node* node = (struct TCP_status_node*)malloc(sizeof(struct TCP_status_node));
	memset(node, 0, sizeof(struct TCP_status_node));
	node->node.state = TCP_LISTEN;
	memcpy(node->node.ip, ip, 16);
	node->node.port = port;
	node->node.local_port = local_port;
	if (head == 0) {
		head = node;
	} else {
		node->next = head;
		head = node;
	}
	/*
	struct TCP_status_node* curr = head;
	while(curr != 0) {
		print_ip_to_serial(curr->node.ip);
		Serial.print(".");
		Serial.println(curr->node.port);
		curr = curr->next;
	}
	*/
	return (struct TCP_status*)node;
}

void removeStatus(uint8_t* ip, uint16_t port) {
	struct TCP_status_node* curr = head;
	struct TCP_status_node* prev = 0;
	while (curr != 0) {
		if (curr->node.port == port && IPv6::cmp_ip(curr->node.ip, ip) == 0) {
			if (prev != 0) {
				prev->next = curr->next;
			} else {
				head = curr->next;
			}
			free(curr);
			break;
		}
		prev = curr;
		curr = curr->next;
	}
}

struct TCP_handler* handlers = 0;

void (*getHandler(uint16_t port))(struct TCP_handler_args*) {
	struct TCP_handler* curr = handlers;
	while (curr != 0) {
		if (curr->port == port) {
			return curr->handler;
		}
		curr = curr->next;
	}
	return 0;
}

void TCP::registerHandler(uint16_t port, void (*handler)(struct TCP_handler_args*)) {
	if (getHandler(port)) {
		return;
	}
	struct TCP_handler* node = (struct TCP_handler*)malloc(sizeof(struct TCP_handler));
	node->port = port;
	node->handler = *handler;
	node->next = handlers;
	handlers = node;
}

void TCP::packetProcess(uint8_t* src_ip, uint16_t offset, uint16_t length) {
	TCP::header = (struct TCP_header*) (TCP::buffer + offset);

#ifdef DEBUG_TCP
	Serial.print(F("Src IP: "));
	print_ip_to_serial(src_ip);
	Serial.println();

	Serial.print(F("Src port: "));
	Serial.println(SWAP_16_H_L(TCP::header->src_port));

	Serial.print(F("Dest port: "));
	Serial.println(SWAP_16_H_L(TCP::header->dst_port));

	Serial.print(F("Seq: 0x"));
	Serial.println(SWAP_32(TCP::header->seq_num), 16);

	Serial.print(F("Ack: 0x"));
	Serial.println(SWAP_32(TCP::header->ack_num), 16);
#endif

	struct TCP_status* status = getStatus(src_ip, SWAP_16_H_L(TCP::header->src_port));

	uint16_t len;

	if (status == 0) {
		status = addStatus(src_ip, SWAP_16_H_L(TCP::header->src_port), SWAP_16_H_L(TCP::header->dst_port));
	}

	switch (status->state) {
	case TCP_LISTEN:
		status->handler = getHandler(status->local_port);
		if (!(TCP::header->control_bits & (1 << 1)) || // SYN
			status->handler == 0) {
			removeStatus(status->ip, status->port);
			break;
		}
		status->local_num = random(0xFFFF);
		status->remote_num = SWAP_32(TCP::header->seq_num)+1;
		len = sizeof(struct TCP_header) + 4;
		IPv6::packetPrepare(src_ip, TCP_NEXT_HEADER, len);
		TCP::header->src_port = SWAP_16_H_L(status->local_port);
		TCP::header->dst_port = SWAP_16_H_L(status->port);
		TCP::header->seq_num = SWAP_32(status->local_num);
		TCP::header->ack_num = SWAP_32(status->remote_num);
		TCP::header->data_offset = 0xF0 & (6 << 4); // 1 option
		TCP::header->control_bits = 0x3F & (1 << 1 | 1 << 4); // SYN ACK
		TCP::header->window = SWAP_16_H_L(400); // reduce window size
		TCP::header->checksum = 0;
		TCP::header->urgent_pointer = 0;
		// Max Seg Size
		TCP::header->options[0] = 2;
		TCP::header->options[1] = 4;
		TCP::header->options[2] = (0xFF00 & 400) >> 8;
		TCP::header->options[3] = (0x00FF & 400) >> 0;
		TCP::header->checksum = IPv6::generateChecksum(0);
		TCP::header->checksum = SWAP_16_H_L(TCP::header->checksum);
		IPv6::packetSend(offset+len);
		status->state = TCP_SYN_RECEIVED;
		break;
	case TCP_SYN_RECEIVED:
		if (!(TCP::header->control_bits & (1 << 4)) || // ACK
			SWAP_32(TCP::header->ack_num) != status->local_num + 1) {
			removeStatus(status->ip, status->port);
			break;
		}

		status->state = TCP_ESTABLISHED;
		status->local_num++;
		break;
	case TCP_ESTABLISHED:
		if (TCP::header->control_bits & (1 << 4)) { // ACK
			if (SWAP_32(TCP::header->ack_num) != status->local_num) {
				break;
			}
		}

		if (TCP::header->control_bits & (1 << 0)) { // FIN
			status->remote_num++;

			len = sizeof(struct TCP_header);
			IPv6::packetPrepare(src_ip, TCP_NEXT_HEADER, len);
			TCP::header->src_port = SWAP_16_H_L(status->local_port);
			TCP::header->dst_port = SWAP_16_H_L(status->port);
			TCP::header->seq_num = SWAP_32(status->local_num);
			TCP::header->ack_num = SWAP_32(status->remote_num);
			TCP::header->data_offset = 0xF0 & (5 << 4); // 0 options
			TCP::header->control_bits = 0x3F & (1 << 4); // ACK
			TCP::header->window = SWAP_16_H_L(400); // reduce window size
			TCP::header->checksum = 0;
			TCP::header->urgent_pointer = 0;
			TCP::header->checksum = IPv6::generateChecksum(0);
			TCP::header->checksum = SWAP_16_H_L(TCP::header->checksum);
			IPv6::packetSend(offset+len);

			status->state = TCP_CLOSE_WAIT;

			TCP::header->control_bits = 0x3F & (1 << 0 | 1 << 4); // FIN ACK
			TCP::header->checksum = 0;
			TCP::header->checksum = IPv6::generateChecksum(0);
			TCP::header->checksum = SWAP_16_H_L(TCP::header->checksum);
			IPv6::packetSend(offset+len);

			status->state = TCP_LAST_ACK;
			break;
		}

		if (TCP::header->control_bits & (1 << 3)) { // PSH
			uint16_t data_offset = 4 * (TCP::header->data_offset >> 4);
			status->remote_num += length - data_offset;

			uint16_t tmp_len = length - data_offset;
			//uint8_t* tmp_buf = (uint8_t*)malloc(tmp_len);
			//memcpy(tmp_buf, ((uint8_t*)TCP::header) + data_offset, tmp_len);

			//len = sizeof(struct TCP_header);
			/*IPv6::packetPrepare(src_ip, TCP_NEXT_HEADER, len);
			TCP::header->src_port = SWAP_16_H_L(status->local_port);
			TCP::header->dst_port = SWAP_16_H_L(status->port);
			TCP::header->seq_num = SWAP_32(status->local_num);
			TCP::header->ack_num = SWAP_32(status->remote_num);
			TCP::header->data_offset = 0xF0 & (5 << 4); // 0 options
			TCP::header->control_bits = 0x3F & (1 << 4); // ACK
			TCP::header->window = SWAP_16_H_L(400); // reduce window size
			TCP::header->checksum = 0;
			TCP::header->urgent_pointer = 0;
			TCP::header->checksum = IPv6::generateChecksum(0);
			TCP::header->checksum = SWAP_16_H_L(TCP::header->checksum);
			IPv6::packetSend(offset+len);*/

			struct TCP_handler_args args = {status, ((uint8_t*)TCP::header) + data_offset, tmp_len};
			status->handler(&args);
			//free(tmp_buf);
		}

		break;
	case TCP_LAST_ACK:
		removeStatus(status->ip, status->port);
		break;
	}
}

void TCP::send(struct TCP_status* status, uint8_t* data, uint16_t length) {
	if (status->state != TCP_ESTABLISHED) {
		return;
	}

	memmove(TCP::header->options, data, length);

	uint16_t len = sizeof(struct TCP_header) + length;
	uint16_t offset = IPv6::packetPrepare(status->ip, TCP_NEXT_HEADER, len);
	TCP::header = (struct TCP_header*) (TCP::buffer + offset);
	TCP::header->src_port = SWAP_16_H_L(status->local_port);
	TCP::header->dst_port = SWAP_16_H_L(status->port);
	TCP::header->seq_num = SWAP_32(status->local_num);
	TCP::header->ack_num = SWAP_32(status->remote_num);
	TCP::header->data_offset = 0xF0 & (5 << 4); // 0 options
	TCP::header->control_bits = 0x3F & (1 << 4 | 1 << 3); // ACK PSH
	TCP::header->window = SWAP_16_H_L(400); // reduce window size
	TCP::header->checksum = 0;
	TCP::header->urgent_pointer = 0;
	TCP::header->checksum = IPv6::generateChecksum(0);
	TCP::header->checksum = SWAP_16_H_L(TCP::header->checksum);
	IPv6::packetSend(offset+len);

	status->local_num += length;
}
