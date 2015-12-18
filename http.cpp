#include "net.h"

#include "http.h"
#include "Arduino.h"

#include "tcp.h"

#include "debug.h"

prog_char http_index[] PROGMEM =
		"<html>"
			"<head>"
				"<title>Arduino Board</title>"
			"</head>"
			"<body><h1>404</h1></body>"
		"</html>";
prog_char http_e_404[] PROGMEM =
		"<html>"
			"<head>"
				"<title>Arduino Board</title>"
			"</head>"
			"<body><h1>404</h1></body>"
		"</html>";

prog_char http_newline[] PROGMEM = "\r\n";
prog_char http_200[] PROGMEM = "HTTP/1.1 200 OK\r\n";
prog_char http_404[] PROGMEM = "HTTP/1.1 404 Not Found\r\n";
prog_char http_h_content_type[] PROGMEM = "Content-Type: text/html; charset=UTF-8\r\n";
prog_char http_h_content_length[] PROGMEM = "Content-Length: %i\r\n";

prog_char http_r_index[] PROGMEM = "GET / ";
prog_char http_r_mode_r[] PROGMEM = "GET /mode/";
prog_char http_m_mode_r[] PROGMEM = "GET /mode/%i ";
prog_char http_r_mode_w[] PROGMEM = "POST /mode/";
prog_char http_m_mode_w[] PROGMEM = "POST /mode/%i/%i ";
prog_char http_r_read[] PROGMEM = "GET /pin/";
prog_char http_m_read[] PROGMEM = "GET /pin/%i";
prog_char http_r_write[] PROGMEM = "POST /pin/";
prog_char http_m_write[] PROGMEM = "POST /pin/%i/%i";

char HTTP::buffer[400];

void HTTP::initialize() {
}

#define http_pin_ok(pin) (((pin) > 1 && (pin) < 8) || ((pin) > 13 && ((pin) < 20)))
#define http_pin_analog(pin) ((pin) > 13 && (pin) < 20)

void serve404(struct TCP_handler_args* args);

void serveIndex(struct TCP_handler_args* args) {
	char* s;
	char* buffer = HTTP::buffer;

	buffer[0] = 0;
	strcat_P(buffer, http_200);
	strcat_P(buffer, http_h_content_type);
	s = buffer;
	while (*++s);
	sprintf_P(s, http_h_content_length, strlen_P(http_index));
	strcat_P(buffer, http_newline);
	strcat_P(buffer, http_index);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));
}

void serveModeR(struct TCP_handler_args* args) {
	uint8_t pin = 0;
	if (sscanf_P((char*)args->data, http_m_mode_r, &pin) != 1 || !http_pin_ok(pin)) {
		serve404(args);
		return;
	}

	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg, *out;

	reg = portModeRegister(port);
	out = portOutputRegister(port);

	uint8_t mode;
	if (*reg & bit) {
		mode = OUTPUT;
	} else if (*out & bit) {
		mode = INPUT_PULLUP;
	} else {
		mode = INPUT;
	}

	char* buffer = HTTP::buffer;
	char* s = buffer;
	buffer[0] = 0;
	strcat_P(buffer, http_200);
	strcat_P(buffer, http_h_content_type);
	while (*++s);
	sprintf_P(s, http_h_content_length, 1);
	strcat_P(buffer, http_newline);
	while (*++s);
	sprintf(s, "%i", mode);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));

}

void serveModeW(struct TCP_handler_args* args) {
	uint8_t pin;
	uint8_t mode;
	if (sscanf_P((char*)args->data, http_m_mode_w, &pin, &mode) != 2 || !http_pin_ok(pin) || mode > 2) {
		serve404(args);
		return;
	}

	pinMode(pin, mode);

	char* buffer = HTTP::buffer;
	char* s = buffer;
	buffer[0] = 0;
	strcat_P(buffer, http_200);
	strcat_P(buffer, http_h_content_type);
	while (*++s);
	sprintf_P(s, http_h_content_length, 0);
	strcat_P(buffer, http_newline);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));
}

void serveWrite(struct TCP_handler_args* args) {
	uint8_t pin;
	uint8_t value;
	if (sscanf_P((char*)args->data, http_m_write, &pin, &value) != 2 || !http_pin_ok(pin) || value > 1) {
		serve404(args);
		return;
	}

	digitalWrite(pin, value);

	char* buffer = HTTP::buffer;
	char* s = buffer;
	buffer[0] = 0;
	strcat_P(buffer, http_200);
	strcat_P(buffer, http_h_content_type);
	while (*++s);
	sprintf_P(s, http_h_content_length, 0);
	strcat_P(buffer, http_newline);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));
}

void serveRead(struct TCP_handler_args* args) {
	uint8_t pin;
	if (sscanf_P((char*)args->data, http_m_read, &pin) != 1 || !http_pin_ok(pin)) {
		serve404(args);
		return;
	}

	uint16_t value = http_pin_analog(pin) ? analogRead(pin-14) : digitalRead(pin);

	char* buffer = HTTP::buffer;
	char* s = buffer;
	buffer[0] = 0;
	strcat_P(buffer, http_200);
	strcat_P(buffer, http_h_content_type);
	while (*++s);
	sprintf_P(s, http_h_content_length, 1);
	strcat_P(buffer, http_newline);
	while (*++s);
	sprintf(s, "%i", value);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));
}

void serve404(struct TCP_handler_args* args) {
	char* s;
	char* buffer = HTTP::buffer;

	buffer[0] = 0;
	strcat_P(buffer, http_404);
	strcat_P(buffer, http_h_content_type);
	s = buffer;
	while (*++s);
	sprintf_P(s, http_h_content_length, strlen_P(http_e_404));
	strcat_P(buffer, http_newline);
	strcat_P(buffer, http_e_404);

	TCP::send(args->status, (uint8_t*)buffer, strlen(buffer));
}

void HTTP::handler(struct TCP_handler_args* args) {
#ifdef DEBUG_HTTP
	Serial.println(args->length);
	uint16_t i = 0;
	while (i < args->length) {
		Serial.print((char)args->data[i]);
		i++;
	}
#endif

	if (memcmp_P(args->data, http_r_index, strlen_P(http_r_index)) == 0) {
		Serial.println(F("Index"));
		serveIndex(args);
	} else if (memcmp_P(args->data, http_r_mode_r, strlen_P(http_r_mode_r)) == 0) {
		Serial.println(F("mode r"));
		serveModeR(args);
	} else if (memcmp_P(args->data, http_r_mode_w, strlen_P(http_r_mode_w)) == 0) {
		Serial.println(F("mode w"));
		serveModeW(args);
	} else if (memcmp_P(args->data, http_r_read, strlen_P(http_r_read)) == 0) {
		Serial.println(F("read"));
		serveRead(args);
	} else if (memcmp_P(args->data, http_r_write, strlen_P(http_r_write)) == 0) {
		Serial.println(F("write"));
		serveWrite(args);
	} else {
		Serial.println(F("not found"));
		serve404(args);
	}

}
