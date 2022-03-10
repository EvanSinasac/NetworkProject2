#pragma once

#include <map>
#include <vector>
#include <string>

#include "ChatClientInfo.h"

enum ChatProgramMessage
{
	kAllGood = 0,
	kUnknownError = 1,
	kRoomIsEmpty = 2,
	kRoomDoesNotExist = 3,
	kClientAlreadyInRoom = 4,
	kClientNotInRoom = 5,
};

struct ChatRoom
{
	ChatRoom(const std::string& roomName);
	std::string name;
	std::vector<ChatClientInfo*> clients;
};

class ChatProgram
{
public:
	ChatProgram();
	~ChatProgram();

	ChatProgramMessage JoinRoom(const std::string &roomName, ChatClientInfo* client);
	ChatProgramMessage LeaveRoom(const std::string& roomName, ChatClientInfo* client);
	std::vector<ChatClientInfo*> GetClientsInRoom(const std::string& roomName);

private:
	void CreateRoomIfNotExist(const std::string& roomName);
	bool RoomExist(const std::string& roomName);

	std::map<std::string, ChatRoom*> m_ChatRooms;
};