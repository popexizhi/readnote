/*-----------------------------------------------------------------------
��12��  ע�뼼��
����������ܣ����İ棩��
(c)  ��ѩѧԺ www.kanxue.com 2000-2018
-----------------------------------------------------------------------*/

// RtlCreateInject.cpp : Defines the entry point for the console application.
// Tested On:WinXP SP3/Win7 x86,x64

#include "stdafx.h"
#include <windows.h>
#include <TLHELP32.H>

typedef LONG NTSTATUS;

#define STATUS_SUCCESS                      ((NTSTATUS) 0x00000000L)
#define STATUS_UNSUCCESSFUL					((NTSTATUS) 0xC0000001L)
#define STATUS_INFO_LENGTH_MISMATCH			((NTSTATUS) 0xC0000004L) 
#define STATUS_IO_DEVICE_ERROR              ((NTSTATUS) 0xC0000185L) 

typedef struct _CLIENT_ID { 
    HANDLE UniqueProcess; 
    HANDLE UniqueThread; 
} CLIENT_ID; 
typedef CLIENT_ID *PCLIENT_ID; 

typedef PVOID PUSER_THREAD_START_ROUTINE; 

typedef NTSTATUS (__stdcall *PCreateThread)(
	IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits OPTIONAL,
    IN SIZE_T MaximumStackSize OPTIONAL,
    IN SIZE_T CommittedStackSize OPTIONAL,
    IN PUSER_THREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE Thread OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL
	); 

HANDLE RtlCreateRemoteThread(
	IN  HANDLE hProcess,
	IN  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	IN  DWORD dwStackSize,
	IN  LPTHREAD_START_ROUTINE lpStartAddress,
	IN  LPVOID lpParameter, 
	IN  DWORD dwCreationFlags, 
	OUT LPDWORD lpThreadId 
	);



DWORD ProcesstoPid(char *Processname);
BOOL WINAPI InjectDllToProcess(DWORD dwTargetPid ,LPCTSTR DllPath );
VOID PrepareShellCode(BYTE *pShellCode);
PCreateThread RtlCreateUserThread; 

#ifdef _WIN64
EXTERN_C VOID ShellCodeFun64(VOID);
char g_szDllPath[MAX_PATH] = "F:\\Program2016\\DllInjection\\MsgDll64.dll" ;
char g_szProcName[MAX_PATH] = "HostProc64.exe";
#else
VOID ShellCodeFun(VOID);
// char g_szDllPath[MAX_PATH] = "F:\\Program2016\\DllInjection\\MsgDll.dll" ;
//char g_szDllPath[MAX_PATH] = "C:\\tool\\PEDIY_BOOK4\\chap12\\MsgDll.dll" ;
char g_szDllPath[MAX_PATH] = "MsgDll.dll" ;
char g_szProcName[MAX_PATH] = "HostProc.exe";
#endif

int main(int argc, char* argv[])
{
	BOOL bResult = FALSE ;
	DWORD dwPid = ProcesstoPid(g_szProcName);
	printf("[*] Target Process Pid = %d\n",dwPid);
	if (dwPid == 0)
	{
		printf("Process %s not found!\n",g_szProcName);
		return 0 ;
	}
	InjectDllToProcess(dwPid,g_szDllPath);
	getchar();
	return 0;
}

//ע��������
BOOL WINAPI InjectDllToProcess(DWORD dwTargetPid ,LPCTSTR DllPath )
{
    HANDLE hProc = NULL;
	hProc=OpenProcess(PROCESS_ALL_ACCESS,//Win7��Ҫ���Ȩ�޽ϸ�
		FALSE,
		dwTargetPid
		);
	
    if(hProc == NULL)
    {
        printf("[-] OpenProcess Failed.\n");
        return FALSE;
    }
	
    LPTSTR psLibFileRemote = NULL;
	
    //ʹ��VirtualAllocEx������Զ�̽��̵��ڴ��ַ�ռ����DLL�ļ�������
    psLibFileRemote=(LPTSTR)VirtualAllocEx(hProc, NULL, lstrlen(DllPath)+1,
		MEM_COMMIT, PAGE_READWRITE);
	
    if(psLibFileRemote == NULL)
    {
        printf("[-] VirtualAllocEx Failed.\n");
        return FALSE;
    }
	
    //ʹ��WriteProcessMemory������DLL��·�������Ƶ�Զ�̵��ڴ�ռ�
    if(WriteProcessMemory(hProc, psLibFileRemote, (void *)DllPath, lstrlen(DllPath)+1, NULL) == 0)
    {
        printf("[-] WriteProcessMemory Failed.\n");
        return FALSE;
    }
	
    //����LoadLibraryA����ڵ�ַ
    PTHREAD_START_ROUTINE pfnStartAddr=(PTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandle("Kernel32"),"LoadLibraryA");
	
    if(pfnStartAddr == NULL)
    {
        printf("[-] GetProcAddress Failed.\n");
        return FALSE;
    }
	
    //pfnStartAddr��ַ����LoadLibraryA����ڵ�ַ
	
	//DebugBreak();
    HANDLE hThread = RtlCreateRemoteThread(hProc,
        NULL,
        0,
        pfnStartAddr,
        psLibFileRemote,
        0,
        NULL);
	
    if(hThread == NULL)
    {
        printf("[-] CreateRemoteThread Failed. ErrCode = %d\n",GetLastError());
        return FALSE;
    }
	
	printf("[*] Inject Succesfull.\n");
    return TRUE;
}

typedef struct _INJECT_DATA 
{
	BYTE ShellCode[0x30];
	LPVOID lpThreadStartRoutine;
	LPVOID lpParameter;
	LPVOID AddrOfZwTerminateThread;
}INJECT_DATA;

#ifndef _WIN64
__declspec (naked)
VOID ShellCodeFun(VOID)
{
	__asm
	{
		call L001
L001:
		pop ebx
		sub ebx,5
		push dword ptr ds:[ebx]INJECT_DATA.lpParameter //lpParameter
		call dword ptr ds:[ebx]INJECT_DATA.lpThreadStartRoutine //ThreadProc
		xor eax,eax
		push eax
		push -2 //CurrentThread
		call dword ptr ds:[ebx]INJECT_DATA.AddrOfZwTerminateThread //ZwTerminateThread
		nop //no return
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
	//x64����ֱ��ʹ��__asm��,ֻ������,���Ҹú������㺯����û����call�������Կ�ʼʱesp % 16 =0����call֮ǰ���뱣֤
	//x64������ָ�볤����8,����ע�����������ƫ�Ʊ仯
	/*
	BYTE ShellCode64[]=
		"\x50"					//push    rax						;eq to sub rsp,8
		"\x53"					//push    rbx
		"\x9c"					//pushfq
		"\xe8\x00\x00\x00\x00"  //call  next
		"\x5b"					//pop     rbx
		"\x66\x83\xe3\x00"      //and     bx,0						;rbx = Allocated Mem base
		"\x48\x8b\x4b\x38"      //mov     rcx,qword ptr [rbx+38h]	;lpParameter
		"\xff\x53\x30"          //call    qword ptr [rbx+30h]		;lpThreadStartRoutine
		"\x48\x33\xC0"			//xor		rax,rax					;ExitStatus
		"\x48\x8d\x48\xfe"		//lea		rcx,[rax-2]				;-2 = CurrentThread
		"\x48\x8b\xd0"			//mov	rdx,rax
		"\xff\x53\x40"          //call    qword ptr [rbx+40h]		;ZwTerminateThread
		"\x9d"					//popfq								;no return here
		"\x5b"					//pop     rbx
		"\xc3"					//ret
		"\x90"					//nop
		;

		memcpy(pOutShellCode,ShellCode64,38);
		*/
	
	// for x 64
	BYTE *pShellcodeStart = (BYTE*)ShellCodeFun64;
#else
	// for x86
	BYTE *pShellcodeStart = (BYTE*)ShellCodeFun;
#endif

	BYTE *pShellcodeEnd = 0 ;
	SIZE_T ShellCodeSize = 0 ;
	if (pShellcodeStart[0] == 0xE9)
	{
		//Debugģʽ�£�������ͷ��һ����תָ�����ȡ����������ַ
		pShellcodeStart = pShellcodeStart + *(ULONG*)(pShellcodeStart +1 ) + 5;
	}

	//����Shellcode������־
	pShellcodeEnd = pShellcodeStart;
	while (memcmp(pShellcodeEnd,"\x90\x90\x90\x90\x90",5) != 0)
	{
		pShellcodeEnd++;
	}

	ShellCodeSize = pShellcodeEnd - pShellcodeStart;
	printf("[*] Shellcode Len = %d\n",ShellCodeSize);
	memcpy(pOutShellCode,pShellcodeStart,ShellCodeSize);

}

HANDLE RtlCreateRemoteThread(
	IN  HANDLE hProcess,
	IN  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	IN  DWORD dwStackSize,
	IN  LPTHREAD_START_ROUTINE lpStartAddress,
	IN  LPVOID lpParameter, 
	IN  DWORD dwCreationFlags, 
	OUT LPDWORD lpThreadId 
	)
{
    NTSTATUS status = STATUS_SUCCESS; 
    CLIENT_ID Cid; 
	HANDLE hThread = NULL ;
	SIZE_T dwIoCnt = 0 ;
	
    if (hProcess == NULL
		|| lpStartAddress == NULL)
    {
        return NULL;
    }
	
    //��ȡNative API������ַ 
    RtlCreateUserThread =(PCreateThread)GetProcAddress(GetModuleHandle("ntdll"),"RtlCreateUserThread"); 
	
    if(RtlCreateUserThread == NULL)
    { 
        return NULL;
    }

	//��Ŀ������������ڴ�д��ShellCode
	PBYTE pMem = (PBYTE)VirtualAllocEx(hProcess,NULL,0x1000,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	if (pMem == NULL)
	{
		return NULL;
	}
	
	printf("[*] pMem = 0x%p\n",pMem);
	
	//���첢���INJECT_DATA
	INJECT_DATA Data;
	ZeroMemory(&Data,sizeof(INJECT_DATA));
	PrepareShellCode(Data.ShellCode);
	Data.lpParameter = lpParameter;
	Data.lpThreadStartRoutine = lpStartAddress;
	Data.AddrOfZwTerminateThread = GetProcAddress(GetModuleHandle("ntdll"),"ZwTerminateThread"); 
	

	//д��ShellCode
	if (!WriteProcessMemory(hProcess,pMem,&Data,sizeof(INJECT_DATA),&dwIoCnt))
	{
		printf("[-] WriteProcessMemory Failed!\n");
		VirtualFreeEx(hProcess,pMem,0,MEM_RELEASE);
		return NULL;
	}
	
	printf("[*] ShellCode Write OK.\n");

    status = RtlCreateUserThread(
		hProcess,
		lpThreadAttributes,	//ThreadSecurityDescriptor
		TRUE,	//CreateSuspend
		0,		//ZeroBits
		dwStackSize,		//MaximumStackSize 
		0,		//CommittedStackSize 
		(PUSER_THREAD_START_ROUTINE)pMem,//pMem��ͷ����Shellcode
		NULL,
		&hThread,
		&Cid);
	
	if (status >= 0)
	{
		printf("[*] �̴߳����ɹ�!\n");
		if (lpThreadId != NULL)
		{
			*lpThreadId = (DWORD)Cid.UniqueThread;
		}

		if (!(dwCreationFlags & CREATE_SUSPENDED))
		{
			ResumeThread(hThread);
		}
	}

	
    return hThread;   
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