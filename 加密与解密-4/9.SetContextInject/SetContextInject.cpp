/*-----------------------------------------------------------------------
��12��  ע�뼼��
����������ܣ����İ棩��
(c)  ��ѩѧԺ www.kanxue.com 2000-2018
-----------------------------------------------------------------------*/

// SetContextInject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <TLHELP32.H>
#include <Psapi.h>

#pragma comment(lib,"psapi.lib")

typedef struct _INJECT_DATA 
{
	BYTE ShellCode[0x30];
	ULONG_PTR AddrofLoadLibraryA;
	PBYTE lpDllPath;
	ULONG_PTR OriginalEIP;
	char szDllPath[MAX_PATH];
}INJECT_DATA;

#ifdef _WIN64
EXTERN_C VOID ShellCodeFun64(VOID);
#else
VOID ShellCodeFun(VOID);
#endif

ULONG_PTR GetModuleHandleInProcess( DWORD processID , char *szModuleName );
DWORD ProcesstoPid(char *Processname);
BOOL InjectModuleToProcessBySetContext(DWORD dwPid, char *szDllFullPath);
VOID PrepareShellCode(BYTE *pShellCode);

int main(int argc, char* argv[])
{
#ifdef _WIN64
	char szDllPath[MAX_PATH] = "F:\\Program2016\\DllInjection\\MsgDll64.dll" ;
	DWORD dwPid = ProcesstoPid("HostProc64.exe");
#else
	// char szDllPath[MAX_PATH] = "F:\\Program2016\\DllInjection\\MsgDll.dll" ; 
	char szDllPath[MAX_PATH] = "C:\\tool\\PEDIY_BOOK4\\chap12\\MsgDll.dll" ; 
	//DWORD dwPid = ProcesstoPid("HostProc.exe");
	DWORD dwPid = ProcesstoPid("mintty.exe");
#endif
	if (dwPid == 0)
	{
		printf("[-] Target Process Not Found!\n");
		return 0;
	}
	printf("[*] Target Process Pid = %d\n",dwPid);
	BOOL bResult = InjectModuleToProcessBySetContext(dwPid,szDllPath);
	printf("[*] Result = %d\n",bResult);
	return 0;
}




#ifndef _WIN64
__declspec (naked)
VOID ShellCodeFun(VOID)
{
	__asm
	{
		push eax //ռλ,һ���������ת��ַ
		pushad   //��С0x20
		pushfd   //��С0x04
		call L001
L001:
		pop ebx
		sub ebx,8
		push dword ptr ds:[ebx+0x34] //szDllPath
		call dword ptr ds:[ebx+0x30] //LoadLibraryA
		mov eax,dword ptr ds:[ebx+0x38] //OriginalEIP
		xchg eax,[esp+0x24] //��ԭ����EIP������ջ��
		popfd
		popad
		retn //jmp to OriginalEIP
		nop
		nop
		nop
		nop
		nop
	}
}
#endif


VOID PrepareShellCode(BYTE *pOutShellCode)
{
#ifdef _WIN64
	//x64����ʹ��__asm�ˣ�ֻ��ֱ��дShellcode�򵥶�д��asm�ļ���
	/*
	BYTE ShellCode64[]=
		"\x50"					//push    rax
		"\x53"					//push    rbx
		"\x9c"					//pushfq
		"\xe8\x00\x00\x00\x00"  //call  next
		"\x5b"					//pop     rbx
		"\x66\x83\xe3\x00"      //and     bx,0
		"\x48\x8b\x4b\x38"      //mov     rcx,qword ptr [rbx+38h] ; lpDllPath
		"\xff\x53\x30"          //call    qword ptr [rbx+30h]		;AddrofLoadLibraryA
		"\x48\x8b\x43\x40"      //mov     rax,qword ptr [rbx+40h]	;OriginalEIP
		"\x48\x87\x44\x24\x10"  //xchg    rax,qword ptr [rsp+10h]	;saved retnaddr
		"\x9d"					//popfq
		"\x5b"					//pop     rbx
		"\xc3"					//ret
		"\x90"					//nop
	;

	memcpy(pOutShellCode,ShellCode64,33);
	*/
	BYTE *pShellcodeStart = (BYTE*)ShellCodeFun64;
#else
	BYTE *pShellcodeStart = (BYTE*)ShellCodeFun;
#endif

	BYTE *pShellcodeEnd = 0 ;
	SIZE_T ShellCodeSize = 0 ;
	if (pShellcodeStart[0] == 0xE9) //[popexizhi: OxE9��debug�ı�־��]
	{
		//Debugģʽ�£�������ͷ��һ����תָ�����ȡ����������ַ
		pShellcodeStart = pShellcodeStart + *(ULONG*)(pShellcodeStart +1 ) + 5;
	}
	
	//����Shellcode������־
	pShellcodeEnd = pShellcodeStart;
	while (memcmp(pShellcodeEnd,"\x90\x90\x90\x90\x90",5) != 0) //[popexizhi: ?why--ok, shellcode stop ������5��nop��Ϊ��������]
	{
		pShellcodeEnd++;
	}
	
	ShellCodeSize = pShellcodeEnd - pShellcodeStart;
	printf("[*] Shellcode Len = %d\n",ShellCodeSize);
	memcpy(pOutShellCode,pShellcodeStart,ShellCodeSize);


}
BOOL InjectModuleToProcessBySetContext(DWORD dwPid, char *szDllFullPath)
{
	SIZE_T   dwRet = 0 ;  
    BOOL    bStatus = FALSE ;  
    PBYTE   lpData = NULL ;  
    SIZE_T  uLen = 0x1000;
	INJECT_DATA Data;
	HANDLE hProcess,hThread;
	DWORD i = 0 ;
	

	//1.��ȡĿ�������LoadLibraryA�ĵ�ַ [popexizhi:why, kernel32.dll]
	//֮������ô��ȡ���ǿ�����ASLR��Ӱ�죬��ʱĿ�������kernel32.dll�ļ���λ�ò�һ���뱾������ͬ [popexizhi: ASLR��? ��ַ�ռ䲼���������address space layout randomization��ASLR��]
	ULONG_PTR uKernelBaseInTargetProc = GetModuleHandleInProcess(dwPid,"kernel32.dll"); //[popexizhi: ���dwPid��kernel32.dll�ļ���λ��]
	ULONG_PTR uKernelBaseInCurProc = (ULONG_PTR)GetModuleHandle("kernel32.dll");//
	ULONG_PTR uLoadLibraryAddrInCurProc =  (ULONG_PTR)GetProcAddress((HMODULE)uKernelBaseInTargetProc,"LoadLibraryA");//��ѯdwPid�е�kernell32.dll��LoadLibraryA�ĵ�ַ,
	//��ʽ���ӵ� DLL �Ľ��̵��� GetProcAddress ����ȡ DLL ���������ĵ�ַ�� ʹ�÷��صĺ���ָ����� DLL ����
	ULONG_PTR uLoadLibraryAddrInTargetProc = uLoadLibraryAddrInCurProc -  uKernelBaseInCurProc + uKernelBaseInTargetProc; //[popexizhi: ����Ǽ��� ��ǰ���̽��̵�kernel32.dll�� LoadLibraryA��ַ]
	printf("[*] Ŀ������� LoadLibraryA Addr = 0x%p\n",uLoadLibraryAddrInTargetProc); //[popexizhi:? Ϊʲô��Ŀ����̣�Ŀ����̵Ĳ��� uLoadLibraryAddrInCurProc ��debug��һ�£���������ȷ��,
	//����uLoadLibraryAddrInTargetProc��uLoadLibraryAddrInCurProc��һ����Ӧ����û�л����ģ����Ӧ�ò����?! ���ǲ���������㷨!!!  ]

	//2.��ȡĿ������е��߳��б�
    THREADENTRY32 te32 = {sizeof(THREADENTRY32)} ;
	DWORD dwTidList[1024]={0};
	DWORD Index = 0 ;
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0) ;  
    if (hThreadSnap == INVALID_HANDLE_VALUE)   
        return FALSE ;   
	
    bStatus = FALSE ; 
	printf("[*] ��ʼö��Ŀ������е��߳�.\n");
    // ö�������߳�  
    if (Thread32First(hThreadSnap, &te32))  
    {  
        do{  
            // �ж��Ƿ�Ŀ������е��߳�  
            if (te32.th32OwnerProcessID == dwPid)  
            {
				bStatus = TRUE;
                dwTidList[Index++] = te32.th32ThreadID;
            }   
			
        }while (Thread32Next ( hThreadSnap, &te32 )) ;  
    }  
	
    CloseHandle (hThreadSnap) ;  
	
	if (!bStatus)
	{
		printf("[-] �޷��õ�Ŀ����̵��߳��б�!\n");
		return FALSE;
	}
	printf("[*] �߳�ö����ϣ����� %d ���߳�.\n",Index);

	//3. ��Ŀ�����  �������ڴ棬д��Shellcode�Ͳ���
	ULONG_PTR uDllPathAddr = 0;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid) ;  
	if (hProcess == NULL)
	{
		printf("[-] �޷���Ŀ�����!\n");
		return FALSE;
	}

	//3.���δ��̣߳���ȡContext
	bStatus = FALSE ;
	CONTEXT Context;
	ULONG_PTR uEIP = 0 ;
	for (i=0;i<Index;i++)
	{
		hThread = OpenThread(THREAD_ALL_ACCESS,FALSE,dwTidList[i]);
		if (hThread == NULL)
		{
			printf("[-] ���߳� %d ʧ��!\n",dwTidList[i]);
			continue;
		}
		
		printf("[*] ���߳� %d �ɹ�.\n",dwTidList[i]);

		//��ͣ�߳�
		DWORD dwSuspendCnt = SuspendThread(hThread);
		if (dwSuspendCnt == (DWORD)-1)
		{
			printf("[-] ��ͣ�߳� %d ʧ��!\n",dwTidList[i]);
			CloseHandle(hThread);
			continue;
		}
		
		printf("[*] ��ͣ�߳� %d �ɹ� Cnt = %d.\n",dwTidList[i],dwSuspendCnt);
		//��ȡContext
		ZeroMemory(&Context,sizeof(CONTEXT));
		Context.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(hThread,&Context))
		{
			printf("[-] �޷���ȡ�߳� %d ��Context!\n",dwTidList[i]);
			CloseHandle(hThread);
			continue;
		}
		
#ifdef _WIN64
		uEIP = Context.Rip;
#else
		uEIP = Context.Eip;
#endif
		printf("[*] ��ȡ�߳� %d ��Context�ɹ� EIP = 0x%p\n",dwTidList[i],uEIP);
		
		// ����ռ�  
        lpData = (PBYTE)VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (lpData == NULL)
		{
			printf("[-] ��Ŀ����������ڴ�ʧ��!\n");
			CloseHandle(hThread);
			continue;
		}
		
		printf("[*] ��Ŀ������������ڴ�ɹ�, lpData = 0x%p\n",lpData);

		//����ShellCode
		ZeroMemory(&Data,sizeof(INJECT_DATA));
		PrepareShellCode(Data.ShellCode);
		lstrcpy(Data.szDllPath,szDllFullPath); //Dll·��

		Data.AddrofLoadLibraryA = uLoadLibraryAddrInTargetProc; //LoadLibraryA�ĵ�ַ
		Data.OriginalEIP = uEIP; //ԭʼ��EIP��ַ
		Data.lpDllPath = lpData + FIELD_OFFSET(INJECT_DATA,szDllPath) ; //szDllPath��Ŀ������е�λ��
		printf("[*] ShellCode������.\n");

		//��Ŀ�����д��ShellCode
		if (!WriteProcessMemory(hProcess, lpData, &Data, sizeof(INJECT_DATA), &dwRet))
		{
			printf("[-] ��Ŀ�����д��ShellCodeʧ��!\n");
			CloseHandle(hThread);
			CloseHandle(hProcess);
			return FALSE;
		}

		printf("[*] ��Ŀ�����д��ShellCode�ɹ�.\n");
		
		//�����̵߳�Context,ʹEIPָ��ShellCode��ʼ��ַ
#ifdef _WIN64
		Context.Rip = (ULONG_PTR)lpData;
#else
		Context.Eip = (ULONG_PTR)lpData;
#endif
		//����Context
		if (!SetThreadContext(hThread,&Context))
		{
			printf("[-] �޷������߳� %d ��Context!\n",dwTidList[i]);
			CloseHandle(hThread);
			continue;
		}

		printf("[*] �����߳� %d ��Context�ɹ�.\n",dwTidList[i]);

		//�ָ��߳�ִ��
		dwSuspendCnt = ResumeThread(hThread);	
		if (dwSuspendCnt == (DWORD)-1)
		{
			printf("[-] �ָ��߳� %d ʧ��!\n",dwTidList[i]);
			CloseHandle(hThread);
			continue;
		}
		
		printf("[*] �ָ��߳� %d �ɹ�. Cnt = %d\n",dwTidList[i],dwSuspendCnt);

		bStatus = TRUE;
		CloseHandle(hThread);

		//Sleepһ��ʱ�䣬��������һ�̲߳�������ȷ���ɹ���
		Sleep(1000);


	}

	CloseHandle(hProcess);
	printf("[*] ����ȫ�����.\n");

	return bStatus;	
}

ULONG_PTR GetModuleHandleInProcess( DWORD processID , char *szModuleNameToFind )
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;
	char *pCompare = NULL ;
	ULONG_PTR uResult = 0 ;
	
    // Print the process identifier.
	
    printf( "\nProcess ID: %u\n", processID );
	
    // Get a list of all the modules in this process.
	
    hProcess = OpenProcess( 
		PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,
		FALSE, 
		processID 
		);
    if (NULL == hProcess)
        return 0;
	
    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))//[popexizhi:EnumProcessModules����ƣ�
		// ��Ҫ��Ϊ�����������Ƴ����ڱ����ȡ�������̵�ģ����Ϣʱʹ�õġ����Ŀ����̵�ģ���б��Ѿ��𻵣�������δ��ʼ������ô��EnumProcessModules���ܻ�ִ��ʧ�ܣ����߷��ش������Ϣ��]
    {
        for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            char szModName[MAX_PATH];
			
            // Get the full path to the module's file.
			
            if ( GetModuleFileNameEx(hProcess, hMods[i], szModName,
				sizeof(szModName)/sizeof(char)))
            {
                // Print the module name and handle value.
				pCompare = strrchr(szModName,'\\');
				pCompare = (pCompare == NULL) ? szModName:(pCompare+1);
				if (stricmp(pCompare,szModuleNameToFind) == 0)
				{
					uResult = (ULONG_PTR)hMods[i];
					break;
				}
            }
        }
    }
	
    CloseHandle( hProcess );

	return uResult;
}

DWORD ProcesstoPid(char *Processname) //����ָ�����̵�PID(Process ID)
{
	HANDLE hProcessSnap=NULL;
	DWORD ProcessId=0;
	PROCESSENTRY32 pe32={0};
	hProcessSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0); //�򿪽��̿���
	if(hProcessSnap==(HANDLE)-1)
	{
		printf("\nCreateToolhelp32Snapshot() Error: %d",GetLastError());
		return 0;
	}
	pe32.dwSize=sizeof(PROCESSENTRY32);
	if(Process32First(hProcessSnap,&pe32)) //��ʼö�ٽ���
	{
		do
		{
			if(!stricmp(Processname,pe32.szExeFile)) //�ж��Ƿ���ṩ�Ľ�������ȣ��ǣ����ؽ��̵�ID
			{
				ProcessId=pe32.th32ProcessID;
				break;
			}
		}
		while(Process32Next(hProcessSnap,&pe32)); //����ö�ٽ���
	}
	else
	{
		printf("\nProcess32First() Error: %d",GetLastError());
		return 0;
	}
	CloseHandle(hProcessSnap); //�ر�ϵͳ���̿��յľ��
	return ProcessId;
}