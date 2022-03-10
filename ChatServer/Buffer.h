#pragma once

#include <vector>
#include <string>

using std::vector;
using std::string;

class Buffer
{
public:
	Buffer(size_t size);
	Buffer(void);

	void Initialize(size_t size);

	void Reset(void);

	void WriteUInt32BE(uint32_t data);
	void WriteUInt32BE(uint32_t data, int index);

	void WriteUInt16BE(uint16_t data);
	void WriteUInt16BE(uint16_t data, int index);

	void WriteString(const string& data);
	void WriteString(const string& data, int index);

	uint32_t ReadUInt32LE(void);
	uint32_t ReadUInt32LE(int index);

	uint16_t ReadUInt16LE(void);
	uint16_t ReadUInt16LE(int index);

	string ReadString(size_t length);
	string ReadString(size_t length, int index);

	uint8_t* GetData(void);

private:
	vector<uint8_t> m_Data;
	void CheckBufferCapacity(size_t size);
	void GrowVector(void);
	unsigned int m_ReadIndex;
	unsigned int m_WriteIndex;
};