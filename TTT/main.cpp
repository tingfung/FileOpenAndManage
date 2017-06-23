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
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用  
	char *psText; // psText为char*的临时数组，作为赋值给std::string的中间变量  
	psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用  
	szDst = psText;// std::string赋值  
	delete[]psText;// psText的清除  
}

//#define MAX_PATH 1024  //最长路径长度  

/*----------------------------
* 功能 : 递归遍历文件夹，找到其中包含的所有文件
*----------------------------
* 函数 : find
* 访问 : public
*
* 参数 : lpPath [in]      需遍历的文件夹目录
* 参数 : fileList [in]      以文件名称的形式存储遍历后的文件
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
	选择需要处理的时间段
	@timeStart：开始时间
	@timeEnd：结束时间
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
				start = contentIter; // 记录开始位置
				break;
			}
		}
		for (; contentIter != fileListIter->content.end(); ++contentIter)
		{
			if (contentIter->time >= timeEnd) 
			{
				end = contentIter; // 记录结束位置
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
	寻找异常特征：计算差分
*/
void findAbnormalData(vector<fileData> &fileList, vector<vector<int>> &res)
{
	for (vector<fileData>::iterator fileIter = fileList.begin(); fileIter != fileList.end(); ++fileIter)
	{
		if (fileIter->content.empty()) break;
		//vector<double> thisContentValue(fileIter->content[value])
		//vector<mData>::iterator maxContent = max_element(fileIter->content.begin(), fileIter->content.end());	// 最大值
		//vector<mData>::iterator minContent = min_element(fileIter->content.begin(), fileIter->content.end());	// 最小值
		double maxNum = -99999999, minNum = 999999999;
		for (vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter != fileIter->content.end(); ++contentIter)
		{
			if (contentIter->value > maxNum) maxNum = contentIter->value; // 最大值
			if (contentIter->value < minNum) minNum = contentIter->value;	// 最小值
		}
		double range = maxNum - minNum;

		vector<int> contentRes(2, 0); //每组应有两个元素，是否有突起，是否有凹陷

		double diff = fileIter->content[1].time - fileIter->content[0].time;
		const int window = 0.001 / diff;  // 设置窗口大小
		for (int contentIter = 0; contentIter < fileIter->content.size() - window; contentIter += window)//vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter < fileIter->content.end(); contentIter += window)
		{	// 检查所有窗口中是否有突起和凹陷
			if(contentIter > fileIter->content.size()) break;
			vector<mData>::iterator contentBegin = fileIter->content.begin();
			vector<mData> thisDataContent(contentBegin + contentIter, contentBegin + contentIter + window);
			double maxThisNum = -99999999, minThisNum = 999999999;
			//vector<mData>::iterator maxthisContent = max_element(contentIter, contentIter + window);	//窗口内最大值
			//vector<mData>::iterator minthisContent = min_element(contentIter, contentIter + window);	//窗口内最小值
			int maxthisContent, minthisContent;
			for (int i = 0; i < thisDataContent.size(); ++i)
			{
				if (thisDataContent[i].value > maxThisNum)
					if (thisDataContent[i].value > maxThisNum)
				{
					maxThisNum = fileIter->content[contentIter].value; // 最大值
					maxthisContent = i;
				}
				if (thisDataContent[i].value  < minThisNum)
				{
					minThisNum = fileIter->content[contentIter].value;	// 最小值
					minthisContent = i;
				}
			}
			double thisRange = maxThisNum - minThisNum;
			if (thisRange > range * 0.5)
			{
				if (thisDataContent[maxthisContent].time - thisDataContent[minthisContent].time > 0) contentRes[0] = 1; // 突起
				else contentRes[1] = 1; // 凹陷
			}
		}
		res.push_back(contentRes);
		//for (vector<mData>::iterator contentIter = fileIter->content.begin(); contentIter != fileIter->content.end(); ++contentIter)
		//{
		//	double maxContent = -999999.9, minContent = 999999.9, maxRange = 0;
		//	if (contentIter->value > maxContent) maxContent = contentIter->value;	// 最大值
		//	if (contentIter->value < minContent) minContent = contentIter->value;		// 最小值
		//	double bigContent = -999999.9, smallContent = 999999.9, lastNum = -999999.9, bigRange = 0;
		//	

		//	
		//}
	}
}


int main()
{
	vector<fileData> fileList;// 定义一个存放结果文件名称的列表

	// 遍历一次结果的所有文件,获取文件名列表  
	find("C:\\Users\\win7\\Desktop\\appfile1", fileList); // 之后可对文件列表中的文件进行相应的操作  

	// 选择需要的时间段
	const double timeStart = 1.0;
	const double timeEnd = 3.0;
	choosePeriod(fileList, timeStart, timeEnd);

	// 寻找异常特征
	vector<vector<int>> res;
	findAbnormalData(fileList, res);




	// 输出文件夹下所有文件的名称  
	for (int i = 0; i < fileList.size(); i++)
	{
		cout << fileList[i].fileName << endl;
	}
	cout << "文件数目：" << fileList.size() << endl;
	return 0;
}