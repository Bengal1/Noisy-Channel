#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 4
#define IP_SIZE 16
#define MAX_NAME 200
#define MASK 0x03FFFFFF
#define DATA_LEN 26
#define FOUR_BYTES 32
#define FAILED -1
#define P1 0b1
#define P2 0b10
#define P3 0b100
#define P4 0b1000
#define P5 0b10000



int powerOfTwo(int x)
{
	if (x < 0) {
		printf("Error: Negative exponent is not allowed!\n");
		return FAILED;
	}
	return 1 << x;
}

char* toLowerCase(char* str)
{
	if (str == NULL) { 
		printf("Attempt to operate on NULL pointer\n");
		return NULL;
	}
	
	int str_size = strlen(str);

	for (int i = 0; i < str_size; i++) {
		str[i] = tolower(str[i]);
	}
	return str;
}

bool parityBitCheck(int type, unsigned int number)
{
	int on_bits = 0;
	unsigned int num = number;
	int shift = (type == 0) ? 1 : (1 << (type - 1)); // Determine skip pattern

	if (type == 0) { // Special case - count all bits
		while (num) {
			on_bits += (num & 1);
			num >>= 1;
		}
	}
	else {
		num >>= shift; // Skip initial bits
		while (num) {
			for (int i = 0; i < shift && num; i++) {
				on_bits += (num & 1);
				num >>= 1;
			}
			num >>= shift;
		}
	}

	return (on_bits % 2 == 0);
}

unsigned int hammingEncode(unsigned int mes)
{
	int j = 0, i = 0;
	unsigned int res = 0, message = mes;
	unsigned int bits[DATA_LEN] = { 0 };

	while (message > 0) { //extract data
		if (message & P1) {
			bits[i] = 1;
		}
		message = message >> 1;
		i++;
	}

	for (int i = 3; i < FOUR_BYTES; i++) { //set data in hamming figure
		if ((i == P3) || (i == P4) || (i == P5)) {
			continue;
		}
		res = res + ((bits[j]) * powerOfTwo(i));
		j++;
	}

	for (j = 1; j < 6; j++) {  // set parity bits
		if (parityBitCheck(j, res) == true)
			continue;
		else
			res = res ^ powerOfTwo((powerOfTwo(j - 1)));
	}
	if (parityBitCheck(0, res) == false) // parity zero
		res = res ^ P1;

	return res;
}

void resourcesRelease(SOCKET socket, FILE* fp, bool end)
{
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	if (end) {
		shutdown(socket, SD_BOTH);
		WSACleanup();
		return;
	}
	if (socket != INVALID_SOCKET) {
		closesocket(socket);
	}

	return;
}

int main(int argc, char* argv[])
{
	int data_sent = 0, message_len = 0, file_len = 0;
	unsigned int buffer = 0, message = 0;
	unsigned short port_number = 0;
	char ip_number[IP_SIZE] = { '\0' }, file_name[MAX_NAME] = { '\0' }, ready[BUFFER_LEN] = { '\0' };
	FILE* fptr = NULL;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in sock_info;

	strcpy(ip_number, argv[1]);
	port_number = atoi(argv[2]);

	// Initialize Winsock
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with Error Code : %d\n", iResult);
		return EXIT_FAILURE;
	}

	sock_info.sin_family = AF_INET;
	sock_info.sin_addr.s_addr = inet_addr(ip_number);
	sock_info.sin_port = htons(port_number);

	while (1) { // sender routine
		printf("enter file name:\n");
		gets(file_name);
		if (0 == strcmp(toLowerCase(file_name), "quit")) {
			printf("Quiting...\n");
			resourcesRelease(sock, NULL, true);
			return EXIT_SUCCESS;
		}
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("socket function failed with error: %d\n", WSAGetLastError());
			resourcesRelease(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((connect(sock, (struct sockaddr*)&sock_info, sizeof(sock_info))) == SOCKET_ERROR) {
			printf("connect() failed with error: %d\n", WSAGetLastError());
			resourcesRelease(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if (recv(sock, ready, BUFFER_LEN, 0) <= 0) { // wait for receiver to connect
			printf("recv() failed with error: %d\n", WSAGetLastError());
			resourcesRelease(sock, NULL, true);
			return EXIT_FAILURE;
		}
		if ((fptr = fopen(file_name, "rb")) == NULL) {
			printf("Failed to open file stream\n");
			resourcesRelease(sock, NULL, true);
			return EXIT_FAILURE;
		}
		else if (0 != strcmp(ready, "OK")) {
			printf("channel failed to connect sender\n");
			resourcesRelease(sock, fptr, true);
			return EXIT_SUCCESS; // sender.exe did'nt fail
		}

		while (1) {
			message_len = fread(&buffer, 1U, BUFFER_LEN * sizeof(char), fptr);
			if (feof(fptr))
				break;
			fseek(fptr, -1L, SEEK_CUR);
			message = hammingEncode(buffer & MASK);  // hamming (31,26,3)
			send(sock, (char*)&message, BUFFER_LEN, 0);
			buffer = 0;
			data_sent++;
		}
		// last message
		message = hammingEncode(buffer & MASK);  // hamming (31,26,3)
		send(sock, (char*)&message, BUFFER_LEN, 0);

		fseek(fptr, 0, SEEK_END);
		file_len = ftell(fptr);
		data_sent++;
		data_sent *= BUFFER_LEN;

		printf("file length: %d bytes\nsent: %d bytes\n", file_len, data_sent);

		data_sent = 0, file_len = 0;
		resourcesRelease(sock, fptr, false);
	}

	return EXIT_SUCCESS;
}

