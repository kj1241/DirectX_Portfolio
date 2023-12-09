#include "stdafx.h"
#include "CXML.h"

XML::CXML::CXML():pXMLDoc(nullptr)
{
	//CoInitialize(nullptr);
	//CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_ALL, IID_IXMLDOMDocument, (void**)&pXMLDoc);
}

XML::CXML::~CXML()
{
	CoUninitialize();
}

bool XML::CXML::Open(LPCWSTR url)
{
	short sResult=0; // 결과
	try
	{
		//파일이 유효한지 확인하는용 xml은 찾을수 없으니
		std::ifstream  file(url, std::ios::in); //읽기전용
		if (!file.is_open())
			return false;
		file.close();

		pXMLDoc->put_async(false); //비동기적으로 로드x
		_bstr_t varString = (LPCTSTR)url; //VARIANT 형태로 변환 ... c++17에 해더로있지않나 일단 패스
		VARIANT path;
		path.vt = VT_BSTR;
		path.bstrVal = varString;

		int hr = pXMLDoc->load(path, &sResult);
		if (hr < 0)
			return false;
	}
	catch (...)
	{
		pXMLDoc->Release(); //
		pXMLDoc = nullptr;
		return false;
	}
	return true;
}

void XML::CXML::Close()
{
	pXMLDoc->Release();
}

IXMLDOMNodeList* XML::CXML::FindElement(LPCTSTR strElement)
{
	IXMLDOMNodeList* pNodeList = nullptr; //노드 리스
	if (pXMLDoc == nullptr)
		return nullptr;

	try
	{
		_bstr_t bstrPath = strElement; //선언안해주면 wchar_t에서 VARIANT 변환못해서 못찾음
		pXMLDoc->selectNodes(bstrPath, &pNodeList);
	}
	catch (...)
	{

	}
	return pNodeList;
}

IXMLDOMNodeList* XML::CXML::FindElement(IXMLDOMNode* pNode, LPCTSTR strElement)
{
	IXMLDOMNodeList* pNodeList = nullptr; //노드 리스
	if (pXMLDoc == nullptr)
		return nullptr;

	try
	{
		_bstr_t bstrPath = strElement;
		pNode->selectNodes(bstrPath, &pNodeList);
	}
	catch (...)
	{
	}

	return pNodeList;
}

bool XML::CXML::GetElementText(IXMLDOMNode* pNode, LPSTR strRet)
{
	BSTR bstr = nullptr;
	pNode->get_text(&bstr); 
	//const char_t* strResult = _bstr_t(bstr, false);
	const wchar_t* strResult = _bstr_t(bstr, false); //BSTR을 const wchar*로 변환하는 부분

	
	size_t convertedChars = 0; 
	//strncpy(strRet, strResult, 128);
	wcstombs_s(&convertedChars, strRet, 127, strResult, _TRUNCATE); //Unicode 변환을 다루기 위해 wcstombs 사용
	::SysFreeString(bstr); // BSTR 해제

	return true;
}

int XML::CXML::GetAttributeText(IXMLDOMNode* pNode, LPSTR strAttrName, LPSTR strRet)
{
	wchar_t wstrAttr[128];
	IXMLDOMNode* pAttrNode = nullptr;
	IXMLDOMNamedNodeMap* pMap = nullptr;
	VARIANT	varValue; //c++17쓰는것도 더 좋을지도?

	try
	{
		int n = mbstowcs(wstrAttr, strAttrName, 128); //멀티바이트 문자열을 와일드바이트 문자열로 변환
		pNode->get_attributes(&pMap); //노드와 속성 가져옴
		pMap->getNamedItem(wstrAttr, &pAttrNode); //속성 노드 가져옴
		pAttrNode->get_nodeValue(&varValue); // 메서드 속성 값 가저옴

			
		//const char* strResult = _bstr_t(varValue.bstrVal, false);
		//strncpy(strRet, strResult, 127); // 127는 버퍼 오버플로우 방지하려면 이따구로 써야됨

		wcstombs(strRet, varValue.bstrVal, 128); //멀티바이트를 문자열로 변환하고 복사

		//메크로 만들어야되나?
		if (pAttrNode != nullptr)
		{
			pAttrNode->Release();
			pAttrNode = nullptr;
		}

		if (pMap != nullptr)
		{
			pMap->Release();
			pMap = nullptr;
		}

	}
	catch (...)
	{
		if (pAttrNode != nullptr)
		{
			pAttrNode->Release();
			pAttrNode = nullptr;
		}

		if (pMap != nullptr)
		{
			pMap->Release();
			pMap = nullptr;
		}
		return false;
	}

	return true;
}


