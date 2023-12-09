#pragma once
#include "CXML.h"

//3d max -> xml -> directx ....파싱과 파서 커플링작업...
//노가다는 날 강하게 만든다.
namespace XML
{
	class XMLParser
	{
	public:
		XMLParser();
		~XMLParser();

		bool XMLOpen(wchar_t* lpszFilename);


	private:
		CXML xml; //xml 불러오기
		int	_ParseInfo( );

	};
};