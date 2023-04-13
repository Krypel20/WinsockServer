#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "202012"
#define DEFAULT_BUFLEN 512

struct addrinfo* result = NULL, * ptr = NULL, hints;
char recvbuf[DEFAULT_BUFLEN];
int iResult, iSendResult;
int recvbuflen = DEFAULT_BUFLEN;
bool gameRunning = false;

SOCKET ClientSockets[2] = { INVALID_SOCKET,INVALID_SOCKET };
void gameEvents(SOCKET clientSocket);
void gameEnd(SOCKET clientSocket);

int main() {
	
	WSADATA wsaData;
	int iResult;

Start:
	// Funkcja WSAStartup jest wywolywana w celu zainicjowania uzycia WS2_32.dll.
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

Listen:
	SOCKET ListenSocket = INVALID_SOCKET; //SOCKET o nazwie ListenSocket, aby serwer nas�uchiwa� po��cze� klient�w.
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	else { printf("Socket valid!\n"); }


	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else { printf("TCP listening socket activated!\n"); }
	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//ClientSocket = INVALID_SOCKET;
	int connectedSockets = 0;
	while (connectedSockets < 2)
	{
		printf("Waiting for connection...\n");
		// Get client's IP address and port number
		sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);
		getpeername(ClientSockets[connectedSockets], (sockaddr*)&clientAddr, &addrLen);
		char clientIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

		SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		ClientSockets[connectedSockets] = ClientSocket;
		connectedSockets++;
		printf("Player %d connected", connectedSockets);
		printf(" Client address: %s\n", clientIP);
		printf("Players connected: %d/2\n", connectedSockets);
	}
Rematch:
	printf("-----------------Game starts!-----------------\n");
	gameRunning = true;

	while (gameRunning == true) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(ClientSockets[0], &readfds);
		FD_SET(ClientSockets[1], &readfds);

		// Checking which sockets are ready to read
		int activity = select(0, &readfds, NULL, NULL, NULL);
		if (activity == SOCKET_ERROR) {
			printf("select call failed with error: %d\n", WSAGetLastError());
			break;
		}

		// Tworzymy wątki dla każdego klienta
		if (FD_ISSET(ClientSockets[0], &readfds)) {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gameEvents, (LPVOID)ClientSockets[0], 0, NULL);
		}

		if (FD_ISSET(ClientSockets[1], &readfds)) {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gameEvents, (LPVOID)ClientSockets[1], 0, NULL);
		}
	}

	while (gameRunning==false)
	{
		printf("Waiting for players decisions...\n");
		break;
	}
	// Zamknij sockety i zwolnij zasoby
	printf("---------CLOSING CONNECTIONS ON BOTH SOCKETS!-------------");
	closesocket(ClientSockets[0]);
	closesocket(ClientSockets[1]);
	closesocket(ListenSocket);
	WSACleanup();
}

void gameEvents(SOCKET clientSocket) {
	char recvbuf[DEFAULT_BUFLEN];
	int iResult, iSendResult;

	while (gameRunning == true) {
		iResult = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
		if (iResult > 0) {
			// Wysyłamy wiadomość od klienta do drugiego klienta
			if (clientSocket == ClientSockets[0]) {
				iSendResult = send(ClientSockets[1], recvbuf, iResult, 0);
			}
			else {
				iSendResult = send(ClientSockets[0], recvbuf, iResult, 0);
			}
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				break;
			}
		}
		else if (iResult == 0) {
			printf("Client disconnected\n");
			gameRunning = false;
			break;
		}
		else {
			printf("Client left: %d\n", WSAGetLastError());
			gameRunning = false;
			break;
		}
	}
}