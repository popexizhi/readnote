// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
//#include "stdafx.h"
#include <Windows.h>

//���������ֱ��ת����lpkOrg.dll
#pragma comment(linker, "/EXPORT:LpkInitialize=lpkOrg.LpkInitialize,@1")
#pragma comment(linker, "/EXPORT:LpkTabbedTextOut=lpkOrg.LpkTabbedTextOut,@2")
#pragma comment(linker, "/EXPORT:LpkDllInitialize=lpkOrg.LpkDllInitialize,@3")
#pragma comment(linker, "/EXPORT:LpkDrawTextEx=lpkOrg.LpkDrawTextEx,@4")
#pragma comment(linker, "/EXPORT:LpkEditControl=lpkOrg.LpkEditControl,@5")
#pragma comment(linker, "/EXPORT:LpkExtTextOut=lpkOrg.LpkExtTextOut,@6")
#pragma comment(linker, "/EXPORT:LpkGetCharacterPlacement=lpkOrg.LpkGetCharacterPlacement,@7")
#pragma comment(linker, "/EXPORT:LpkGetTextExtentExPoint=lpkOrg.LpkGetTextExtentExPoint,@8")
#pragma comment(linker, "/EXPORT:LpkPSMTextOut=lpkOrg.LpkPSMTextOut,@9")
#pragma comment(linker, "/EXPORT:LpkUseGDIWidthCache=lpkOrg.LpkUseGDIWidthCache,@10")
#pragma comment(linker, "/EXPORT:ftsWordBreak=lpkOrg.ftsWordBreak,@11")


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI TreadWorking(LPVOID lpParameters);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DWORD WINAPI ThreadWorking(LPVOID lpParameters)
{
	MessageBox(NULL, L"Fake lpk loaded!", L"Notice", MB_OK);
	OutputDebugString(L"LPK.dll is working.\n");
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  dwReason,
                       LPVOID lpReserved
					 )
{
	if(dwReason == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, ThreadWorking, NULL, 0, NULL);
		DisableThreadLibraryCalls(hModule);
	}
	else if(dwReason == DLL_PROCESS_DETACH)
	{
	}

	return TRUE;
}


