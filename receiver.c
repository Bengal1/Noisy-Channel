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


int power2(int x)
{
	int res = 1;

	if (x < 0) {
		printf("argument is negative!\n");
		return FAILED;
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
		(*s) = INVALID_SOCKET;
	}	

	return;
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


unsigned int hamming_decode(char* mes, int* errors)
{
	int j = 0, error_pose = 0; 
	unsigned int res = 0, message = 0, * imessage = NULL;
	int par_checks[5] = { 0 };

	imessage = (unsigned int*)mes;
	message = *imessage;
	imessage = &message;

	//error detection & correction
	for (int i = 1; i < 6; i++) {
		if ((parity_bit_check(i, message)) == false) {
			error_pose += power2(i - 1);
		}
	}
	if ((parity_bit_check(0, message) == false) && (error_pose == 0)) {
		message = message ^ 0b1;
		(*errors)++;
	}
	if (error_pose > 0) {
		message = message ^ (power2(error_pose)); //1-bit error correct
		(*errors)++;
	}
	
	//extract data
	for (int i = 3; i < FOUR_BYTES; i++) {
		if ((i == 4) || (i == 8) || (i == 16)) {
			continue;
		}
		else {
			if (power2(i) & message) {
				res += power2(j);
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
		if (0 == strcmp(lowercase(file_name), "quit")) {
			printf("Quiting...\n");
			resources_release(INVALID_SOCKET, NULL, true);
			return EXIT_SUCCESS;
		}
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("socket function failed with error: %ld\n", WSAGetLastError());
			resources_release(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((connect(sock, (struct sockaddr*)&sock_info, sizeof(sock_info))) == SOCKET_ERROR) {
			printf("connect() failed with error code: %d\n", WSAGetLastError());
			resources_release(INVALID_SOCKET, NULL, true);
			return EXIT_FAILURE;
		}
		if ((fptr = fopen(file_name, "wb")) == NULL) {
			printf("failed to open file!\n");
			resources_release(sock, NULL, true);
			return EXIT_FAILURE;
		}
		bytes_recv = recv(sock, message, BUFFER_LEN, 0); //first message
		if ((h_buffer = hamming_decode(message, perrors)) == FAILED) { //hamming (31,26,3)
			resources_release(sock, fptr, true);
			return EXIT_FAILURE;
		}
		while ((bytes_recv = recv(sock, message, BUFFER_LEN, 0)) > 0)
		{
			w_buffer = h_buffer;
			if ((h_buffer = hamming_decode(message, perrors)) == FAILED) { //hamming (31,26,3)
				resources_release(sock, fptr, true);
				return EXIT_FAILURE;
			}
			fwrite(&w_buffer, 1U, BUFFER_LEN * sizeof(char), fptr); //write to files
			fseek(fptr, -1L, SEEK_CUR);
			data_recv++;
		}
		if (bytes_recv < 0) { //recv() failure
			printf("recv() failed with error code: %d\n", WSAGetLastError());
			resources_release(sock, fptr, true);
			return EXIT_FAILURE;
		}
		//last message
		fwrite(&h_buffer, 1U, (strlen(message) - 1) * sizeof(char), fptr);
		
		data_recv++;
		data_recv *= BUFFER_LEN;
		fseek(fptr, 0, SEEK_END);
		file_len = ftell(fptr);
		printf("received: %d\nwrote: %d\ncorrected %d errors\n", data_recv, file_len, errors);
		
		resources_release(INVALID_SOCKET, fptr, false);
		data_recv = 0, file_len = 0, errors = 0;
	}
	return EXIT_SUCCESS;
}
