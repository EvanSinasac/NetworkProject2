#include "Buffer.h"

Buffer::Buffer(size_t size)
	: m_ReadIndex(0)
	, m_WriteIndex(0)
{
	Initialize(size);
}

Buffer::Buffer(void)
	: m_ReadIndex(0)
	, m_WriteIndex(0)
{
	Initialize(512);
}

void Buffer::Initialize(size_t size)
{
	m_Data.resize(size, 0);
}

void Buffer::Reset(void)
{
	m_ReadIndex = 0;
	m_WriteIndex = 0;
}

void Buffer::WriteUInt32BE(uint32_t data)
{
	WriteUInt32BE(data, m_WriteIndex);
	m_WriteIndex += sizeof(uint32_t);
}

void Buffer::WriteUInt32BE(uint32_t data, int index)
{
	CheckBufferCapacity(sizeof(data));
	m_Data[index] = data;
	m_Data[index + 1] = data >> 8;
	m_Data[index + 2] = data >> 16;
	m_Data[index + 3] = data >> 24;
}

void Buffer::WriteUInt16BE(uint16_t data)
{
	WriteUInt16BE(data, m_WriteIndex);
	m_WriteIndex += sizeof(uint16_t);
}

void Buffer::WriteUInt16BE(uint16_t data, int index)
{
	CheckBufferCapacity(sizeof(data));
	m_Data[index] = data;
	m_Data[index + 1] = data >> 8;
}

void Buffer::WriteString(const string& data)
{
	WriteString(data, m_WriteIndex);
	m_WriteIndex += data.length();
}

void Buffer::WriteString(const string& data, int index)
{
	size_t strLength = data.length();
	CheckBufferCapacity(strLength);
	memcpy(&m_Data[index], &data[0], strLength);
}

uint32_t Buffer::ReadUInt32LE(void)
{
	uint32_t value = ReadUInt32LE(m_ReadIndex);
	m_ReadIndex += sizeof(uint32_t);
	return value;
}

uint32_t Buffer::ReadUInt32LE(int index)
{
	uint32_t value = m_Data[index];
	value |= m_Data[index + 1] << 8;
	value |= m_Data[index + 2] << 16;
	value |= m_Data[index + 3] << 24;
	return value;
}

uint16_t Buffer::ReadUInt16LE(void)
{
	uint16_t value = ReadUInt16LE(m_ReadIndex);
	m_ReadIndex += sizeof(uint16_t);
	return value;
}

uint16_t Buffer::ReadUInt16LE(int index)
{
	uint16_t value = m_Data[index];
	value |= m_Data[index + 1] << 8;
	return value;
}

string Buffer::ReadString(size_t length)
{
	string value = ReadString(length, m_ReadIndex);
	m_ReadIndex += length;
	return value;
}

string Buffer::ReadString(size_t length, int index)
{
	string value = "";
	for (int i = 0; i < length; i++)
	{
		value += m_Data[index + i];
	}
	m_ReadIndex += length;
	return value;
}

void Buffer::CheckBufferCapacity(size_t size)
{
	if (m_WriteIndex + size >= m_Data.size())
		GrowVector();
}

void Buffer::GrowVector(void)
{
	m_Data.resize(m_Data.size() * 2, 0);
}

uint8_t* Buffer::GetData(void)
{
	return m_Data.data();
}