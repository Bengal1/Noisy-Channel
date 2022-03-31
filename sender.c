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
#define P1 0b1
#define P2 0b10
#define P3 0b100
#define P4 0b1000
#define P5 0b10000


int power2(int x)
{
	int res = 1;

	if (x < 0) {
		printf("error:negative exponent!\n");
		return -1;
	}

	for (int i = 0; i < x; i++) {
		res *= 2;
	}
	return res;
}

char* lowercase(char* str)
{
	int str_size = strlen(str);
	char* str_before = str, * str_after = NULL;

	if ((str_after = (char*)calloc(1, (str_size + 1) * sizeof(char))) == NULL) {
		printf("Allocation Failed\n");
		exit(EXIT_FAILURE);
	}

	if (str != NULL) {
		for (int i = 0; i < str_size; i++) {
			(*(str_after + i)) = tolower((*(str_before + i)));
		}
	}
	else {
		printf("Attempt to operate on NULL pointer\n");
		return NULL;
	}
	return str_after;
}

bool parity_bit_check(int type, unsigned int number)
{
	int on_bits = 0;
	unsigned int num = number;

	switch (type) {
	case 0: { //p0 - all bits parity - check for 2 errors
		while (num > 0) {
			if (num & 0b1) {
				on_bits++;
			}
			num = num >> 1;
		}
		break;
	}
	case 1: { // p1
		num = num >> 1;
		while (num > 0) {
			if (num & 0b1) {
				on_bits++;
			}
			num = num >> 2;
		}
		break;
	}
	case 2: { //p2
		num = num >> 2;
		while (num > 0) {
			for (int i = 0; i < 2; i++) {
				if (num & 0b1) {
					on_bits++;
				}
				num = num >> 1;
			}
			num = num >> 2;
		}
		break;
	}
	case 3: { //p3
		num = num >> 4;
		while (num > 0) {
			for (int i = 0; i < 4; i++) {
				if (num & 0b1) {
					on_bits++;
				}
				num = num >> 1;
			}
			num = num >> 4;
		}
		break;
	}
	case 4: { //p4
		num = num >> 8;
		while (num > 0) {
			for (int i = 0; i < 8; i++) {
				if (num & 0b1) {
					on_bits++;
				}
				num = num >> 1;
			}
			num = num >> 8;
		}
		break;
	}
	case 5: { //p5
		num = num >> 16;
		while (num > 0) {
			if (num & 0b1) {
				on_bits++;
			}
			num = num >> 1;
		}
		break;
	}
	}
	if (on_bits % 2 == 0) {
		return true;
	}
	else {
		return false;
	}
}


unsigned int hamming_encode(unsigned int mes)
{
	int j = 0, i = 0;
	unsigned int res = 0, message = mes;
	unsigned int bits[DATA_LEN] = { 0 };

	while (message > 0) { //extract data
		if (message & 0b1) {
			bits[i] = 1;
		}
		message = message >> 1;
		i++;
	}

	for (int i = 3; i < FOUR_BYTES; i++) { //set data in hamming figure
		if ((i == P3) || (i == P4) || (i == P5)) {
			continue;
		}
		res = res + ((bits[j])*power2(i));
		j++;
	}

	for (j = 1; j < 6; j++) {  // set parity bits
		if (parity_bit_check(j, res) == true)
			continue;
		else
			res = res ^ power2((power2(j - 1)));
	}
	if (parity_bit_check(0, res) == false) // parity zero (general parity)
		res = res ^ 0b1;

	return res;
}

void resources_release(SOCKET socket, FILE* fp, bool end)
{
	SOCKET* s = &socket;

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	if (end) {
		shutdown(socket, 2);
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
	FILE* fptr = NULL, *fptr2 = NULL;
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
		if (0 == strcmp(lowercase(file_name), "quit")) {
			printf("Quiting...\n");
			resources_release(sock, NULL, true);
			return EXIT_SUCCESS;
		}
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("socket function failed with error: %d\n", WSAGetLastError());
			resources_release(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((connect(sock, (struct sockaddr*)&sock_info, sizeof(sock_info))) == SOCKET_ERROR) {
			printf("connect() failed with error: %d\n", WSAGetLastError());
			resources_release(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if (recv(sock, ready, BUFFER_LEN, 0) <= 0) { // wait for receiver to connect
			printf("recv() failed with error: %d\n", WSAGetLastError());
			resources_release(sock, NULL, true);
			return EXIT_FAILURE;
		}
		if ((fptr = fopen(file_name, "rb")) == NULL) {
			printf("Failed to open file stream\n");
			resources_release(sock, NULL, true);
			return EXIT_FAILURE;
		}
		else if (0 != strcmp(ready, "OK")) {
			printf("channel failed to connect sender\n");
			resources_release(sock, fptr, true);
			return EXIT_SUCCESS; // sender.exe did'nt fail
		}

		while (1) {
			message_len = fread(&buffer, 1U, BUFFER_LEN*sizeof(char), fptr);
			if (feof(fptr))
				break;
			fseek(fptr, -1L, SEEK_CUR);
			message = hamming_encode(buffer & MASK);  // hamming (31,26,3)
			send(sock, (char*)&message, BUFFER_LEN, 0);
			buffer = 0;
			data_sent++;
		}
		// last message
		message = hamming_encode(buffer & MASK);  // hamming (31,26,3)
		send(sock, (char*)&message, BUFFER_LEN, 0);

		fseek(fptr, 0, SEEK_END);
		file_len = ftell(fptr);
		data_sent++;
		data_sent *= BUFFER_LEN;

		printf("file length: %d bytes\nsent: %d bytes\n", file_len, data_sent);
		
		data_sent = 0, file_len = 0;
		resources_release(sock, fptr, false);
	}

	return EXIT_SUCCESS;
}

