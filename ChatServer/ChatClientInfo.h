#pragma once

#include <WinSock2.h>
#include "Buffer.h"

struct ChatClientInfo
{
	SOCKET socket;
	Buffer buffer;
	int packetLength;
	int bytesRECV;
};