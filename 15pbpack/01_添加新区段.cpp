#include <iostream>
using namespace std;


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

int main(int argc,char** argv )
{
	// 1. ��ȡ���ӿǳ����·��
	char path[MAX_PATH] = { 0 };
	printf(">");
	gets_s(path, MAX_PATH);

	// 2. ���������
	int nFileSize = 0;
	char* pFileData = getFileData(path, &nFileSize);
	addSection(pFileData, nFileSize, "15PPACK", 200, "--");

	// 3. ��浽һ���ļ���.
	savePeFile(pFileData, nFileSize, "pack.exe");

	return 0;
}
