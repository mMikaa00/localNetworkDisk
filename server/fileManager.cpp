#include "fileManager.h"

void initFileFolder(const char *path, unordered_map<string, file> &fileFolder) {
	WIN32_FIND_DATA find = { 0 };
	HANDLE hFind = FindFirstFile(path, &find);
	BOOL nResult = TRUE;
	while (nResult == TRUE)
	{
		if (find.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)//����ļ���һ��Ŀ¼  
		{
			/*_tprintf(TEXT("Ŀ¼��%s\n"), find.cFileName);
			if (find.cFileName[0] != '.')
			{
			//
			TCHAR szNextPath[MAX_PATH] = { 0 };
			_stprintf(szNextPath, TEXT("%s%s\\"), szPath, find.cFileName);
			Find(szNextPath, szType);
			}*/

		}
		else							//�ļ�����һ��Ŀ¼  
		{
			fileFolder.emplace(find.cFileName, file(find.cFileName, path,find.nFileSizeLow));
		}
		nResult = FindNextFile(hFind, &find);
	}
	FindClose(hFind);
}