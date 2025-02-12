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

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 4
#define IP_SIZE 16
#define MAX_NAME 200
#define FOUR_BYTES 32
#define FAILED -1
#define P1 0b1
#define P2 0b10
#define P3 0b100
#define P4 0b1000
#define P5 0b10000
#define MASK 0x03FFFFFF


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

void resourcesRelease(SOCKET socket, FILE* fp, bool end)
{
	SOCKET* s = &socket;

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
		(*s) = INVALID_SOCKET;
	}

	return;
}

bool parityBitCheck(int type, unsigned int number)
{
	int on_bits = 0;
	unsigned int num = number;
	int shift = (type == 0) ? 1 : (1 << (type - 1)); // Determine skip pattern

	if (type == 0) { // Special case: count all bits
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
			num >>= shift; // Skip bits
		}
	}

	return (on_bits % 2 == 0);
}


unsigned int hammingDecode(char* mes, int* errors)
{
	int j = 0, error_pose = 0;
	unsigned int res = 0, message = 0, * imessage = NULL;
	int par_checks[5] = { 0 };

	imessage = (unsigned int*)mes;
	message = *imessage;
	imessage = &message;

	//error detection & correction
	for (int i = 1; i < 6; i++) {
		if ((parityBitCheck(i, message)) == false) {
			error_pose += powerOfTwo(i - 1);
		}
	}
	if ((parityBitCheck(0, message) == false) && (error_pose == 0)) {
		message = message ^ P1;
		(*errors)++;
	}
	if (error_pose > 0) {
		message = message ^ (powerOfTwo(error_pose)); //1-bit error correct
		(*errors)++;
	}

	//extract data
	for (int i = 3; i < FOUR_BYTES; i++) {
		if ((i == 4) || (i == 8) || (i == 16)) {
			continue;
		}
		else {
			if (powerOfTwo(i) & message) {
				res += powerOfTwo(j);
			}
			j++;
		}
	}
	res = res & MASK;

	return res;
}

int main(int argc, char* argv[])
{
	int bytes_recv = 1, data_recv = 0, file_len = 0, errors = 0, * perrors = NULL;
	unsigned int w_buffer = 0, h_buffer = 0;
	unsigned short port_number = 0;
	char message[BUFFER_LEN] = { '\0' }, ip_number[IP_SIZE] = { '\0' }, file_name[MAX_NAME] = { '\0' };
	FILE* fptr = NULL;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in sock_info;

	strcpy(ip_number, argv[1]);
	port_number = atoi(argv[2]);
	perrors = &errors;

	// Initialize Winsock
	WSADATA wsaData;
	int iResult, ports[2];

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with Error Code : %d\n", iResult);
		return EXIT_FAILURE;
	}

	sock_info.sin_family = AF_INET;
	sock_info.sin_addr.s_addr = inet_addr(ip_number);
	sock_info.sin_port = htons(port_number);

	while (1) { // receiver routine
		printf("enter file name:\n");
		gets(file_name);
		if (0 == strcmp(toLowerCase(file_name), "quit")) {
			printf("Quiting...\n");
			resourcesRelease(INVALID_SOCKET, NULL, true);
			return EXIT_SUCCESS;
		}
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("socket function failed with error: %ld\n", WSAGetLastError());
			resourcesRelease(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((connect(sock, (struct sockaddr*)&sock_info, sizeof(sock_info))) == SOCKET_ERROR) {
			printf("connect() failed with error code: %d\n", WSAGetLastError());
			resourcesRelease(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((fptr = fopen(file_name, "wb")) == NULL) {
			printf("failed to open file!\n");
			resourcesRelease(sock, NULL, true);
			return EXIT_FAILURE;
		}
		bytes_recv = recv(sock, message, BUFFER_LEN, 0); //first message
		if ((h_buffer = hammingDecode(message, perrors)) == FAILED) { //hamming (31,26,3)
			resourcesRelease(sock, fptr, true);
			return EXIT_FAILURE;
		}
		while ((bytes_recv = recv(sock, message, BUFFER_LEN, 0)) > 0)
		{
			w_buffer = h_buffer;
			if ((h_buffer = hammingDecode(message, perrors)) == FAILED) { //hamming (31,26,3)
				resourcesRelease(sock, fptr, true);
				return EXIT_FAILURE;
			}
			fwrite(&w_buffer, 1U, BUFFER_LEN * sizeof(char), fptr); //write to files
			fseek(fptr, -1L, SEEK_CUR);
			data_recv++;
		}
		if (bytes_recv < 0) { //recv() failure
			printf("recv() failed with error code: %d\n", WSAGetLastError());
			resourcesRelease(sock, fptr, true);
			return EXIT_FAILURE;
		}
		//last message
		fwrite(&h_buffer, 1U, (strlen(message) - 1) * sizeof(char), fptr);

		data_recv++;
		data_recv *= BUFFER_LEN;
		fseek(fptr, 0, SEEK_END);
		file_len = ftell(fptr);
		printf("received: %d\nwrote: %d\ncorrected %d errors\n", data_recv, file_len, errors);

		resourcesRelease(INVALID_SOCKET, fptr, false);
		data_recv = 0, file_len = 0, errors = 0;
	}
	return EXIT_SUCCESS;
}
