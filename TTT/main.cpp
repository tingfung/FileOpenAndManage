#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#include <stdio.h>  
#include <iostream>  
#include <vector>  
#include <Windows.h>  
#include <fstream>    
#include <iterator>  
#include <string>  
#include <algorithm> 
using namespace std;

struct mData
{
	double time;
	double value;
};

struct fileData
{
	string fileName;
	vector<mData> content;
};

void Wchar_tToString(string& szDst, wchar_t *wchar)
{
	wchar_t * wText = wchar;
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte������  
	char *psText; // psTextΪchar*����ʱ���飬��Ϊ��ֵ��std::string���м����  
	psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte���ٴ�����  
	szDst = psText;// std::string��ֵ  
	delete[]psText;// psText�����  
}

//#define MAX_PATH 1024  //�·������  

/*----------------------------
* ���� : �ݹ�����ļ��У��ҵ����а����������ļ�
*----------------------------
* ���� : find
* ���� : public
*
* ���� : lpPath [in]      ��������ļ���Ŀ¼
* ���� : fileList [in]      ���ļ����Ƶ���ʽ�洢��������ļ�
*/
void find(char* lpPath, vector<fileData> &fileList)
{
	char szFind[MAX_PATH];
	WIN32_FIND_DATA FindFileData;

	strcpy(szFind, lpPath);
	strcat(szFind, "\\*.*");

	WCHAR wszClassName[1024];
	memset(wszClassName, 0, sizeof(wszClassName));
	MultiByteToWideChar(CP_ACP, 0, szFind, strlen(szFind) + 1, wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));

	HANDLE hFind = ::FindFirstFile(wszClassName, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)    return;

	while (true)
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindFileData.cFileName[0] != '.')
			{
				char szFile[MAX_PATH];
				sprintf(szFile, "%s\\%ws", lpPath, FindFileData.cFileName);
				find(szFile, fileList);
			}
		}
		else
		{
			vector<mData> thisFile;
			mData oneLine;
			string str;

			char buffer[MAX_PATH];
			sprintf(buffer, "%s\\%ws", lpPath, FindFileData.cFileName);

			ifstream infile;
			infile.open(buffer);
			if (infile.good())
			{
				while (getline(infile, str))
				{
					infile >> oneLine.time >> oneLine.value;
					thisFile.push_back(oneLine);
				}
			}
			infile.close();
			Wchar_tToString(str, FindFileData.cFileName);
			fileData fData;
			fData.fileName = str;
			fData.content = thisFile;
			fileList.push_back(fData);
		}
		if (!FindNextFile(hFind, &FindFileData))
			break;
	}
	FindClose(hFind);
}

/*
	ѡ����Ҫ�����ʱ���
	@timeStart����ʼʱ��
	@timeEnd������ʱ��
*/
void choosePeriod(vector<fileData> &fileList, double timeStart, double timeEnd)
{
	for (vector<fileData>::iterator fileListIter = fileList.begin(); fileListIter != fileList.end(); ++fileListIter)
	{
		vector<mData>::iterator contentIter = fileListIter->content.begin();
		vector<mData>::iterator start = fileListIter->content.begin(), end = fileListIter->content.end();
		for (; contentIter != fileListIter->content.end(); ++contentIter)
		{
			if (contentIter->time >= timeStart) 
			{
				start = contentIter; // ��¼��ʼλ��
				break;
			}
		}
		for (; contentIter != fileListIter->content.end(); ++contentIter)
		{
			if (contentIter->time >= timeEnd) 
			{
				end = contentIter; // ��¼����λ��
				break;
			}
		}
		/*if (end < fileListIter->content.end())	fileListIter->content.erase(end, fileListIter->content.end());
		if (start > fileListIter->content.begin())	fileListIter->content.erase(fileListIter->content.begin(), start);*/
		if (start != fileListIter->content.begin() || end != fileListIter->content.end())
		{
			vector<mData> thisContent(start, end);
			swap(fileListIter->content, thisContent);
		}
		//else 
		//{
		//	vector<mData> thisContent;
		//	thisContent.clear();
		//	swap(fileListIter->content, thisContent);
		//}
	}
}


/*
	Ѱ���쳣������������
*/
void findAbnormalData(vector<fileData> &fileList, vector<vector<int>> &res)
{
	for (vector<fileData>::iterator fileIter = fileList.begin(); fileIter != fileList.end(); ++fileIter)
	{
		if (fileIter->content.empty()) break;
		//vector<double> thisContentValue(fileIter->content[value])
		//vector<mData>::iterator maxContent = max_element(fileIter->content.begin(), fileIter->content.end());	// ���ֵ
		//vector<mData>::iterator minContent = min_element(fileIter->content.begin(), fileIter->content.end());	// ��Сֵ
		double maxNum = -99999999, minNum = 999999999;
		for (vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter != fileIter->content.end(); ++contentIter)
		{
			if (contentIter->value > maxNum) maxNum = contentIter->value; // ���ֵ
			if (contentIter->value < minNum) minNum = contentIter->value;	// ��Сֵ
		}
		double range = maxNum - minNum;

		vector<int> contentRes(2, 0); //ÿ��Ӧ������Ԫ�أ��Ƿ���ͻ���Ƿ��а���

		double diff = fileIter->content[1].time - fileIter->content[0].time;
		const int window = 0.001 / diff;  // ���ô��ڴ�С
		for (int contentIter = 0; contentIter < fileIter->content.size() - window; contentIter += window)//vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter < fileIter->content.end(); contentIter += window)
		{	// ������д������Ƿ���ͻ��Ͱ���
			if(contentIter > fileIter->content.size()) break;
			vector<mData>::iterator contentBegin = fileIter->content.begin();
			vector<mData> thisDataContent(contentBegin + contentIter, contentBegin + contentIter + window);
			double maxThisNum = -99999999, minThisNum = 999999999;
			//vector<mData>::iterator maxthisContent = max_element(contentIter, contentIter + window);	//���������ֵ
			//vector<mData>::iterator minthisContent = min_element(contentIter, contentIter + window);	//��������Сֵ
			int maxthisContent, minthisContent;
			for (int i = 0; i < thisDataContent.size(); ++i)
			{
				if (thisDataContent[i].value > maxThisNum)
					if (thisDataContent[i].value > maxThisNum)
				{
					maxThisNum = fileIter->content[contentIter].value; // ���ֵ
					maxthisContent = i;
				}
				if (thisDataContent[i].value  < minThisNum)
				{
					minThisNum = fileIter->content[contentIter].value;	// ��Сֵ
					minthisContent = i;
				}
			}
			double thisRange = maxThisNum - minThisNum;
			if (thisRange > range * 0.5)
			{
				if (thisDataContent[maxthisContent].time - thisDataContent[minthisContent].time > 0) contentRes[0] = 1; // ͻ��
				else contentRes[1] = 1; // ����
			}
		}
		res.push_back(contentRes);
		//for (vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter != fileIter->content.end(); ++contentIter)
		//{
		//	double maxContent = -999999.9, minContent = 999999.9, maxRange = 0;
		//	if (contentIter->value > maxContent) maxContent = contentIter->value;	// ���ֵ
		//	if (contentIter->value < minContent) minContent = contentIter->value;		// ��Сֵ
		//	double bigContent = -999999.9, smallContent = 999999.9, lastNum = -999999.9, bigRange = 0;
		//	

		//	
		//}
	}
}


int main()
{
	vector<fileData> fileList;// ����һ����Ž���ļ����Ƶ��б�

	// ����һ�ν���������ļ�,��ȡ�ļ����б�  
	find("C:\\Users\\win7\\Desktop\\appfile1", fileList); // ֮��ɶ��ļ��б��е��ļ�������Ӧ�Ĳ���  

	// ѡ����Ҫ��ʱ���
	const double timeStart = 1.0;
	const double timeEnd = 3.0;
	choosePeriod(fileList, timeStart, timeEnd);

	// Ѱ���쳣����
	vector<vector<int>> res;
	findAbnormalData(fileList, res);




	// ����ļ����������ļ�������  
	for (int i = 0; i < fileList.size(); i++)
	{
		cout << fileList[i].fileName << endl;
	}
	cout << "�ļ���Ŀ��" << fileList.size() << endl;
	return 0;
}