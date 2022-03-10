#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>

#include <map>
#include <vector>
#include <string>

#include <sstream>

#include "Buffer.h"

#include "ChatProgram.h"
#include "DBHelper.h"

ChatProgram g_ChatProgram;
DBHelper database;

enum MessageType
{
	JOIN_A_ROOM = 1,
	LEAVE_THE_ROM = 2,
	SEND_THE_MSAGE_TO_ROOM = 3,
	CREATE_ACCOUNT = 4,
	AUTHENTICATE_ACCOUNT = 5,
};

std::map<SOCKET, ChatClientInfo*> g_MapSocketToClientPtr;

typedef std::vector<SOCKET> SocketList;

const int HEADER_SIZE = 12;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5151"

Buffer g_Buffer(DEFAULT_BUFLEN);

int TotalClients = 0;
ChatClientInfo* ClientArray[FD_SETSIZE];

void AddClient(SOCKET socket)
{
	ChatClientInfo* info = new ChatClientInfo();
	info->socket = socket;
	info->bytesRECV = 0;
	ClientArray[TotalClients] = info;
	TotalClients++;
	printf("New client connected on socket %d\n", (int)socket);
}

void RemoveCient(int index)
{
	ChatClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

	// We also need to cleanup the ClientInfo data
	// TODO: Delete Client
}

void SendMessageToClient(SOCKET socket, const std::string& message)
{
	Buffer buffer(512);
	uint32_t packetLength = message.length() + HEADER_SIZE;
	buffer.WriteUInt32BE(packetLength);
	buffer.WriteUInt32BE(0);
	buffer.WriteUInt32BE(message.length());
	buffer.WriteString(message);
	int result = send(socket, (const char*)(&buffer.GetData()[0]), packetLength, 0);
	if (result > 0)
	{
		printf("Sent %d bytes to %d\n", result, socket);
	}
	else if (result < 0)
	{
		printf("Winsock error: %d\n", WSAGetLastError());
	}
}

void BroadcastMessageToRoom(const std::string& roomName, const std::string& message)
{
	std::vector<ChatClientInfo*> clients = g_ChatProgram.GetClientsInRoom(roomName);
	std::size_t numClientsInRoom = clients.size();

	std::stringstream ss;
	ss << "[" << roomName << "] " << message;

	if (clients.size() == 0)
	{
		printf("ERROR: Attempting to send a message to an empty room: %s (%s).\n", roomName.c_str(), message.c_str());
		return;
	}

	for (int i = 0; i < numClientsInRoom; ++i)
	{
		SendMessageToClient(clients[i]->socket, ss.str());
	}
}

void JoinRoom(ChatClientInfo* client, std::string roomName)
{
	ChatProgramMessage msg = g_ChatProgram.JoinRoom(roomName, client);
	std::stringstream ss;

	switch (msg)
	{
	case kAllGood:
		ss << "[" << client->socket << "] joined the room!";
		BroadcastMessageToRoom(roomName, ss.str());
		break;

	case kClientAlreadyInRoom:
		SendMessageToClient(client->socket, "You are already in that room!");
		break;
	}
}

void LeaveRoom(ChatClientInfo* client, std::string roomName)
{
	ChatProgramMessage msg = g_ChatProgram.LeaveRoom(roomName, client);
	std::stringstream ss;

	switch (msg)
	{
	case kAllGood:
		ss << "[" << client->socket << "] has left the room!";
		SendMessageToClient(client->socket, ss.str());
		BroadcastMessageToRoom(roomName, ss.str());
		break;

	case kRoomDoesNotExist:
		SendMessageToClient(client->socket, "Room does not exit!");
		break;

	case kClientNotInRoom:
		SendMessageToClient(client->socket, "You are not in that room!");
		break;
	}
}

void MessageRoom(ChatClientInfo* client, std::string roomName, std::string message)
{
	auto clientsInRoom = g_ChatProgram.GetClientsInRoom(roomName);
	if (std::find(std::begin(clientsInRoom), std::end(clientsInRoom), client) == clientsInRoom.end())
	{
		SendMessageToClient(client->socket, "You are not in that room!");
		return;
	}

	std::stringstream ss;
	ss << client->socket << ": " << message;
	BroadcastMessageToRoom(roomName, ss.str());
}

void ParseMessageFromClient(ChatClientInfo* client)
{
	uint32_t messageId = client->buffer.ReadUInt32LE();

	switch (messageId)
	{
	case MessageType::JOIN_A_ROOM:
	{
		uint32_t roomLength = client->buffer.ReadUInt32LE();
		std::string roomName = client->buffer.ReadString(roomLength);
		JoinRoom(client, roomName);
	}
	break;

	case MessageType::LEAVE_THE_ROM:
	{
		uint32_t roomLength = client->buffer.ReadUInt32LE();
		std::string roomName = client->buffer.ReadString(roomLength);
		LeaveRoom(client, roomName);
	}
	break;

	case MessageType::SEND_THE_MSAGE_TO_ROOM:
	{
		uint32_t roomLength = client->buffer.ReadUInt32LE();
		std::string roomName = client->buffer.ReadString(roomLength);
		uint32_t messageLength = client->buffer.ReadUInt32LE();
		messageLength = messageLength < 256 ? messageLength : 256;
		std::string message = client->buffer.ReadString(messageLength);
		MessageRoom(client, roomName, message);
	}
	break;

	case MessageType::CREATE_ACCOUNT:
	{
		uint32_t emailLength = client->buffer.ReadUInt32LE();
		std::string email = client->buffer.ReadString(emailLength);
		uint32_t passwordLength = client->buffer.ReadUInt32LE();
		passwordLength = passwordLength < 256 ? passwordLength : 256;
		std::string password = client->buffer.ReadString(passwordLength);
		// After parsing the message from the client, create the account in the database
		CreateAccountWebResult result = database.CreateAccount(email, password);
		// And then send the client the success/failure message
		switch (result)
		{
		case CreateAccountWebResult::ACCOUNT_ALREADY_EXISTS:
		{
			SendMessageToClient(client->socket, "That account already exists!");
		}
		break;
		case CreateAccountWebResult::INTERNAL_SERVER_ERROR:
		{
			SendMessageToClient(client->socket, "There was an internal server error, was unable to add the account!");
		}
		break;
		case CreateAccountWebResult::SUCCESS:
		{
			SendMessageToClient(client->socket, "Success!  Account added!");
		}
		break;
		case CreateAccountWebResult::NO_CONNECTION:
		{
			SendMessageToClient(client->socket, "I'm sorry, the server is currently not connected to the database.  Please try restarting the program.");
		}
		break;
		default:
			printf("Error: Message type does not exist!");
			break;

		} //end of switch
	}
	break;

	case MessageType::AUTHENTICATE_ACCOUNT:
	{
		uint32_t emailLength = client->buffer.ReadUInt32LE();
		std::string email = client->buffer.ReadString(emailLength);
		uint32_t passwordLength = client->buffer.ReadUInt32LE();
		passwordLength = passwordLength < 256 ? passwordLength : 256;
		std::string password = client->buffer.ReadString(passwordLength);
		// After parsing the message from the client, authenticate the account
		CreateAccountWebResult result = database.AuthenticateAccount(email, password);
		// And tell the client what the result was
		switch (result)
		{
		case CreateAccountWebResult::ACCOUNT_DOES_NOT_EXIST:
		{
			SendMessageToClient(client->socket, "That account does not exist!");
			//whoops, forgot to break lol
		}
		break;
		case CreateAccountWebResult::INTERNAL_SERVER_ERROR:
		{
			SendMessageToClient(client->socket, "There was an internal server error, was unable to add the account!");
		}
		break;
		case CreateAccountWebResult::VALID_PASSWORD:
		{
			std::string dateCreated = database.GetAccountCreationDate(email);
			std::stringstream ss;
			if (dateCreated == "")
			{
				ss << "Failed, was not able to get the account creation date.";
			}
			else
			{
				ss << "Success!  That is the correct password!  Account was created on: " << dateCreated;
			}
			SendMessageToClient(client->socket, ss.str().c_str());
		}
		break;
		case CreateAccountWebResult::INVALID_PASSWORD:
		{
			SendMessageToClient(client->socket, "Nuh huh!  That's the wrong password!");
		}
		break;
		case CreateAccountWebResult::NO_CONNECTION:
		{
			SendMessageToClient(client->socket, "I'm sorry, the server is currently not connected to the database.  Please try restarting the program.");
		}
		break;
		default:
			printf("Error: Message type does not exist!");
			break;

		} //end of switch
	}
	break;

	default:
		printf("ERROR: Message type does not exist: %d\n", messageId);
		break;
	}

	client->bytesRECV = 0;
	client->packetLength = 0;
	client->buffer.Reset();
}

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;

	// Connect to the database
	database.Connect("127.0.0.1", "root", "root89");
	//while (!database.IsConnected())
	//{
	//	printf("let's break this shit\n");
	//	database.Connect("127.0.0.1", "root", "root89");
	//}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	// #1 Socket
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	// Create a SOCKET for connecting to the server
	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		// -1 -> Actual Error Code
		// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	// #2 Bind - Setup the TCP listening socket
	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	// We don't need this anymore
	freeaddrinfo(addrResult);

	// #3 Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}

	// Change the socket mode on the listening socket from blocking to
	// non-blocking so the application will not block waiting for requests
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SentBytes;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		// Initialize our read set
		FD_ZERO(&ReadSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &ReadSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		// Call our select function to find the sockets that
		// require our attention
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}

		// #4 Check for arriving connections on the listening socket
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			FD_CLR(listenSocket, &ReadSet);
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");
					AddClient(acceptSocket);
				}
			}
		}

		// #5 recv & send
		for (int i = 0; i < TotalClients; i++)
		{
			ChatClientInfo* client = ClientArray[i];

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;

				DWORD flags = 0;
				iResult = recv(client->socket, (char*)(&client->buffer.GetData()[client->bytesRECV]), DEFAULT_BUFLEN, flags);
				client->bytesRECV += iResult;

				if (iResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("recv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveCient(i);
					}
				}
				else if (iResult > 0)
				{
					if (client->bytesRECV >= 4)
					{
						if (client->packetLength == 0)
						{
							client->packetLength = client->buffer.ReadUInt32LE();
						}

						if (client->bytesRECV == client->packetLength)
						{
							ParseMessageFromClient(client);
						}
					}
				}
				else
				{
					// Client disconnected..
					RemoveCient(i);
				}
			}
		}

	}

	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}