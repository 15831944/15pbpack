#include <iostream>
using namespace std;
#include "../stub/Conf.h"

#include <windows.h>

// ��һ�������е�pe�ļ�
HANDLE openPeFile(_In_ const char* path) {
	return CreateFileA(path,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

// �ر��ļ�
void closePeFile(_In_ HANDLE hFile) {
	CloseHandle(hFile);
}

// ���ļ����浽ָ��·����
bool savePeFile(_In_  const char* pFileData,
	_In_  int nSize,
	_In_ const char* path) {
	HANDLE hFile = CreateFileA(path,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwWrite = 0;
	// ������д�뵽�ļ�
	WriteFile(hFile, pFileData, nSize, &dwWrite, NULL);
	// �ر��ļ����
	CloseHandle(hFile);
	return dwWrite == nSize;
}

// ��ȡ�ļ����ݺʹ�С
char* getFileData(_In_ const char* pFilePath,
	_Out_opt_ int* nFileSize = NULL) {
	// ���ļ�
	HANDLE hFile = openPeFile(pFilePath);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	// ��ȡ�ļ���С
	DWORD dwSize = GetFileSize(hFile, NULL);
	if (nFileSize)
		*nFileSize = dwSize;
	// ����Կռ�
	char* pFileBuff = new char[dwSize];
	memset(pFileBuff, 0, dwSize);
	// ��ȡ�ļ����ݵ��ѿռ�
	DWORD dwRead = 0;
	ReadFile(hFile, pFileBuff, dwSize, &dwRead, NULL);
	CloseHandle(hFile);
	// ���ѿռ䷵��
	return pFileBuff;
}

// �ͷ��ļ�����
void freeFileData(_In_  char* pFileData) {
	delete[] pFileData;
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

// ��ȡָ�����ֵ�����ͷ
IMAGE_SECTION_HEADER* getSection(_In_ char* pFileData,
	_In_  const char* scnName)//��ȡָ�����ֵ�����
{
	// ��ȡ���θ�ʽ
	DWORD dwScnCount = getFileHeader(pFileData)->NumberOfSections;
	// ��ȡ��һ������
	IMAGE_SECTION_HEADER* pScn = IMAGE_FIRST_SECTION(getNtHeader(pFileData));
	char buff[10] = { 0 };
	// ��������
	for (DWORD i = 0; i < dwScnCount; ++i) {
		memcpy_s(buff, 8, (char*)pScn[i].Name, 8);
		// �ж��Ƿ�����ͬ������
		if (strcmp(buff, scnName) == 0)
			return pScn + i;
	}
	return nullptr;
}


// ��ȡ���һ������ͷ
IMAGE_SECTION_HEADER* getLastSection(_In_ char* pFileData)// ��ȡ���һ������
{
	// ��ȡ���θ���
	DWORD dwScnCount = getFileHeader(pFileData)->NumberOfSections;
	// ��ȡ��һ������
	IMAGE_SECTION_HEADER* pScn = IMAGE_FIRST_SECTION(getNtHeader(pFileData));
	// �õ����һ����Ч������
	return pScn + (dwScnCount - 1);
}

// ��������С
int aligment(_In_ int size, _In_  int aliginment) {
	return (size) % (aliginment) == 0 ? (size) : ((size) / (aliginment)+1)* (aliginment);
}

void addSection(char*& pFileData,/*��������ε�PE�ļ�������*/
	int&   nFileSize/*PE�ļ����ݵ��ֽ���*/,
	const char* pNewSecName,/*�����ε�����*/
	int   nSecSize,/*�����ε��ֽ���*/
	const void* pSecData/*�����ε�����*/)
{
	// 1. �޸��ļ�ͷ�����θ���
	getFileHeader(pFileData)->NumberOfSections++;
	// 2. �޸�������ͷ
	IMAGE_SECTION_HEADER* pScn = getLastSection(pFileData);
	// 2.1 ������
	memcpy(pScn->Name, pNewSecName, 8);
	// 2.2 ���εĴ�С
	// 2.2.1 ʵ�ʴ�С
	pScn->Misc.VirtualSize = nSecSize;
	// 2.2.2 �����Ĵ�С
	pScn->SizeOfRawData = aligment(nSecSize,
		getOptionHeader(pFileData)->FileAlignment);
	// 2.3 ���ε�λ��
	// 2.3.1 �ļ���ƫ�� = �������ļ���С
	pScn->PointerToRawData = aligment(nFileSize,
		getOptionHeader(pFileData)->FileAlignment);

	// 2.3.2 �ڴ��ƫ�� = ��һ�����ε��ڴ�ƫ�ƵĽ���λ��
	IMAGE_SECTION_HEADER* pPreSection = NULL;
	pPreSection = pScn - 1;
	pScn->VirtualAddress = pPreSection->VirtualAddress 
		+ aligment(pPreSection->SizeOfRawData,
			getOptionHeader(pFileData)->SectionAlignment);
	// 2.4 ���ε�����
	// 2.4.1 �ɶ���д��ִ��
	pScn->Characteristics = 0xE00000E0;
	// 3. ������չͷ��ӳ���С.
	getOptionHeader(pFileData)->SizeOfImage =
		pScn->VirtualAddress + pScn->SizeOfRawData;

	// 4. ���·��������ڴ�ռ��������µ���������
	int nNewSize = pScn->PointerToRawData + pScn->SizeOfRawData;
	char* pBuff = new char[nNewSize];
	memcpy(pBuff, pFileData, nFileSize);
	memcpy(pBuff + pScn->PointerToRawData,
		pSecData,
		pScn->Misc.VirtualSize);
	freeFileData(pFileData);
	
	// �޸��ļ���С
	pFileData = pBuff;
	nFileSize = nNewSize;
}

struct StubDll
{
	char* pFileData; // DLL�ļ��ػ�ַ

	char* pTextData; // ����ε�����
	DWORD dwTextDataSize; // ����εĴ�С

	StubConf* pConf;// DLL�е�����ȫ�ֱ���
	void* start;    // DLL�е�������
};

void loadStub(StubDll* pStub)
{
	// ��stub.dll���ص��ڴ�
	// ���ص��ڴ�ֻ��Ϊ�˸�����ػ�ȡ,�Լ��޸�
	// dll������,����������Ҫ����dll�Ĵ���.
	pStub->pFileData = 
		(char*)LoadLibraryEx(L"stub.dll",NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (pStub->pFileData==NULL) {
		MessageBox(NULL, L"DLL����ʧ��", 0, 0);
		ExitProcess(0);
	}
	IMAGE_SECTION_HEADER* pTextScn;
	pTextScn = getSection(pStub->pFileData, ".text");
	pStub->pTextData = 
		pTextScn->VirtualAddress + pStub->pFileData;
	pStub->dwTextDataSize = pTextScn->Misc.VirtualSize;

	// ��ȡ������������
	pStub->pConf = (StubConf*)
		GetProcAddress((HMODULE)pStub->pFileData,
			"g_conf");
	pStub->start = GetProcAddress((HMODULE)pStub->pFileData,
		"start");
}

void fixStubRelocation(StubDll* stub/*stub.dll���ڴ��е���Ϣ*/,
	char* pFileData, /*���ӿǳ�����ļ����ݻ�����*/
	DWORD dwNewScnRva)
{
	// 1. ���ҵ�stub.dll�����е��ض�λ��.
	// 1.1 �����ض�λ��.
	// 1.2 �޸��ض�λ(��DLL�����е��ض�λ���ݸĵ�)
	//     �ض�λ�� = �ض�λ�� - ��ǰ���ػ�ַ - ��ǰ����rva + �µļ��ػ�ַ(���ӿǳ���ļ��ػ�ַ) + �����εĶ���RVA.
	IMAGE_BASE_RELOCATION* pRel =
		(IMAGE_BASE_RELOCATION*)
		(getOptionHeader(stub->pFileData)->DataDirectory[5].VirtualAddress + (DWORD)stub->pFileData);

	DWORD pStubTextRva = getSection(stub->pFileData, ".text")->VirtualAddress;
	while (pRel->SizeOfBlock != 0)
	{
		struct TypeOffset
		{
			WORD ofs : 12;
			WORD type : 4;
		}*typOfs = NULL;

		typOfs = (TypeOffset*)(pRel + 1);
		DWORD count = (pRel->SizeOfBlock - 8) / 2;
		for (size_t i = 0; i < count; i++)
		{
			if (typOfs[i].type == 3) {
				DWORD fixAddr = 
					typOfs[i].ofs 
					+ pRel->VirtualAddress 
					+ (DWORD)stub->pFileData;

				DWORD oldProt = 0;
				VirtualProtect((LPVOID)fixAddr, 1, PAGE_EXECUTE_READWRITE, &oldProt);
				*(DWORD*)fixAddr -= (DWORD)stub->pFileData;
				*(DWORD*)fixAddr -= pStubTextRva;
				*(DWORD*)fixAddr += getOptionHeader(pFileData)->ImageBase;
				*(DWORD*)fixAddr += dwNewScnRva;
				VirtualProtect((LPVOID)fixAddr, 1, oldProt, &oldProt);
			}
		}

		pRel = (IMAGE_BASE_RELOCATION*)
			((LPBYTE)pRel + pRel->SizeOfBlock);
	}
}

int main(int argc,char** argv )
{
	// 1. ��ȡ���ӿǳ����·��
	char path[MAX_PATH] = { 0 };
	printf(">");
	gets_s(path, MAX_PATH);


	// ����stub.dll
	StubDll stub;
	loadStub(&stub);

	// ��ȡ�ļ�����
	int nFileSize = 0;
	char* pFileData = getFileData(path, &nFileSize);



	// �����ӿǳ������Ϣ���浽stub�ĵ����ṹ�������.
	stub.pConf->oep = getOptionHeader(pFileData)->AddressOfEntryPoint;


	// ����dll���ض�λ����
	IMAGE_SECTION_HEADER* pLastScn = getLastSection(pFileData);
	DWORD dwNewScnRva = pLastScn->VirtualAddress + aligment(pLastScn->SizeOfRawData,getOptionHeader(pFileData)->SectionAlignment);
	fixStubRelocation(&stub, pFileData, dwNewScnRva);

	// 2. ���������
	//    ����stub.dll����������(�������ݵ��ض�λ�Ѿ�������)
	//    ��������������.
	addSection(pFileData,
		nFileSize,
		"15PBPACK",
		stub.dwTextDataSize,
		stub.pTextData/*stub�Ĵ��������*/);

	// ��OEP���õ���������(stub.dll�Ĵ������).
	// stub.dll�е�һ��VAת���ɱ��ӿǳ����е�VA
	// VA - stub.dll���ػ�ַ ==> RVA
	// RVA - stub.dll�Ĵ���ε�RVA ==> ����ƫ��
	// ����ƫ�� + �����ε�RVA ==> ���ӿǳ����е�RVA
	DWORD stubStartRva = (DWORD)stub.start;
	stubStartRva -= (DWORD)stub.pFileData;
	stubStartRva -= getSection(stub.pFileData,".text" )->VirtualAddress;
	stubStartRva += getSection(pFileData, "15PBPACK")->VirtualAddress;
	getOptionHeader(pFileData)->AddressOfEntryPoint = stubStartRva;


	// 3. ��浽һ���ļ���.
	savePeFile(pFileData, nFileSize, "pack.exe");
	return 0;
}
