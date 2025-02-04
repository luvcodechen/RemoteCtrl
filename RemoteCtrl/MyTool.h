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

	static void ShowError()
	{
		LPWSTR lpMessageBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, GetLastError(),
		              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		              (LPWSTR)&lpMessageBuf, 0, NULL); //获取错误信息
		OutputDebugString(lpMessageBuf); //输出错误信息

		LocalFree(lpMessageBuf); //释放内存
		exit(0);
	}

	static bool IsAdmin()
	{
		HANDLE hToken = NULL; //令牌句柄
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) //打开进程令牌
		{
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve; //令牌提升
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) //获取令牌信息
		{
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve))
		{
			return eve.TokenIsElevated;
		}
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}

	static bool RunAsAdmin()
	{
		//本地策略组 开启administrator账户 禁止空密码只能登录本地控制台
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi = {0};
		TCHAR sPath[MAX_PATH] = _T("");
		// GetCurrentDirectory(MAX_PATH, sPath);//获取当前目录
		GetModuleFileName(NULL, sPath, MAX_PATH); //获取当前程序路径

		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL,
		                                   (LPWSTR)(LPCWSTR)sPath,
		                                   CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret)
		{
			CMyTool::ShowError();
			MessageBox(NULL, sPath, _T("程序错误"), 0);
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE); //等待进程结束
		CloseHandle(pi.hProcess); //关闭进程句柄
		CloseHandle(pi.hThread); //关闭线程句柄
		return true;
	}

	static bool WriteRegisterTable(const CString strPath)
	{//通过修改注册表实现开机自启
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH); //获取当前程序路径
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE)
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
			return false;
		}
		HKEY hkey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hkey);
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hkey);
			MessageBox(NULL, _T("设置开机自启失败"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
			return false;
		}
		ret = RegSetValueEx(hkey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath,
		                    strPath.GetLength() * sizeof(TCHAR)); //写入注册表
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hkey);
			MessageBox(NULL, _T("设置开机自启失败,是否权限不足、\r\n程序启动失败"), _T("错误"), MB_TOPMOST | MB_ICONERROR);
			return false;
		}
		RegCloseKey(hkey);
		return true;
	}

	static BOOL WriteStartupDir(const CString strPath)
	{//通过复制文件实现开机自启
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH); //获取当前程序路径
		return CopyFile(sPath, strPath, FALSE);
	}
	static bool Init()
	{//初始化MFC
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr)
		{
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
};
