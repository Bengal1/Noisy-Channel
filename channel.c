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


typedef struct message {
	char * content;
	struct message * next;
} Message;


int power2(int x)
{
	int res = 1;

	if (x < 0) {
		printf("error:negative exponent!\n");
		return FAILED;
	}

	for (int i = 0; i < x; i++) {
		res *= 2;
	}
	return res;
}

void gracefull_termination(Message* list, SOCKET listen_s, SOCKET listen_r, SOCKET send, SOCKET recv, int WSA) 
{
	Message* previous = NULL, * current = NULL;

	if (list != NULL) { //free data struct Message
		previous = list;
		current = previous->next;
		while (current != NULL) {
			free(previous->content);
			free(previous);
			previous = current;
			current = previous->next;
		}
		free(previous->content);
		free(previous);
	}

	if (listen_s != INVALID_SOCKET)
		shutdown(listen_s, 2);
	if (listen_r != INVALID_SOCKET)
		shutdown(listen_r, 2);
	if (send != INVALID_SOCKET)
		closesocket(send);
	if (recv != INVALID_SOCKET)
		closesocket(recv);
	if(WSA)
		WSACleanup();

	return;
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

int init_sockets(SOCKET sockets[2])
{
	struct sockaddr_in sockets_details;
	struct hostent* localHost;
	struct addrinfo hints;
	char* localIP = NULL;

	// Initialize Winsock
	WSADATA wsaData;
	int iResult, ports[2];

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with Error Code : %d\n", iResult);
		return EXIT_FAILURE;
	}

	srand((unsigned int)time(NULL));
	// Get the local host information
	localHost = gethostbyname("");
	localIP = inet_ntoa(*(struct in_addr*)*localHost->h_addr_list);

	for (int i = 0; i < 2; i++) {
		if ((sockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
			printf("Failed to create a new socket with Error Code : %d\n", WSAGetLastError());
			return EXIT_FAILURE;
		}

		ports[i] = (rand() % (MAX_PORT - MIN_PORT)) + MIN_PORT;  // choose port randomly -- 1024-65535

		// Set up the sockaddr structure
		sockets_details.sin_family = AF_INET;
		sockets_details.sin_port = htons(ports[i]);
		sockets_details.sin_addr.s_addr = inet_addr(localIP);//S_un.S_addr = INADDR_ANY; //s_addr = inet_addr(localIP);


		iResult = bind(sockets[i], (SOCKADDR*)&sockets_details, sizeof(sockets_details));
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			return EXIT_FAILURE;
		}
		if (listen(sockets[i], SOMAXCONN) == SOCKET_ERROR) {
			printf("Listen failed with error: %d\n", WSAGetLastError());
			return EXIT_FAILURE;
		}
	}
	printf("sender socket : IP address = %s  port = %d\nreceiver socket : IP address = %s  port = %d\n", localIP, ports[0], localIP, ports[1]);
	

	return EXIT_SUCCESS;
}

int noise_addition(Message* head, const char* noise_type, const int noise_param)
{
	int range = 0, flipped = 0, target = 0, offset = 0;
	unsigned int ibuff = 0, * p_ibuff = NULL;
	char* cbuff = NULL;
	Message* data = head->next;


	if (0 == strcmp(noise_type, "-r")) { //probability n/(2^16) for error in a bit
		srand((unsigned int)time(NULL));
		range = power2(16)/noise_param;
		while (data != NULL) {
			target = rand() % range;
			for (int i = 0; i < ((target + offset) / 32); i++) {
				data = data->next;
				if (data == NULL)
					return flipped;
			}
			offset = (target + offset) % 32;
			// flip bit
			p_ibuff = (unsigned int*)data->content;
			ibuff = *p_ibuff;
			p_ibuff = &ibuff;
			ibuff = ibuff ^ (power2(offset));
			cbuff = (char*)p_ibuff;
			memcpy((data->content), cbuff, BUFFER_LEN);
			flipped++;
			for (int i = 0; i < ((range - target + offset) / 32); i++) {
				data = data->next;
				if (data == NULL)
					return flipped;
			}
			offset = range % 32;
		}
		
	}
	else if (0 == strcmp(noise_type, "-d")) { //every n-th bit
		target = noise_param;
		while (data != NULL) {
			for (int i = 0; i < ((target + offset)/ 32); i++) {
				data = data->next;
				if (data == NULL)
					return flipped;
			}
			offset = (target + offset) % 32;
			// flip bit
			p_ibuff = (unsigned int *)data->content;
			ibuff = *p_ibuff;
			p_ibuff = &ibuff;
			ibuff = ibuff ^ (power2(offset));
			cbuff = (char*)p_ibuff;
			memcpy((data->content), cbuff, BUFFER_LEN);
			flipped++;
		}
	}
	else {
		printf("unfimiliar noise type...\n");
		return FAILED;
	}
	return flipped;
}

int main(int argc, char* argv[])
{
	int noise_param = 0, bytes_recv = 1, bytes_sent = 0, messages_recv = 0, flipped_bits = 0;
	char noise_type[3], buffer[BUFFER_LEN] = { '\0' }, answer[BUFFER_LEN] = { '\0' }, * s_buffer = NULL;
	SOCKET listen_sockets[2] = { INVALID_SOCKET }, sender = INVALID_SOCKET, receiver = INVALID_SOCKET;
	Message* data = NULL, * head = NULL, *struct_buff = NULL;

	strcpy(noise_type, argv[1]);
	noise_param = atoi(argv[2]);

	if (init_sockets(listen_sockets) == EXIT_FAILURE) {
		printf("Failed to initiate sockets: %d\n", WSAGetLastError());
		gracefull_termination(NULL, listen_sockets[0], listen_sockets[0], INVALID_SOCKET, INVALID_SOCKET, 1);
		WSACleanup();
		return EXIT_FAILURE;
	}
	
	while (1) {//channel routine
		sender = accept(listen_sockets[0], NULL, NULL);
		if (sender == INVALID_SOCKET) {
			printf("accept() failed: %d\n", WSAGetLastError());
			gracefull_termination(NULL, listen_sockets[0], listen_sockets[0], INVALID_SOCKET, INVALID_SOCKET, 1);
			return EXIT_FAILURE;
		}

		receiver = accept(listen_sockets[1], NULL, NULL);
		if (receiver == INVALID_SOCKET) {
			printf("accept() failed: %d\n", WSAGetLastError());
			gracefull_termination(NULL, listen_sockets[0], listen_sockets[0], sender, INVALID_SOCKET, 1);
			return EXIT_FAILURE;
		}
		
		if ((head = (Message*)malloc(sizeof(Message))) == NULL) {
			printf("Allocation Failed\n");
			gracefull_termination(NULL, listen_sockets[0], listen_sockets[0], sender, receiver, 1);
			return EXIT_FAILURE;
		}
		data = head;
		head->content = NULL;
		head->next = NULL;
	
		bytes_sent = send(sender, "OK", 2, 0); // send 'ready' message to sender - receiver connected
		if (bytes_sent != 2) {
			printf("send() failed: %d\n", WSAGetLastError());
			gracefull_termination(head, listen_sockets[0], listen_sockets[0], sender, receiver, 1);
			return EXIT_FAILURE;
		}
		
		while ((bytes_recv = recv(sender, buffer, BUFFER_LEN, 0)) > 0) { // recv data from sender
			if ((data->next = (Message*)malloc(sizeof(Message))) == NULL) {
				printf("Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
			//data->next = struct_buff;
			data = data->next;
			data->next = NULL;
			data->content = NULL;
			if ((data->content = (char*)malloc(BUFFER_LEN*sizeof(char))) == NULL) {
				printf("Allocation Failed\n");
				exit(EXIT_FAILURE);
			}
			
			memcpy(data->content, buffer, BUFFER_LEN);
			messages_recv ++;
		}
		if (bytes_recv < 0) { 
			printf("recv() failed with code: %d\n", WSAGetLastError());
			gracefull_termination(head, listen_sockets[0], listen_sockets[0], sender, receiver, 1);
			return EXIT_FAILURE;
		}
		data = head->next;
		
		if ((flipped_bits = noise_addition(head, noise_type, noise_param)) == FAILED) {  // adding noise
			gracefull_termination(head, listen_sockets[0], listen_sockets[0], sender, receiver, 1);
			return EXIT_FAILURE;
		}

		while (data != NULL) {  // send data to receiver 
			s_buffer = data->content;
			bytes_sent = send(receiver, s_buffer, BUFFER_LEN, 0);
			if ((bytes_sent) != BUFFER_LEN) {
				printf("send() failed with code: %d\n", WSAGetLastError());
				gracefull_termination(head, listen_sockets[0], listen_sockets[0], sender, receiver, 1);
				return EXIT_FAILURE;
			}
			data = data->next;
		}
		closesocket(receiver);

		messages_recv *= BUFFER_LEN;
		printf("retransmitted %d bytes, flipped %d bits\n", messages_recv, flipped_bits);
		messages_recv = 0, flipped_bits = 0;
		printf("continue? (yes/no)\n");
		gets(answer);
		if (0 == strcmp(lowercase(answer), "yes")) {
			strcpy(answer,"");
			gracefull_termination(head, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, 0);
			continue;
		}
		else {
			printf("Quiting...\n");
			gracefull_termination(head, listen_sockets[0], listen_sockets[0], INVALID_SOCKET, INVALID_SOCKET, 1);
			return EXIT_SUCCESS;
		}
	}
}


