#pragma once
#include <string>
#include <atlimage.h>

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

	static int Byte2Image(CImage& image, const std::string strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); //分配内存
		if (hMem == NULL)
		{
			AfxMessageBox("内存分配失败");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL; //创建流对象
		HRESULT hRet = CreateStreamOnHGlobal(hMem, true, &pStream); //创建流对象
		if (hRet == S_OK) //创建成功
		{
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length); //写入数据
			LARGE_INTEGER bg = {0};
			pStream->Seek(bg, STREAM_SEEK_SET, NULL); //设置流的位置
			if ((HBITMAP)image != NULL)
				image.Destroy(); //销毁图片
			image.Load(pStream); //加载图片
		}
		return hRet; //返回结果
	}
};
