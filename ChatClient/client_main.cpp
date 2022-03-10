#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

//#define ENABLE_DEBUG

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <conio.h>

#include <string>
#include "Buffer.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512						// Default buffer length of our buffer in characters
#define DEFAULT_PORT "5151"					// The default port to use
#define SERVER "127.0.0.1"						// The IP of our server
SOCKET g_ServerSocket = INVALID_SOCKET;
bool g_IsRunning = true;

void PrintHelpMessage();

enum MessageType
{
	NONE = 0,
	JOIN_A_ROOM = 1,
	LEAVE_THE_ROM = 2,
	SEND_THE_MSAGE_TO_ROOM = 3,
	CREATE_ACCOUNT = 4,
	AUTHENTICATE_ACCOUNT = 5,
};

struct CommandData
{
	string messageType;
	string data1;
	string data2;
};

void PrintToConsoIe(const std::string& msg)
{
	printf("%s", msg.c_str());
}

void ClearConso1eLine()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO  info;
	GetConsoleScreenBufferInfo(hConsole, &info);
	COORD position = { 0, info.dwCursorPosition.Y };
	printf("\33[2K\r");
	SetConsoleCursorPosition(hConsole, position);
}

void PrintConso1eLine(const std::string& msg)
{
	printf("%s\n", msg.c_str());
}

int Initialize()
{
	WSADATA wsaData;							// holds Winsock data

	struct addrinfo* infoResult = NULL;			// Holds the address information of our server
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	const char* sendbuf = "Hello World!";		// The messsage to send to the server

	char recvbuf[DEFAULT_BUFLEN];				// The maximum buffer size of a message to send
	int result;									// code of the result of any command we use
	int recvbuflen = DEFAULT_BUFLEN;			// The length of the buffer we receive from the server

	// Stp #1 Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error: %d\n", result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Step #2 Resolve the server address and port
	result = getaddrinfo(SERVER, DEFAULT_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}

	// Step #3 Attempt to connect to an address until one succeeds
	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		g_ServerSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (g_ServerSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		result = connect(g_ServerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				break;
			}
			closesocket(g_ServerSocket);
			g_ServerSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	// Client Non-blocking setup
	DWORD NonBlock = 1;
	result = ioctlsocket(g_ServerSocket, FIONBIO, &NonBlock);
	if (result == SOCKET_ERROR)
	{
		printf("ioctsocket() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	printf("Successfully connected to the chat server.\n\n");
	PrintHelpMessage();

	freeaddrinfo(infoResult);

	if (g_ServerSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	return 0;
}

int Disconnect()
{
	// Step #5 shutdown the connection since no more data will be sent
	int result = shutdown(g_ServerSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(g_ServerSocket);
		WSACleanup();
		return 1;
	}

	// Step #7 cleanup
	closesocket(g_ServerSocket);
	WSACleanup();
	return 0;
}

Buffer g_Buffer;
int bytesRECV;
int CheckForReceive()
{
	DWORD flags = 0;
	int result = recv(g_ServerSocket, (char*)(&g_Buffer.GetData()[bytesRECV]), DEFAULT_BUFLEN, flags);
	if (result == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAEWOULDBLOCK)
		{
			// We can ignore this, it isn't an actual error.
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			return 1;
		}
	}
	else if (result > 0)
	{
		unsigned int packetLength = g_Buffer.ReadUInt32LE();
		unsigned int messageType = g_Buffer.ReadUInt32LE();
		unsigned int messageLength = g_Buffer.ReadUInt32LE();
		std::string msg = g_Buffer.ReadString(messageLength);
		ClearConso1eLine();
		PrintConso1eLine(msg);
		g_Buffer.Reset();
	}
	return 0;
}

std::string ReadNextWord(std::string str, int startIndex)
{
	int stringLength = str.length();

	std::string word = "";
	for (int i = startIndex; i < str.length(); ++i)
	{
		if (str[i] == ' ')
			return word;

		word += str[i];
	}

	return word;
}

CommandData CreateCommandData(std::string str)
{
	CommandData cmd;
	char space = ' ';
	std::size_t cmdPos = str.find(space, 0);
	cmd.messageType = str.substr(0, cmdPos);

	if (cmdPos > str.length())
		return cmd;

	std::size_t roomPos = str.find(space, cmdPos + 1);
	cmd.data1 = str.substr(cmdPos + 1, roomPos - cmdPos - 1);

	if (roomPos > str.length())
		return cmd;

	cmd.data2 = str.substr(roomPos + 1);

	return cmd;
}

void PrintHelpMessage()
{
	printf("Welcome to LG Chat, now with Databse Authentication!!\n");
	printf("Here is a list of comamnds:\n");
	printf("  /join <room>			- Joins a room (no spaces)\n");
	printf("  /leave <room>			- Leaves a room\n");
	printf("  /send <room> <message>	- Sends a message to a room\n");
	printf("  /help				- Displays this help message\n");
	printf("  /quit				- Quits the program\n");
	printf("  /create <email> <password>    - Create an account on the database server.\n");
	printf("  /auth <email> <password>      - Authenticate your account on the database server.\n");
	printf("\n");
}

int ParseAndSendMessage(std::string& message)
{
	CommandData cmd = CreateCommandData(message);

	MessageType type = MessageType::NONE;

	if (strcmp(cmd.messageType.c_str(), "/help") == 0)
	{
		PrintHelpMessage();
		return 0;
	}
	else if (strcmp(cmd.messageType.c_str(), "/quit") == 0)
	{
		g_IsRunning = false;
		return 0;
	}
	else if (strcmp(cmd.messageType.c_str(), "/join") == 0)
	{
		type = JOIN_A_ROOM;
	}
	else if (strcmp(cmd.messageType.c_str(), "/send") == 0)
	{
		type = SEND_THE_MSAGE_TO_ROOM;
	}
	else if (strcmp(cmd.messageType.c_str(), "/leave") == 0)
	{
		type = LEAVE_THE_ROM;
	}
	else if (strcmp(cmd.messageType.c_str(), "/create") == 0)
	{
		type = CREATE_ACCOUNT;
	}
	else if (strcmp(cmd.messageType.c_str(), "/auth") == 0)
	{
		type = AUTHENTICATE_ACCOUNT;
	}

	Buffer buffer(512);
	uint32_t packetLength = 8 + cmd.data1.length() + 4;
	if (type == SEND_THE_MSAGE_TO_ROOM || type == CREATE_ACCOUNT || type == AUTHENTICATE_ACCOUNT)
	{
		packetLength += cmd.data2.length() + 4;
	}
	buffer.WriteUInt32BE(packetLength);
	buffer.WriteUInt32BE(type);
	buffer.WriteUInt32BE(cmd.data1.length());
	buffer.WriteString(cmd.data1);
	if (type == SEND_THE_MSAGE_TO_ROOM || type == CREATE_ACCOUNT || type == AUTHENTICATE_ACCOUNT)
	{
		buffer.WriteUInt32BE(cmd.data2.length());
		buffer.WriteString(cmd.data2);
	}
	int result = send(g_ServerSocket, (const char*)buffer.GetData(), packetLength, 0);

#if ENABLE_DEBUG
	printf("Bytes Sent: %d\n", result);
#endif

	return 0;
}

#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define DISABLE_NEWLINE_AUTO_RETURN  0x0008

int main(int argc, char** argv)
{
	DWORD l_mode;
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hStdout, &l_mode);
	SetConsoleMode(hStdout, l_mode |
		ENABLE_VIRTUAL_TERMINAL_PROCESSING |
		DISABLE_NEWLINE_AUTO_RETURN);

	int result = 0;
	result = Initialize();
	if (result != 0)
	{
		return result;
	}

	std::string message = "";

	g_IsRunning = true;

	do {

		if (CheckForReceive() != 0)
			g_IsRunning = false;

		if (_kbhit())
		{
			char ch = _getch();

			switch (ch)
			{
			case 27:
				g_IsRunning = false;
				break;

			case 8:
				message.pop_back();
				ClearConso1eLine();
				PrintToConsoIe(message);
				break;

			case 13:
				ClearConso1eLine();
				ParseAndSendMessage(message);
				message = "";
				break;

			default:
				message.push_back(ch);
				ClearConso1eLine();
				PrintToConsoIe(message);
				break;
			}
		}

	} while (g_IsRunning);


	result = Disconnect();
	if (result != 0)
		return result;

	return 0;
}