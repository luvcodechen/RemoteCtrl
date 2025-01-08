#pragma once
class CMyTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) //将数据转换为16进制字符串
	{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) //将数据转换为16进制字符串
		{
			char buf[8] = "";
			if (i > 0 && i % 16 == 0) //每16个字节换行
			{
				strOut += "\n";
			}
			snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF); //将一个字节转换为16进制字符串
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}
};
