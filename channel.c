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
#include <string.h>
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 4
#define IP_SIZE 16
#define MAX_PORT 65535
#define MIN_PORT 1024
#define FOUR_BYTES 32
#define FAILED -1


typedef struct MessageNode {
	char* content;
	struct MessageNode* next;
} MessageNode;

int powerOfTwo(int x)
{
	if (x < 0) {
		printf("Error: Negative exponent is not allowed!\n");
		return FAILED;
	}
	return 1 << x;
}

void gracefullTermination(MessageNode* list, SOCKET listen_s, SOCKET listen_r, SOCKET send, SOCKET recv, int WSA)
{
	MessageNode* temp;

	while (list != NULL) {
		temp = list;
		list = list->next;
		free(temp->content);
		free(temp);
	}

	if (listen_s != INVALID_SOCKET) shutdown(listen_s, SD_BOTH);
	if (listen_r != INVALID_SOCKET) shutdown(listen_r, SD_BOTH);
	if (send != INVALID_SOCKET) closesocket(send);
	if (recv != INVALID_SOCKET) closesocket(recv);
	if (WSA) WSACleanup();
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

int initializeSockets(SOCKET sockets[2])
{
	struct sockaddr_in sockets_details;
	struct hostent* localHost;
	char* localIP = NULL;

	// Initialize Winsock
	WSADATA wsaData;
	int iResult, ports[2];

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with Error Code : %d\n", iResult);
		return FAILED;
	}

	srand((unsigned int)time(NULL));
	// Get the local host information
	localHost = gethostbyname("");
	if (!localHost) {
		printf("Failed to retrieve host info\n");
		WSACleanup();
		return FAILED;
	}
	localIP = inet_ntoa(*(struct in_addr*)*localHost->h_addr_list);

	for (int i = 0; i < 2; i++) {
		if ((sockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("Failed to create a new socket with Error Code : %d\n", WSAGetLastError());
			return FAILED;
		}

		ports[i] = (rand() % (MAX_PORT - MIN_PORT)) + MIN_PORT;  // choose port randomly -- 1024-65535

		// Set up the sockaddr structure
		sockets_details.sin_family = AF_INET;
		sockets_details.sin_port = htons(ports[i]);
		sockets_details.sin_addr.s_addr = inet_addr(localIP);

		iResult = bind(sockets[i], (SOCKADDR*)&sockets_details, sizeof(sockets_details));
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			return FAILED;
		}
		if (listen(sockets[i], SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %d\n", WSAGetLastError());
			return FAILED;
		}
	}
	printf("sender socket : IP address = %s  port = %d\nreceiver socket : IP address = %s  port = %d\n\n", localIP, ports[0], localIP, ports[1]);

	return EXIT_SUCCESS;
}

MessageNode* moveForward(MessageNode* node, int bitCount) 
{
	int nodeSteps = bitCount / FOUR_BYTES;
	while (nodeSteps-- > 0 && node) {
		node = node->next;
	}
	return node;
}

void flipBitInNode(MessageNode* node, int bitOffset) 
{
	if (!node || !node->content) return;  // Safety check

	unsigned int* bitBuffer = (unsigned int*)node->content;
	*bitBuffer ^= (1U << bitOffset);
}

int noiseAddition(MessageNode* messageListHead, const char* noiseType, int noiseParameter) 
{
	if (!messageListHead || !noiseType || noiseParameter <= 0) {
		printf("Invalid input parameters.\n");
		return FAILED;
	}

	int bitRange = 0, flippedBitsCount = 0, targetBitIndex = 0, bitOffset = 0;
	MessageNode* currentNode = messageListHead->next;

	if (strcmp(noiseType, "-r") == 0) {  // Random bit flipping
		srand((unsigned int)time(NULL));
		bitRange = powerOfTwo(16) / noiseParameter;  // 2^16 = 65536

		while (currentNode) {
			targetBitIndex = rand() % bitRange;

			currentNode = moveForward(currentNode, targetBitIndex + bitOffset);
			if (!currentNode || !currentNode->content) return flippedBitsCount;

			bitOffset = (targetBitIndex + bitOffset) % FOUR_BYTES;
			flipBitInNode(currentNode, bitOffset);
			flippedBitsCount++;

			currentNode = moveForward(currentNode, bitRange - targetBitIndex + bitOffset);
			bitOffset = bitRange % FOUR_BYTES;
		}
	}
	else if (strcmp(noiseType, "-d") == 0) {  // Flip every n-th bit
		targetBitIndex = noiseParameter;

		while (currentNode) {
			currentNode = moveForward(currentNode, targetBitIndex + bitOffset);
			if (!currentNode || !currentNode->content) return flippedBitsCount;

			bitOffset = (targetBitIndex + bitOffset) % FOUR_BYTES;
			flipBitInNode(currentNode, bitOffset);
			flippedBitsCount++;
		}
	}
	else {
		printf("Unrecognized noise type: %s\n", noiseType);
		return FAILED;
	}
	return flippedBitsCount;
}

int main(int argc, char* argv[])
{
	int noise_param = 0, bytes_recv = 1, bytes_sent = 0, messages_recv = 0, flipped_bits = 0;
	char noise_type[3], buffer[BUFFER_LEN] = { '\0' }, answer[BUFFER_LEN] = { '\0' }, * s_buffer = NULL;
	SOCKET listen_sockets[2] = { INVALID_SOCKET }, sender = INVALID_SOCKET, receiver = INVALID_SOCKET;
	MessageNode* data = NULL, * head = NULL, * struct_buff = NULL;

	strcpy(noise_type, argv[1]);
	noise_param = atoi(argv[2]);

	if (initializeSockets(listen_sockets) == FAILED) {
		printf("Failed to initiate sockets: %d\n", WSAGetLastError());
		gracefullTermination(NULL, listen_sockets[0], listen_sockets[1], INVALID_SOCKET, INVALID_SOCKET, 1);
		WSACleanup();
		return EXIT_FAILURE;
	}

	while (1) {//channel routine
		sender = accept(listen_sockets[0], NULL, NULL);
		if (sender == INVALID_SOCKET) {
			printf("accept() failed: %d\n", WSAGetLastError());
			gracefullTermination(NULL, listen_sockets[0], listen_sockets[1], INVALID_SOCKET, INVALID_SOCKET, 1);
			return EXIT_FAILURE;
		}

		receiver = accept(listen_sockets[1], NULL, NULL);
		if (receiver == INVALID_SOCKET) {
			printf("accept() failed: %d\n", WSAGetLastError());
			gracefullTermination(NULL, listen_sockets[0], listen_sockets[1], sender, INVALID_SOCKET, 1);
			return EXIT_FAILURE;
		}

		if ((head = (MessageNode*)malloc(sizeof(MessageNode))) == NULL) {
			printf("Allocation Failed\n");
			gracefullTermination(NULL, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
			return EXIT_FAILURE;
		}
		data = head;
		head->content = NULL;
		head->next = NULL;

		bytes_sent = send(sender, "OK", 2, 0); // send 'ready' message to sender - receiver connected
		if (bytes_sent != 2) {
			printf("send() failed: %d\n", WSAGetLastError());
			gracefullTermination(head, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
			return EXIT_FAILURE;
		}

		while ((bytes_recv = recv(sender, buffer, BUFFER_LEN, 0)) > 0) { // receive data from sender
			if ((data->next = (MessageNode*)malloc(sizeof(MessageNode))) == NULL) {
				printf("Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
			data = data->next;
			data->next = NULL;
			data->content = NULL;
			if ((data->content = (char*)malloc(BUFFER_LEN * sizeof(char))) == NULL) {
				printf("Allocation Failed\n");
				exit(EXIT_FAILURE);
			}

			memcpy(data->content, buffer, BUFFER_LEN);
			messages_recv++;
		}
		if (bytes_recv < 0) {
			printf("recv() failed with code: %d\n", WSAGetLastError());
			gracefullTermination(head, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
			return EXIT_FAILURE;
		}
		data = head->next;

		if ((flipped_bits = noiseAddition(head, noise_type, noise_param)) == FAILED) {  // adding noise
			gracefullTermination(head, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
			return EXIT_FAILURE;
		}

		while (data != NULL) {  // send data to receiver 
			s_buffer = data->content;
			bytes_sent = send(receiver, s_buffer, BUFFER_LEN, 0);
			if ((bytes_sent) != BUFFER_LEN) {
				printf("Error: send() failed with code: %d\n", WSAGetLastError());
				gracefullTermination(head, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
				return EXIT_FAILURE;
			}
			data = data->next;
		}
		closesocket(receiver);

		messages_recv *= BUFFER_LEN;
		printf("retransmitted %d bytes, flipped %d bits\ncontinue? (yes/no)\n", messages_recv, flipped_bits);
		messages_recv = 0, flipped_bits = 0;

		gets(answer);
		if (0 == strcmp(toLowerCase(answer), "yes")) {
			strcpy(answer, "");
			gracefullTermination(head, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0);
			continue;
		}
		else {
			printf("Quiting...\n");
			gracefullTermination(head, listen_sockets[0], listen_sockets[1], sender, receiver, 1);
			return EXIT_SUCCESS;
		}
	}
}
