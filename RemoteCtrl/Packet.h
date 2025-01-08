#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSUM(0)
	{
	}

	CPacket(WORD nCmd, const BYTE* pDData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4; //包长
		sCmd = nCmd; //命令
		if (nSize > 0)
		{
			strData.resize(nSize); //数据
			memcpy((void*)strData.c_str(), pDData, nSize); //数据
		}
		else
		{
			strData.clear();
		}
		sSUM = 0;
		for (size_t i = 0; i < strData.size(); i++)
		{
			sSUM += BYTE(strData[i]) & 0xFF;
		}
	}

	CPacket(const CPacket& packet)
	{
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSUM = packet.sSUM;
	}

	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2; //跳过包头
				break;
			}
		} //找包头
		if (i + 4 + 2 + 2 > nSize) //包头+包长+命令+校验和
		{
			nSize = 0; //没有找到包头
			return;
		} //没有找到包头或者包数据不全

		nLength = *(DWORD*)(pData + i); //包长
		i += 4; //跳过包长

		if (nLength + i > nSize) //包未完全接收到，就返回，等待下次接收
		{
			nSize = 0;
			return;
		} //包未完整


		sCmd = *(WORD*)(pData + i); //命令
		i += 2; //跳过命令

		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2); //包长-命令-校验和
			memcpy((void*)strData.c_str(), pData + i, nLength - 2 - 2); //包数据
			i += nLength - 2 - 2; //跳过包数据
		}

		sSUM = *(WORD*)(pData + i); //校验和
		i += 2; //跳过校验和
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) //校验和
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSUM)
		{
			nSize = i; //head2 包长的长度4  包长（命令，数据，校验）
			return;
		}
		nSize = 0;
	}

	~CPacket()
	{
	}

	CPacket& operator=(const CPacket& packet)
	{
		if (this == &packet)
		{
			return *this;
		}
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSUM = packet.sSUM;
		return *this;
	}

	int Size() //包大小
	{
		return nLength + 6;
	}

	const char* Data() //包数据
	{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSUM;
		return strOut.c_str();
	}

public:
	WORD sHead; //包头 固定为FE FF
	DWORD nLength; //包长(从命令到校验和)
	WORD sCmd; //命令
	std::string strData; //包数据
	WORD sSUM; //校验和
	std::string strOut; //整个包的数据
};
#pragma pack(pop)
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction; //点击，移动，双击
	WORD nButton; //左键，右键、中键
	POINT ptXY; //鼠标坐标
} MOUSEEV, *PMOUSEEV;


typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasFile = TRUE;
		memset(szFIleName, 0, sizeof(szFIleName));
	}

	BOOL IsInvalid; //是否是无效的 TRUE 是 FALSE 不是
	BOOL IsDirectory; //是否是目录 TRUE 是 FALSE 不是
	BOOL HasFile; //是否有文件 TRUE 是 FALSE 不是
	char szFIleName[256];
} FILEINFO, *PFILEINFO;
