#pragma once

class TxtLoder {
public:
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	TxtLoder();
	~TxtLoder();
	bool LoadModel(const wchar_t* fileName);
	Vertex* GetModel();
	int GetModelSize();
	int GetModelFaceCount();


private:
	
	
	Vertex* Models;
	int vertexCount;
};