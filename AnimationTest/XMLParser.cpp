#include "stdafx.h"
#include "XMLParser.h"
#include "WinAPI.h"

XML::XMLParser::XMLParser()
{
}

XML::XMLParser::~XMLParser()
{
	xml.Close(); //메모리에 안닫혀 있으면 곤란함으로 한번더 점검
}

bool XML::XMLParser::XMLOpen(wchar_t* lpszFilename)
{
	if (!xml.Open(lpszFilename))
	{
		WinAPI::pWinAPI->Log(L"can't open [%s] file.", lpszFilename);
		return false;
	}



}
