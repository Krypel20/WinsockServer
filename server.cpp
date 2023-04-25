#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>
#include <locale>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "20202"
#define DEFAULT_BUFLEN 512

WSADATA wsaData;

struct addrinfo* result = NULL, *ptr = NULL, hints;
char recvbuf[DEFAULT_BUFLEN];
int iResult, iSendResult;
int recvbuflen = DEFAULT_BUFLEN;
int connectedSockets;
bool gameRunning = false;
SOCKET ListenSocket = INVALID_SOCKET; //SOCKET nasluchujacy polaczen klientow
SOCKET ClientSockets[2] = { INVALID_SOCKET,INVALID_SOCKET }; // tablica dwóch socketów dla dwóch graczy (P1, P2)
void gameEvents(SOCKET clientSocket);
void gameRun();

void startupWinsock()
{
	// Funkcja WSAStartup jest wywolywana w celu zainicjowania uzycia WS2_32.dll.
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return;
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
		return;
	}
}

void startListening()
{
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return;
	}
	else { printf("Socket valid!\n"); }


	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	else { printf("TCP listening socket activated!\n"); }
	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	//ClientSocket = INVALID_SOCKET;
	connectedSockets = 0;
	int readyPlayers = 2; //0
	while (connectedSockets < 2)
	{
		printf("Waiting for connection from players...\n");
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
			return;
		}
		ClientSockets[connectedSockets] = ClientSocket;
		connectedSockets++;
		printf("Player %d connected", connectedSockets);
		printf(" Client address: %s\n", clientIP);
		printf("Players connected: %d/2\n", connectedSockets);
	}
	printf("Both players joined the server!\n");
}


bool arePlayersReady(SOCKET clientSocket[])
{
	return true;
}

void gameRun()
{
	while (connectedSockets == 2) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(ClientSockets[0], &readfds);
		FD_SET(ClientSockets[1], &readfds);

		// Sprawdzanie który socket(gracz) jest gotowy do odczytu
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
}

void gameEvents(SOCKET clientSocket) {
	char recvData[DEFAULT_BUFLEN];
	int iResult, iSendResult;

	while (gameRunning == true) {
		iResult = recv(clientSocket, recvData, DEFAULT_BUFLEN, 0);

		if (iResult > 0 && isdigit(recvData[0]) && recvData!="test!") //wysylanie punktów(int) miedzy P1 i P2
		{
			int recvPoints = atoi(recvData);

			// Wysyłamy wiadomość od klienta do drugiego klienta
			if (clientSocket == ClientSockets[0])
			{
				iSendResult = send(ClientSockets[1], recvData, iResult, 0);
				std::cout << "P1: " << recvData << std::endl;
			}
			else if (clientSocket == ClientSockets[1])
			{
				iSendResult = send(ClientSockets[0], recvData, iResult, 0);
				std::cout << "P2: " << recvData<<std::endl;
			}
			if (iResult == SOCKET_ERROR) {
				printf("send failed with error");
				gameRunning = false;
				break;
			}
		}
		else if (iResult > 0) //wysylanie wiadomosci miedzy P1 i P2
		{
			// Wysyłamy wiadomość od klienta do drugiego klienta
			if (clientSocket == ClientSockets[0])
			{
				iSendResult = send(ClientSockets[1], recvData, iResult, 0);
				std::cout << "P1: " << recvData << std::endl;
			}
			else if (clientSocket == ClientSockets[1])
			{
				iSendResult = send(ClientSockets[0], recvData, iResult, 0);
				std::cout << "P2: " << recvData << std::endl;
			}
			if (iResult == SOCKET_ERROR) {
				printf("send failed with error");
				gameRunning = false;
				break;
			}
		}
		else if (iResult == 0) 
		{
			gameRunning = false;
			if (clientSocket == ClientSockets[0])
			{
				printf("P1 disconnected\n");
				closesocket(ClientSockets[0]);
			}
			if (clientSocket == ClientSockets[1])
			{
				printf("P2 disconnected\n");
				closesocket(ClientSockets[1]);
			}
		}
	}
}

int main() {
listen:
	gameRunning = false;
	startupWinsock();
	startListening();

	printf("-----------------Game start!-----------------\n");

	gameRunning = true;
	gameRun();

	// Zamknij sockety i zwolnij zasoby
	printf("---------CLOSING CONNECTION ON BOTH SOCKETS!-------------\n");
	closesocket(ListenSocket);
	WSACleanup();
	goto listen;
}