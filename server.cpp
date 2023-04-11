#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <SFML/Network.hpp>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "1202"
#define DEFAULT_BUFLEN 512

struct addrinfo* result = NULL, *ptr = NULL, hints;
char recvbuf[DEFAULT_BUFLEN];
int iResult, iSendResult;
int recvbuflen = DEFAULT_BUFLEN;
int connectedSockets;

SOCKET ClientSockets[2] = { INVALID_SOCKET,INVALID_SOCKET }; // tablica dwóch socketów dla P1 i P2
void gameEvents(SOCKET clientSocket);
bool gameRestart();

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

listen:
	SOCKET ListenSocket = INVALID_SOCKET; //SOCKET nasluchujacy polaczen klientow
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
			return 1;
		}
		ClientSockets[connectedSockets] = ClientSocket;
		connectedSockets++; 
		printf("Player %d connected", connectedSockets);
		printf(" Client address: %s\n", clientIP);
		printf("Players connected: %d/2\n", connectedSockets);
	}
	printf("Both players joined the server!\n");
	//funkcja sprawdzająca gotowość obu graczy do gry

Rematch:
	printf("-----------------Game starts!-----------------\n");
	//pętla gry
	while (connectedSockets==2) {
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

	//pętla zakonczenia gry lub rewanżu
	while (true)
	{
		printf("Waiting for players decisions...\n");

		if (gameRestart() == true)
		{
			printf("zdecydowano ze bedzie rewanz!\n");
			Sleep(2000);
			goto Rematch;
		}
		else {Sleep(2000); break;}
	}

	// Zamknij sockety i zwolnij zasoby
	printf("---------CLOSING CONNECTIONS ON BOTH SOCKETS!-------------\n");
	closesocket(ClientSockets[0]);
	closesocket(ClientSockets[1]);
	closesocket(ListenSocket);
	WSACleanup();
}


void gameEvents(SOCKET clientSocket) {
	char recvData[DEFAULT_BUFLEN];
	int bytesRead = 8, sendBuf;
	
	while (true) {

		memset(recvData, 0, bytesRead);
		bytesRead = recv(clientSocket, recvData, DEFAULT_BUFLEN, 0);

		if (bytesRead > 0 && recvData!="gameover") {

			// Wysyłamy wiadomość od klienta do drugiego klienta	
			if (clientSocket == ClientSockets[0]) {
				send(ClientSockets[1], recvData, bytesRead, 0);
				printf("P1: %s\n", recvData);
				memset(recvData, 0, bytesRead);
			}
			else {
				send(ClientSockets[0], recvData, bytesRead, 0);
				printf("P2: %s\n", recvData);
				memset(recvData, 0, bytesRead);
			}
			if (sendBuf == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				break;
			}
		}
		else if (recvData == "gameover")
		{
			printf("Jeden z graczy przegral\n");
			if (clientSocket == ClientSockets[0]) {
				sendBuf = send(ClientSockets[1], recvData, bytesRead, 0); //P2 dostaje wiadomosc że P1 przegrał
				printf("P1: %s\n", recvData);
				memset(recvData, 0, bytesRead);
			}
			else {
				sendBuf = send(ClientSockets[0], recvData, bytesRead, 0); //P1 dostaje wiadomosc że P2 przegrał
				printf("P2: %s\n", recvData);
				memset(recvData, 0, bytesRead);
			}
		}
		else {
			printf("Client disconnected: %d\n", WSAGetLastError());
			connectedSockets--;
			break;
		}
	}
}

bool gameRestart()
{
	char message[27] = "Czy chcesz powtorzyc gre?\n";
	char P1msg[] = "";
	char P2msg[] = "";
	send(ClientSockets[0], message, 27, 0);
	send(ClientSockets[1], message, 27, 0);

	while (true)
	{
		if (P1msg == "tak" && P2msg == "tak")
		{
			return true;
		}
		else {
			printf("Gracze się nie zgodzili. P1: %s", P1msg); printf(" P2: %s\n", P2msg);
			char message[50] = "Koniec gry, przeciwnik nie zgodzil sie na rewanz\n";
			send(ClientSockets[0], message, 50, 0);
			send(ClientSockets[1], message, 50, 0);
			return false;
		}
	}
}