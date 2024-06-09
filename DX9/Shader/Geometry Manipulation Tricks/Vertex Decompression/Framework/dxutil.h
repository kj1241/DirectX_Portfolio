#ifndef DXUTIL_H
#define DXUTIL_H

// 기타 도우미 기능
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


#ifndef UNDER_CE
// 시스템 레지스트리에 저장된 DirectX SDK 경로를 반환합니다.
// SDK 설치 중.
HRESULT DXUtil_GetDXSDKMediaPathCch( TCHAR* strDest, int cchDest );
HRESULT DXUtil_GetDXSDKMediaPathCb( TCHAR* szDest, int cbDest );
HRESULT DXUtil_FindMediaFileCch( TCHAR* strDestPath, int cchDest, TCHAR* strFilename );
HRESULT DXUtil_FindMediaFileCb( TCHAR* szDestPath, int cbDest, TCHAR* strFilename );
#endif // !UNDER_CE

//문자열 레지스트리 키를 읽고 쓰는 도우미 함수
HRESULT DXUtil_WriteStringRegKey( HKEY hKey, TCHAR* strRegName, TCHAR* strValue );
HRESULT DXUtil_WriteIntRegKey( HKEY hKey, TCHAR* strRegName, DWORD dwValue );
HRESULT DXUtil_WriteGuidRegKey( HKEY hKey, TCHAR* strRegName, GUID guidValue );
HRESULT DXUtil_WriteBoolRegKey( HKEY hKey, TCHAR* strRegName, BOOL bValue );

HRESULT DXUtil_ReadStringRegKeyCch( HKEY hKey, TCHAR* strRegName, TCHAR* strDest, DWORD cchDest, TCHAR* strDefault );
HRESULT DXUtil_ReadStringRegKeyCb( HKEY hKey, TCHAR* strRegName, TCHAR* strDest, DWORD cbDest, TCHAR* strDefault );
HRESULT DXUtil_ReadIntRegKey( HKEY hKey, TCHAR* strRegName, DWORD* pdwValue, DWORD dwDefault );
HRESULT DXUtil_ReadGuidRegKey( HKEY hKey, TCHAR* strRegName, GUID* pGuidValue, GUID& guidDefault );
HRESULT DXUtil_ReadBoolRegKey( HKEY hKey, TCHAR* strRegName, BOOL* pbValue, BOOL bDefault );

// 타이머 작업을 수행합니다. 다음 명령을 사용하십시오.
// TIMER_RESET - 타이머를 재설정합니다.
// TIMER_START - 타이머를 시작합니다.
// TIMER_STOP - 타이머를 중지(또는 일시 중지)합니다.
// TIMER_ADVANCE - 타이머를 0.1초 앞당깁니다.
// TIMER_GETABSOLUTETIME - 절대 시스템 시간을 가져옵니다.
// TIMER_GETAPPTIME - 현재 시간을 가져옵니다.
// TIMER_GETELAPSEDTIME - 경과된 시간을 가져옵니다. 
// TIMER_GETELAPSEDTIME 호출
enum TIMER_COMMAND { 
    TIMER_RESET, 
    TIMER_START, 
    TIMER_STOP, 
    TIMER_ADVANCE, 
    TIMER_GETABSOLUTETIME, 
    TIMER_GETAPPTIME, 
    TIMER_GETELAPSEDTIME 
};
FLOAT __stdcall DXUtil_Timer( TIMER_COMMAND command );

//CHAR, TCHAR, WCHAR 문자열 간 변환을 위한 UNICODE 지원
HRESULT DXUtil_ConvertAnsiStringToWideCch( WCHAR* wstrDestination, const CHAR* strSource, int cchDestChar );
HRESULT DXUtil_ConvertWideStringToAnsiCch( CHAR* strDestination, const WCHAR* wstrSource, int cchDestChar );
HRESULT DXUtil_ConvertGenericStringToAnsiCch( CHAR* strDestination, const TCHAR* tstrSource, int cchDestChar );
HRESULT DXUtil_ConvertGenericStringToWideCch( WCHAR* wstrDestination, const TCHAR* tstrSource, int cchDestChar );
HRESULT DXUtil_ConvertAnsiStringToGenericCch( TCHAR* tstrDestination, const CHAR* strSource, int cchDestChar );
HRESULT DXUtil_ConvertWideStringToGenericCch( TCHAR* tstrDestination, const WCHAR* wstrSource, int cchDestChar );
HRESULT DXUtil_ConvertAnsiStringToWideCb( WCHAR* wstrDestination, const CHAR* strSource, int cbDestChar );
HRESULT DXUtil_ConvertWideStringToAnsiCb( CHAR* strDestination, const WCHAR* wstrSource, int cbDestChar );
HRESULT DXUtil_ConvertGenericStringToAnsiCb( CHAR* strDestination, const TCHAR* tstrSource, int cbDestChar );
HRESULT DXUtil_ConvertGenericStringToWideCb( WCHAR* wstrDestination, const TCHAR* tstrSource, int cbDestChar );
HRESULT DXUtil_ConvertAnsiStringToGenericCb( TCHAR* tstrDestination, const CHAR* strSource, int cbDestChar );
HRESULT DXUtil_ConvertWideStringToGenericCb( TCHAR* tstrDestination, const WCHAR* wstrSource, int cbDestChar );

// 읽어보기 기능
VOID DXUtil_LaunchReadme( HWND hWnd, TCHAR* strLoc = NULL );

// GUID를 문자열로 변환
HRESULT DXUtil_ConvertGUIDToStringCch( const GUID* pGuidSrc, TCHAR* strDest, int cchDestChar );
HRESULT DXUtil_ConvertGUIDToStringCb( const GUID* pGuidSrc, TCHAR* strDest, int cbDestChar );
HRESULT DXUtil_ConvertStringToGUID( const TCHAR* strIn, GUID* pGuidOut );

// 추가 디버그 인쇄 지원은 dxerr9.h를 참조하세요.
VOID    DXUtil_Trace( TCHAR* strMsg, ... );

#if defined(DEBUG) | defined(_DEBUG)
    #define DXTRACE           DXUtil_Trace
#else
    #define DXTRACE           sizeof
#endif

// CArrayList에 데이터를 저장하는 방법을 나타냅니다.
enum ArrayListType
{
    AL_VALUE,       // 항목 데이터가 목록에 복사됩니다.
    AL_REFERENCE,    // 항목 포인터가 목록에 복사됩니다.
};

//확장 가능한 배열
class CArrayList
{
protected:
    ArrayListType m_ArrayListType;
    void* m_pData;
    UINT m_BytesPerEntry;
    UINT m_NumEntries;
    UINT m_NumEntriesAllocated;

public:
    CArrayList( ArrayListType Type, UINT BytesPerEntry = 0 );
    ~CArrayList( void );
    HRESULT Add( void* pEntry );
    void Remove( UINT Entry );
    void* GetPtr( UINT Entry );
    UINT Count( void ) { return m_NumEntries; }
    bool Contains( void* pEntryData );
    void Clear( void ) { m_NumEntries = 0; }
};

//WinCE 빌드 지원

#ifdef UNDER_CE

#define CheckDlgButton(hdialog, id, state) ::SendMessage(::GetDlgItem(hdialog, id), BM_SETCHECK, state, 0)
#define IsDlgButtonChecked(hdialog, id) ::SendMessage(::GetDlgItem(hdialog, id), BM_GETCHECK, 0L, 0L)
#define GETTIMESTAMP GetTickCount
#define _TWINCE(x) _T(x)

__inline int GetScrollPos(HWND hWnd, int nBar)
{
	SCROLLINFO si;
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	if (!GetScrollInfo(hWnd, nBar, &si))
	{
		return 0;
	}
	else
	{
		return si.nPos;
	}
}

#else // !UNDER_CE

#define GETTIMESTAMP timeGetTime
#define _TWINCE(x) x

#endif // UNDER_CE

#endif // DXUTIL_H
