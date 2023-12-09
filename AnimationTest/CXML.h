#pragma once
#include <msxml2.h>
#include <comdef.h> // 

//XML 클래스
//결국 xml도 


namespace XML
{
	//솔직히 그냥 텍스트인데 검색도 지원해주니깐 쓴다...
	class CXML
	{
	public:
		CXML();
		~CXML();
		bool Open(LPCWSTR url);
		void Close();
		IXMLDOMNodeList* FindElement(LPCTSTR strElement); //원소 찾기
		IXMLDOMNodeList* FindElement(IXMLDOMNode* pNode, LPCTSTR strElement); //요소 찾기
		bool GetElementText(IXMLDOMNode* pNode, LPSTR strRet); //원소의 택스트 넣기
		int	GetAttributeText(IXMLDOMNode* pNode, LPSTR strAttrName, LPSTR strRet);

	private:
		IXMLDOMDocument* pXMLDoc; 
		wchar_t sizeFilename[MAX_PATH];
	};;
};