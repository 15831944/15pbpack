#include <Windows.h>
#pragma comment(linker,"/merge:.data=.text")
#pragma comment(linker,"/merge:.rdata=.text")
//#pragma comment(linker,"/merge:.tls=.text")
#pragma comment(linker, "/section:.text,RWE")
#include "Conf.h"

int WINAPI DllMain(HMODULE, LPVOID, int)
{
	return TRUE;
}



//��ȡDOSͷ
IMAGE_DOS_HEADER* getDosHeader(_In_  char* pFileData) {
	return (IMAGE_DOS_HEADER *)pFileData;
}

// ��ȡNTͷ
IMAGE_NT_HEADERS* getNtHeader(_In_  char* pFileData) {
	return (IMAGE_NT_HEADERS*)(getDosHeader(pFileData)->e_lfanew + (SIZE_T)pFileData);
}

//��ȡ�ļ�ͷ
IMAGE_FILE_HEADER* getFileHeader(_In_  char* pFileData) {
	return &getNtHeader(pFileData)->FileHeader;
}

//��ȡ��չͷ
IMAGE_OPTIONAL_HEADER* getOptionHeader(_In_  char* pFileData) {
	return &getNtHeader(pFileData)->OptionalHeader;
}


typedef LPVOID* (WINAPI* FnGetProcAddress)(HMODULE, const char*);
FnGetProcAddress pfnGetProcAddress;

typedef HMODULE(WINAPI* FnLoadLibraryA)(const char*);
FnLoadLibraryA pfnLoadLibraryA;

typedef DWORD(WINAPI* FnMessageBoxA)(HWND, const char*, const char*, UINT);
FnMessageBoxA pfnMessageBoxA;

DWORD MyStrcmp(const char* dest, const char* src)
{
	int i = 0;
	while (dest[i])
	{
		if (dest[i] != src[i])
		{
			return 1;
		}
		i++;
	}

	return 0;
}

void getApi()
{
	// 1. �Ȼ�ȡkernel32�ļ��ػ�ַ
	HMODULE hKernel32=NULL;
	_asm
	{
		mov eax, FS:[0x30];
		mov eax, [eax + 0xc];
		mov eax, [eax + 0xc];
		mov eax, [eax];
		mov eax, [eax];
		mov eax, [eax + 0x18];
		mov hKernel32, eax;
	}
	// 2. �ٻ�ȡLoadLibrayA��GetProcAddress�����ĵ�ַ
	// 2.1 �����������ȡ������ַ
	IMAGE_EXPORT_DIRECTORY* pExp=NULL;
	pExp = (IMAGE_EXPORT_DIRECTORY*)
		(getOptionHeader((char*)hKernel32)->DataDirectory[0].VirtualAddress + (DWORD)hKernel32);
	
	DWORD* pEAT = NULL, *pENT=NULL;
	WORD* pEOT = NULL;
	pEAT = (DWORD*)(pExp->AddressOfFunctions + (DWORD)hKernel32);
	pENT = (DWORD*)(pExp->AddressOfNames + (DWORD)hKernel32);
	pEOT = (WORD*)(pExp->AddressOfNameOrdinals+(DWORD)hKernel32);
	for (size_t i = 0; i < pExp->NumberOfNames; i++)
	{
		char* pName = pENT[i] + (char*)hKernel32;
		if (MyStrcmp(pName, "GetProcAddress") == 0) {
			int index = pEOT[i];
			pfnGetProcAddress = (FnGetProcAddress)(pEAT[index] + (DWORD)hKernel32);
			break;
		}
	}
	// 3. ͨ��������API��ȡ������API
	pfnLoadLibraryA=
		(FnLoadLibraryA)pfnGetProcAddress(hKernel32, "LoadLibraryA");


	// 4. ������Ϣ�����
	HMODULE hUser32=pfnLoadLibraryA("user32.dll");
	pfnMessageBoxA = 
		(FnMessageBoxA)pfnGetProcAddress(hUser32, "MessageBoxA");

	pfnMessageBoxA(0, "����һ����", "��ʾ", 0);

}

extern"C"
{
	_declspec(dllexport) StubConf g_conf = {0xFFFFFFFF};

	_declspec(dllexport) _declspec(naked) 
		void start()
	{

		// 1. �Ȼ�ȡ��Ҫ��API��ַ.
		getApi();


		g_conf.oep += 0x400000;
		_asm jmp g_conf.oep;
	}
}