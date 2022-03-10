#include "ChatProgram.h"

ChatRoom::ChatRoom(const std::string& roomName)
	: name(roomName)
	, clients(std::vector<ChatClientInfo*>())
{

}

ChatProgram::ChatProgram()
{

}

ChatProgram::~ChatProgram()
{

}

ChatProgramMessage ChatProgram::JoinRoom(const std::string& roomName,ChatClientInfo* client)
{
	CreateRoomIfNotExist(roomName);

	ChatRoom* room = m_ChatRooms[roomName];
	if (std::find(std::begin(room->clients), std::end(room->clients), client) != room->clients.end())
	{
		printf("ERROR: Client attempted to join the \"%s\" room, but they are already in it.\n", roomName.c_str());
		return kClientAlreadyInRoom;
	}

	room->clients.push_back(client);

	return kAllGood;
}

ChatProgramMessage ChatProgram::LeaveRoom(const std::string& roomName, ChatClientInfo* client)
{
	if (!RoomExist(roomName))
	{
		printf("ERROR: Client trying to leave the \"%s\" room, but that room does not exist.\n", roomName.c_str());
		return kRoomDoesNotExist;
	}

	ChatRoom* room = m_ChatRooms[roomName];
	std::vector<ChatClientInfo*>::iterator clientIter = std::find(std::begin(room->clients), std::end(room->clients), client);
	if (clientIter == room->clients.end())
	{
		printf("ERROR: Client trying to leave the \"%s\" room, but the client is not in that room.\n", roomName.c_str());
		return kClientNotInRoom;
	}

	room->clients.erase(clientIter);

	return kAllGood;
}

std::vector<ChatClientInfo*> ChatProgram::GetClientsInRoom(const std::string& roomName)
{
	std::vector<ChatClientInfo*> clients;
	if (RoomExist(roomName))
	{
		ChatRoom* room = m_ChatRooms[roomName];
		return room->clients;
	}

	return clients;
}

void ChatProgram::CreateRoomIfNotExist(const std::string& roomName)
{
	if (RoomExist(roomName))
		return;

	printf("> Created room %s\n", roomName.c_str());

	m_ChatRooms.insert(std::pair<std::string, ChatRoom*>
		(roomName, new ChatRoom(roomName)));
}

bool ChatProgram::RoomExist(const std::string& roomName)
{
	auto roomIter = m_ChatRooms.find(roomName);
	return roomIter != m_ChatRooms.end();
}