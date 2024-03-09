#include "stdafx.h"
#include "TxtLoder.h"

TxtLoder::TxtLoder() :Models(nullptr)
{
}

TxtLoder::~TxtLoder()
{
	if (Models != nullptr)
	{
		delete Models;
		Models = nullptr;
	}
}

bool TxtLoder::LoadModel(const wchar_t* fileName)
{
	std::ifstream fin;
	fin.open(fileName);
	
	wchar_t currentDir[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, currentDir);


	if (!fin.is_open()&&fin.fail())
	{
		return false;
	}
	
	//get으로 읽을까 geline으로 읽을까 고민했는데 get으로 하나씩 읽자
	wchar_t input = 0;
	fin.seekg(sizeof("Vertex Count:"), std::ios::cur);
	fin >> vertexCount;//버텍스 카운터
	Models = new Vertex[vertexCount]; //개수만큼 할당
	
	char word[10] = {};
	while (!fin.eof()) //끝까지
	{
		if (strcmp(word ,"Data:\0")==0) //특정단어를 찾았다면 
			break;
		fin >> word; //단어찾기위해서 읽기
	}

	for (int i = 0; i < vertexCount; ++i)
	{
		fin >> Models[i].position.x >> Models[i].position.y >> Models[i].position.z;
		fin >> Models[i].color.x >> Models[i].color.y >> Models[i].color.z;
		Models[i].color.w = 1.0f;
	}


	fin.close();

	return true;
}

TxtLoder::Vertex* TxtLoder::GetModel()
{
	return Models;
}

int TxtLoder::GetModelSize()
{
	return sizeof(Vertex)* vertexCount;
}

int TxtLoder::GetModelFaceCount()
{
	return vertexCount;
}



