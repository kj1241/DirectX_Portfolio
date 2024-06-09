//DX 개체 사용을 위한 바로가기 매크로 및 기능
#ifndef STRICT
#define STRICT
#endif // !STRICT
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#include <stdio.h> 
#include <stdarg.h>
#include "DXUtil.h"


#ifdef UNICODE
    typedef HINSTANCE (WINAPI* LPShellExecute)(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
#else
    typedef HINSTANCE (WINAPI* LPShellExecute)(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
#endif


#ifndef UNDER_CE
//DirectX SDK 미디어 경로를 반환합니다.
//cchDest는 strDest의 TCHAR 크기입니다.  
//UNICODE 빌드에서는 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_GetDXSDKMediaPathCch( TCHAR* strDest, int cchDest )
{
    if( strDest == NULL || cchDest < 1 )
        return E_INVALIDARG;

    lstrcpy( strDest, TEXT("") );

    // 적절한 레지스트리 키를 엽니다.
    HKEY  hKey;
    LONG lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                _T("Software\\Microsoft\\DirectX SDK"),
                                0, KEY_READ, &hKey );
    if( ERROR_SUCCESS != lResult )
        return E_FAIL;

    DWORD dwType;
    DWORD dwSize = cchDest * sizeof(TCHAR);
    lResult = RegQueryValueEx( hKey, _T("DX9SDK Samples Path"), NULL,
                              &dwType, (BYTE*)strDest, &dwSize );
    strDest[cchDest-1] = 0; // 버퍼가 너무 작은 경우 RegQueryValueEx는 NULL 용어가 아닙니다.
    RegCloseKey( hKey );

    if( ERROR_SUCCESS != lResult )
        return E_FAIL;

    const TCHAR* strMedia = _T("\\Media\\");
    if( lstrlen(strDest) + lstrlen(strMedia) < cchDest )
        _tcscat( strDest, strMedia );
    else
        return E_INVALIDARG;

    return S_OK;
}
#endif // !UNDER_CE

#ifndef UNDER_CE
// DXSDK 미디어 파일에 대한 유효한 경로를 반환합니다.
// cchDest는 strDestPath의 TCHAR 크기입니다. 
// UNICODE 빌드에서는 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_FindMediaFileCch( TCHAR* strDestPath, int cchDest, TCHAR* strFilename )
{
    HRESULT hr;
    HANDLE file;
    TCHAR* strShortNameTmp = NULL;
    TCHAR strShortName[MAX_PATH];
    int cchPath;

    if( NULL==strFilename || NULL==strDestPath || cchDest < 1 )
        return E_INVALIDARG;

    lstrcpy( strDestPath, TEXT("") );
    lstrcpy( strShortName, TEXT("") );

    // strFileName에서 전체 경로 이름을 만듭니다. (strShortName은 리프 파일 이름이 됩니다.)
    cchPath = GetFullPathName(strFilename, cchDest, strDestPath, &strShortNameTmp);
    if ((cchPath == 0) || (cchDest <= cchPath))
        return E_FAIL;
    if( strShortNameTmp )
        lstrcpyn( strShortName, strShortNameTmp, MAX_PATH );

    // 먼저 전체 경로가 주어진 파일 이름을 찾으려고 시도합니다.
    file = CreateFile( strDestPath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                       OPEN_EXISTING, 0, NULL );
    if( INVALID_HANDLE_VALUE != file )
    {
        CloseHandle( file );
        return S_OK;
    }

    // 다음으로 현재 작업 디렉터리에서 파일 이름을 찾으려고 시도합니다(경로 제거됨).
    file = CreateFile( strShortName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                       OPEN_EXISTING, 0, NULL );
    if( INVALID_HANDLE_VALUE != file )
    {
        _tcsncpy( strDestPath, strShortName, cchDest );
        strDestPath[cchDest-1] = 0;        // _tcsncpy는 공간이 부족해도 NULL 항을 사용하지 않습니다.
        CloseHandle( file );
        return S_OK;
    }
    
    // 마지막으로 파일이 미디어 디렉터리에 있는지 확인합니다.
    if( FAILED( hr = DXUtil_GetDXSDKMediaPathCch( strDestPath, cchDest ) ) )
        return hr;

    if( lstrlen(strDestPath) + lstrlen(strShortName) < cchDest )
        lstrcat( strDestPath, strShortName );
    else
        return E_INVALIDARG;

    file = CreateFile( strDestPath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                       OPEN_EXISTING, 0, NULL );
    if( INVALID_HANDLE_VALUE != file )
    {
        CloseHandle( file );
        return S_OK;
    }

    //실패하면 파일을 경로로 반환합니다.
    _tcsncpy( strDestPath, strFilename, cchDest );
    strDestPath[cchDest-1] = 0;    // _tcsncpy는 공간이 부족해도 NULL 항을 사용하지 않습니다.
    return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
}
#endif // !UNDER_CE

// 레지스트리 키 문자열을 읽는 도우미 함수
// cchDest는 strDest의 TCHAR 크기입니다.
// UNICODE 빌드에서는 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ReadStringRegKeyCch( HKEY hKey, TCHAR* strRegName, TCHAR* strDest, 
                                    DWORD cchDest, TCHAR* strDefault )
{
    DWORD dwType;
    DWORD cbDest = cchDest * sizeof(TCHAR);

    if( ERROR_SUCCESS != RegQueryValueEx( hKey, strRegName, 0, &dwType, (BYTE*)strDest, &cbDest ) )
    {
        _tcsncpy( strDest, strDefault, cchDest );
        strDest[cchDest-1] = 0;

        if( dwType != REG_SZ )
            return E_FAIL;

        return S_OK;
    }
    return E_FAIL;
}

// 레지스트리 키 문자열을 작성하는 도우미 함수
HRESULT DXUtil_WriteStringRegKey( HKEY hKey, TCHAR* strRegName,
                                  TCHAR* strValue )
{
    if( NULL == strValue )
        return E_INVALIDARG;
        
    DWORD cbValue = ((DWORD)_tcslen(strValue)+1) * sizeof(TCHAR);

    if( ERROR_SUCCESS != RegSetValueEx( hKey, strRegName, 0, REG_SZ, (BYTE*)strValue, cbValue ) )
        return E_FAIL;
    return S_OK;
}

//레지스트리 키 int를 읽는 도우미 함수
HRESULT DXUtil_ReadIntRegKey( HKEY hKey, TCHAR* strRegName, DWORD* pdwDest, 
                              DWORD dwDefault )
{
    DWORD dwType;
    DWORD dwLength = sizeof(DWORD);

    if( ERROR_SUCCESS != RegQueryValueEx( hKey, strRegName, 0, &dwType, (BYTE*)pdwDest, &dwLength ) )
    {
        *pdwDest = dwDefault;
        if( dwType != REG_DWORD )
            return E_FAIL;

        return S_OK;
    }
    return E_FAIL;
}

//레지스트리 키 int를 작성하는 도우미 함수
HRESULT DXUtil_WriteIntRegKey( HKEY hKey, TCHAR* strRegName, DWORD dwValue )
{
    if( ERROR_SUCCESS != RegSetValueEx( hKey, strRegName, 0, REG_DWORD, (BYTE*)&dwValue, sizeof(DWORD) ) )
        return E_FAIL;
    return S_OK;
}

//레지스트리 키 BOOL을 읽는 도우미 기능
HRESULT DXUtil_ReadBoolRegKey( HKEY hKey, TCHAR* strRegName, BOOL* pbDest, BOOL bDefault )
{
    DWORD dwType;
    DWORD dwLength = sizeof(BOOL);

    if( ERROR_SUCCESS != RegQueryValueEx( hKey, strRegName, 0, &dwType, (BYTE*)pbDest, &dwLength ) )
    {
        *pbDest = bDefault;
        if( dwType != REG_DWORD )
            return E_FAIL;

        return S_OK;
    }
    return E_FAIL;
}

//레지스트리 키 BOOL을 작성하는 도우미 기능
HRESULT DXUtil_WriteBoolRegKey( HKEY hKey, TCHAR* strRegName, BOOL bValue )
{
    if( ERROR_SUCCESS != RegSetValueEx( hKey, strRegName, 0, REG_DWORD, (BYTE*)&bValue, sizeof(BOOL) ) )
        return E_FAIL;
    return S_OK;
}

//레지스트리 키 GUID를 읽는 도우미 기능
HRESULT DXUtil_ReadGuidRegKey( HKEY hKey, TCHAR* strRegName, GUID* pGuidDest, GUID& guidDefault )
{
    DWORD dwType;
    DWORD dwLength = sizeof(GUID);

    if( ERROR_SUCCESS != RegQueryValueEx( hKey, strRegName, 0, &dwType, (LPBYTE) pGuidDest, &dwLength ) )
    {
        *pGuidDest = guidDefault;
        if( dwType != REG_BINARY )
            return E_FAIL;

        return S_OK;
    }
    return E_FAIL;
}

//레지스트리 키 GUID를 작성하는 도우미 기능
HRESULT DXUtil_WriteGuidRegKey( HKEY hKey, TCHAR* strRegName, GUID guidValue )
{
    if( ERROR_SUCCESS != RegSetValueEx( hKey, strRegName, 0, REG_BINARY, (BYTE*)&guidValue, sizeof(GUID) ) )
        return E_FAIL;
    return S_OK;
}

// 타이머 작업을 수행합니다. 다음 명령을 사용하십시오.
// TIMER_RESET - 타이머를 재설정합니다.
// TIMER_START - 타이머를 시작합니다.
// TIMER_STOP - 타이머를 중지(또는 일시 중지)합니다.
// TIMER_ADVANCE - 타이머를 0.1초 앞당깁니다.
// TIMER_GETABSOLUTETIME - 절대 시스템 시간을 가져옵니다.
// TIMER_GETAPPTIME - 현재 시간을 가져옵니다.
// TIMER_GETELAPSEDTIME - 경과된 시간을 가져옵니다. 
// TIMER_GETELAPSEDTIME 호출
FLOAT __stdcall DXUtil_Timer( TIMER_COMMAND command )
{
    static BOOL     m_bTimerInitialized = FALSE;
    static BOOL     m_bUsingQPF         = FALSE;
    static BOOL     m_bTimerStopped     = TRUE;
    static LONGLONG m_llQPFTicksPerSec  = 0;

    // 타이머 초기화
    if( FALSE == m_bTimerInitialized )
    {
        m_bTimerInitialized = TRUE;

        // QueryPerformanceFrequency()를 사용하여 타이머의 빈도를 가져옵니다.  
        // QPF가 다음과 같은 경우 지원되지 않습니다. 
        // 밀리초를 반환하는 timeGetTime()을 수행합니다.
        LARGE_INTEGER qwTicksPerSec;
        m_bUsingQPF = QueryPerformanceFrequency( &qwTicksPerSec );
        if( m_bUsingQPF )
            m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
    }

    if( m_bUsingQPF )
    {
        static LONGLONG m_llStopTime        = 0;
        static LONGLONG m_llLastElapsedTime = 0;
        static LONGLONG m_llBaseTime        = 0;
        double fTime;
        double fElapsedTime;
        LARGE_INTEGER qwTime;
        
        // 상황에 따라 현재 시간이나 정지 시간을 가져옵니다.
        // 중지되었는지 여부와 어떤 명령이 전송되었는지에 대해
        if( m_llStopTime != 0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME)
            qwTime.QuadPart = m_llStopTime;
        else
            QueryPerformanceCounter( &qwTime );

        // 경과된 시간을 반환
        if( command == TIMER_GETELAPSEDTIME )
        {
            fElapsedTime = (double) ( qwTime.QuadPart - m_llLastElapsedTime ) / (double) m_llQPFTicksPerSec;
            m_llLastElapsedTime = qwTime.QuadPart;
            return (FLOAT) fElapsedTime;
        }
    
        // 현재 시간을 반환
        if( command == TIMER_GETAPPTIME )
        {
            double fAppTime = (double) ( qwTime.QuadPart - m_llBaseTime ) / (double) m_llQPFTicksPerSec;
            return (FLOAT) fAppTime;
        }
    
        // 타이머 재설정
        if( command == TIMER_RESET )
        {
            m_llBaseTime        = qwTime.QuadPart;
            m_llLastElapsedTime = qwTime.QuadPart;
            m_llStopTime        = 0;
            m_bTimerStopped     = FALSE;
            return 0.0f;
        }
    
        // 타이머 시작
        if( command == TIMER_START )
        {
            if( m_bTimerStopped )
                m_llBaseTime += qwTime.QuadPart - m_llStopTime;
            m_llStopTime = 0;
            m_llLastElapsedTime = qwTime.QuadPart;
            m_bTimerStopped = FALSE;
            return 0.0f;
        }
    
        // 타이머를 중지합니다.
        if( command == TIMER_STOP )
        {
            if( !m_bTimerStopped )
            {
                m_llStopTime = qwTime.QuadPart;
                m_llLastElapsedTime = qwTime.QuadPart;
                m_bTimerStopped = TRUE;
            }
            return 0.0f;
        }
    
        // 타이머를 1/10초만큼 앞당깁니다.
        if( command == TIMER_ADVANCE )
        {
            m_llStopTime += m_llQPFTicksPerSec/10;
            return 0.0f;
        }

        if( command == TIMER_GETABSOLUTETIME )
        {
            fTime = qwTime.QuadPart / (double) m_llQPFTicksPerSec;
            return (FLOAT) fTime;
        }

        return -1.0f; // 잘못된 명령이 지정되었습니다.
    }
    else
    {
        // timeGetTime()을 사용하여 시간을 가져옵니다.
        static double m_fLastElapsedTime  = 0.0;
        static double m_fBaseTime         = 0.0;
        static double m_fStopTime         = 0.0;
        double fTime;
        double fElapsedTime;
        
        // 상황에 따라 현재 시간이나 정지 시간을 가져옵니다.
        // 중지되었는지 여부와 어떤 명령이 전송되었는지에 대해
        if( m_fStopTime != 0.0 && command != TIMER_START && command != TIMER_GETABSOLUTETIME)
            fTime = m_fStopTime;
        else
            fTime = GETTIMESTAMP() * 0.001;
    
        // 경과된 시간을 반환
        if( command == TIMER_GETELAPSEDTIME )
        {   
            fElapsedTime = (double) (fTime - m_fLastElapsedTime);
            m_fLastElapsedTime = fTime;
            return (FLOAT) fElapsedTime;
        }

        // 현재 시간을 반환
        if( command == TIMER_GETAPPTIME )
            return (FLOAT) (fTime - m_fBaseTime);
        
    
        // 타이머 재설정
        if( command == TIMER_RESET )
        {
            m_fBaseTime         = fTime;
            m_fLastElapsedTime  = fTime;
            m_fStopTime         = 0;
            m_bTimerStopped     = FALSE;
            return 0.0f;
        }
    
        // 타이머 시작
        if( command == TIMER_START )
        {
            if( m_bTimerStopped )
                m_fBaseTime += fTime - m_fStopTime;
            m_fStopTime = 0.0f;
            m_fLastElapsedTime  = fTime;
            m_bTimerStopped = FALSE;
            return 0.0f;
        }
    
        // 타이머를 중지합니다.
        if( command == TIMER_STOP )
        {
            if( !m_bTimerStopped )
            {
                m_fStopTime = fTime;
                m_fLastElapsedTime  = fTime;
                m_bTimerStopped = TRUE;
            }
            return 0.0f;
        }
    
        // 타이머를 1/10초만큼 앞당깁니다.
        if( command == TIMER_ADVANCE )
        {
            m_fStopTime += 0.1f;
            return 0.0f;
        }

        if( command == TIMER_GETABSOLUTETIME )
            return (FLOAT) fTime;
        
        return -1.0f; // Invalid command specified
    }
}

// 이것은 CHAR 문자열을 CHAR 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// WCHAR 문자열.
// cchDestChar는 wstrDestination의 TCHAR 크기입니다.
// sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ConvertAnsiStringToWideCch( WCHAR* wstrDestination, const CHAR* strSource, int cchDestChar )
{
    if( wstrDestination==NULL || strSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

    int nResult = MultiByteToWideChar( CP_ACP, 0, strSource, -1, wstrDestination, cchDestChar );
    wstrDestination[cchDestChar-1] = 0;
    
    if( nResult == 0 )
        return E_FAIL;
    return S_OK;
}

// WCHAR 문자열을 WCHAR 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// CHAR 문자열. 
// cchDestChar는 strDestination의 TCHAR 크기입니다.
HRESULT DXUtil_ConvertWideStringToAnsiCch( CHAR* strDestination, const WCHAR* wstrSource, int cchDestChar )
{
    if( strDestination==NULL || wstrSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

    int nResult = WideCharToMultiByte( CP_ACP, 0, wstrSource, -1, strDestination, cchDestChar*sizeof(CHAR), NULL, NULL );
    strDestination[cchDestChar-1] = 0;
    
    if( nResult == 0 )
        return E_FAIL;
    return S_OK;
}

// TCHAR 문자열을 TCHAR 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// CHAR 문자열. 
// cchDestChar는 strDestination의 TCHAR 크기입니다.
HRESULT DXUtil_ConvertGenericStringToAnsiCch( CHAR* strDestination, const TCHAR* tstrSource, int cchDestChar )
{
    if( strDestination==NULL || tstrSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

#ifdef _UNICODE
    return DXUtil_ConvertWideStringToAnsiCch( strDestination, tstrSource, cchDestChar );
#else
    strncpy( strDestination, tstrSource, cchDestChar );
    strDestination[cchDestChar-1] = '\0';
    return S_OK;
#endif   
}

// 이것은 TCHAR 문자열을 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// WCHAR 문자열. 
// cchDestChar는 wstrDestination의 TCHAR 크기입니다.
// sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ConvertGenericStringToWideCch( WCHAR* wstrDestination, const TCHAR* tstrSource, int cchDestChar )
{
    if( wstrDestination==NULL || tstrSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

#ifdef _UNICODE
    wcsncpy( wstrDestination, tstrSource, cchDestChar );
    wstrDestination[cchDestChar-1] = L'\0';
    return S_OK;
#else
    return DXUtil_ConvertAnsiStringToWideCch( wstrDestination, tstrSource, cchDestChar );
#endif    
}


// CHAR 문자열을 CHAR 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// TCHAR 문자열. 
// cchDestChar는 tstrDestination의 TCHAR 크기입니다.
// UNICODE 빌드에서 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ConvertAnsiStringToGenericCch( TCHAR* tstrDestination, const CHAR* strSource, int cchDestChar )
{
    if( tstrDestination==NULL || strSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;
        
#ifdef _UNICODE
    return DXUtil_ConvertAnsiStringToWideCch( tstrDestination, strSource, cchDestChar );
#else
    strncpy( tstrDestination, strSource, cchDestChar );
    tstrDestination[cchDestChar-1] = '\0';
    return S_OK;
#endif    
}

// WCHAR 문자열을 WCHAR 문자열로 변환하는 UNICODE 변환 유틸리티입니다.
// TCHAR 문자열. 
// cchDestChar는 tstrDestination의 TCHAR 크기입니다. 
// UNICODE 빌드에서 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ConvertWideStringToGenericCch( TCHAR* tstrDestination, const WCHAR* wstrSource, int cchDestChar )
{
    if( tstrDestination==NULL || wstrSource==NULL || cchDestChar < 1 )
        return E_INVALIDARG;

#ifdef _UNICODE
    wcsncpy( tstrDestination, wstrSource, cchDestChar );
    tstrDestination[cchDestChar-1] = L'\0';    
    return S_OK;
#else
    return DXUtil_ConvertWideStringToAnsiCch( tstrDestination, wstrSource, cchDestChar );
#endif
}

//이 샘플에 대한 readme.txt를 찾아 엽니다.
//어짜피 이함수는 쓰고자하면 고쳐야됨
VOID DXUtil_LaunchReadme( HWND hWnd, TCHAR* strLoc )
{

#ifdef UNDER_CE
    // This is not available on PocketPC
    MessageBox( hWnd, TEXT("For operating instructions, please open the ")
                      TEXT("readme.txt file included with the project."), TEXT("DirectX SDK Sample"), MB_ICONWARNING | MB_OK );

    return;
#else 

    bool bSuccess = false;
    bool bFound = false;
    TCHAR strReadmePath[1024];
    TCHAR strExeName[MAX_PATH];
    TCHAR strExePath[MAX_PATH];
    TCHAR strSamplePath[MAX_PATH];
    TCHAR* strLastSlash = NULL;

    lstrcpy( strReadmePath, TEXT("") );
    lstrcpy( strExePath, TEXT("") );
    lstrcpy( strExeName, TEXT("") );
    lstrcpy( strSamplePath, TEXT("") );

    // 사용자가 추가 정보의 위치를 ​​제공한 경우 먼저 해당 위치를 확인하세요.
    if( strLoc )
    {
        HKEY  hKey;
        LONG lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\DirectX SDK"), 0, KEY_READ, &hKey );
        if( ERROR_SUCCESS == lResult )
        {
            DWORD dwType;
            DWORD dwSize = MAX_PATH * sizeof(TCHAR);
            lResult = RegQueryValueEx( hKey, _T("DX9SDK Samples Path"), NULL, &dwType, (BYTE*)strSamplePath, &dwSize );
            strSamplePath[MAX_PATH-1] = 0;  // 버퍼가 너무 작은 경우 RegQueryValueEx는 NULL 용어가 아닙니다.
            
            if( ERROR_SUCCESS == lResult )
            {
                _sntprintf( strReadmePath, 1023, TEXT("%s\\C++\\%s\\readme.txt"), 
                            strSamplePath, strLoc );
                strReadmePath[1023] = 0;

                if( GetFileAttributes( strReadmePath ) != 0xFFFFFFFF )
                    bFound = TRUE;
            }
        }
        RegCloseKey( hKey );
    }

    // exe 이름과 exe 경로를 가져옵니다.
    GetModuleFileName( NULL, strExePath, MAX_PATH );
    strExePath[MAX_PATH-1]=0;

    strLastSlash = _tcsrchr( strExePath, TEXT('\\') );
    if( strLastSlash )
    {
        _tcsncpy( strExeName, &strLastSlash[1], MAX_PATH );
        strExeName[MAX_PATH-1]=0;

        // exe 경로에서 exe 이름을 잘라냅니다.
        *strLastSlash = 0;

        // exe 이름에서 .exe를 잘라냅니다.
        strLastSlash = _tcsrchr( strExeName, TEXT('.') );
        if( strLastSlash )
            *strLastSlash = 0;
    }

    if( !bFound )
    {
        // "%EXE_DIR%\..\%EXE_NAME%"에서 검색합니다.  
        // 이는 DirectX SDK 레이아웃과 일치합니다.
        _tcscpy( strReadmePath, strExePath );

        strLastSlash = _tcsrchr( strReadmePath, TEXT('\\') );
        if( strLastSlash )
            *strLastSlash = 0;
        lstrcat( strReadmePath, TEXT("\\") );
        lstrcat( strReadmePath, strExeName );
        lstrcat( strReadmePath, TEXT("\\readme.txt") );
        if( GetFileAttributes( strReadmePath ) != 0xFFFFFFFF )
            bFound = TRUE;
    }

    if( !bFound )
    {
        // "%EXE_DIR%\"에서 검색
        _tcscpy( strReadmePath, strExePath );
        lstrcat( strReadmePath, TEXT("\\readme.txt") );
        if( GetFileAttributes( strReadmePath ) != 0xFFFFFFFF )
            bFound = TRUE;
    }

    if( !bFound )
    {
        // "%EXE_DIR%\.."에서 검색합니다.
        _tcscpy( strReadmePath, strExePath );
        strLastSlash = _tcsrchr( strReadmePath, TEXT('\\') );
        if( strLastSlash )
            *strLastSlash = 0;
        lstrcat( strReadmePath, TEXT("\\readme.txt") );
        if( GetFileAttributes( strReadmePath ) != 0xFFFFFFFF )
            bFound = TRUE;
    }

    if( !bFound )
    {
        // "%EXE_DIR%\..\.."에서 검색합니다.
        _tcscpy( strReadmePath, strExePath );
        strLastSlash = _tcsrchr( strReadmePath, TEXT('\\') );
        if( strLastSlash )
            *strLastSlash = 0;
        strLastSlash = _tcsrchr( strReadmePath, TEXT('\\') );
        if( strLastSlash )
            *strLastSlash = 0;
        lstrcat( strReadmePath, TEXT("\\readme.txt") );
        if( GetFileAttributes( strReadmePath ) != 0xFFFFFFFF )
            bFound = TRUE;
    }

    if( bFound )
    {
        // ShellExecute에 대한 GetProcAddress이므로 shell32.lib를 포함할 필요가 없습니다. 
        // dxutil.cpp를 사용하는 모든 프로젝트에서
        LPShellExecute pShellExecute = NULL;
        HINSTANCE hInstShell32 = LoadLibrary(TEXT("shell32.dll"));
        if (hInstShell32 != NULL)
        {
#ifdef UNICODE
            pShellExecute = (LPShellExecute)GetProcAddress(hInstShell32, _TWINCE("ShellExecuteW"));
#else
            pShellExecute = (LPShellExecute)GetProcAddress(hInstShell32, _TWINCE("ShellExecuteA"));
#endif
            if( pShellExecute != NULL )
            {
                if( pShellExecute( hWnd, TEXT("open"), strReadmePath, NULL, NULL, SW_SHOW ) > (HINSTANCE) 32 )
                    bSuccess = true;
            }

            FreeLibrary(hInstShell32);
        }
    }

    if( !bSuccess )
    {
        // 사용자에게 추가 정보를 열 수 없다고 알립니다.
        MessageBox( hWnd, TEXT("Could not find readme.txt"), 
                    TEXT("DirectX SDK Sample"), MB_ICONWARNING | MB_OK );
    }

#endif // UNDER_CE
}

// 변수가 포함된 형식화된 문자열을 디버그 스트림에 출력합니다.
// 인수 목록.
VOID DXUtil_Trace( TCHAR* strMsg, ... )
{
#if defined(DEBUG) | defined(_DEBUG)
    TCHAR strBuffer[512];
    
    va_list args;
    va_start(args, strMsg);
    _vsntprintf( strBuffer, 512, strMsg, args );
    va_end(args);

    OutputDebugString( strBuffer );
#else
    UNREFERENCED_PARAMETER(strMsg);
#endif
}

// 문자열을 GUID로 변환합니다.
HRESULT DXUtil_ConvertStringToGUID( const TCHAR* strSrc, GUID* pGuidDest )
{
    UINT aiTmp[10];

    if( _stscanf( strSrc, TEXT("{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}"), &pGuidDest->Data1, &aiTmp[0], &aiTmp[1],  &aiTmp[2], &aiTmp[3],
                  &aiTmp[4], &aiTmp[5], &aiTmp[6], &aiTmp[7], &aiTmp[8], &aiTmp[9] ) != 11 )
    {
        ZeroMemory( pGuidDest, sizeof(GUID) );
        return E_FAIL;
    }
    else
    {
        pGuidDest->Data2       = (USHORT) aiTmp[0];
        pGuidDest->Data3       = (USHORT) aiTmp[1];
        pGuidDest->Data4[0]    = (BYTE) aiTmp[2];
        pGuidDest->Data4[1]    = (BYTE) aiTmp[3];
        pGuidDest->Data4[2]    = (BYTE) aiTmp[4];
        pGuidDest->Data4[3]    = (BYTE) aiTmp[5];
        pGuidDest->Data4[4]    = (BYTE) aiTmp[6];
        pGuidDest->Data4[5]    = (BYTE) aiTmp[7];
        pGuidDest->Data4[6]    = (BYTE) aiTmp[8];
        pGuidDest->Data4[7]    = (BYTE) aiTmp[9];
        return S_OK;
    }
}

// GUID를 문자열로 변환합니다. 
// cchDestChar는 strDest의 TCHAR 크기입니다. 
// UNICODE 빌드에서 sizeof(strDest)를 전달합니다.
HRESULT DXUtil_ConvertGUIDToStringCch( const GUID* pGuidSrc, TCHAR* strDest, int cchDestChar )
{
    int nResult = _sntprintf( strDest, cchDestChar, TEXT("{%0.8X-%0.4X-%0.4X-%0.2X%0.2X-%0.2X%0.2X%0.2X%0.2X%0.2X%0.2X}"),
                              pGuidSrc->Data1, pGuidSrc->Data2, pGuidSrc->Data3, pGuidSrc->Data4[0], pGuidSrc->Data4[1],  
                              pGuidSrc->Data4[2], pGuidSrc->Data4[3],pGuidSrc->Data4[4], pGuidSrc->Data4[5], pGuidSrc->Data4[6], pGuidSrc->Data4[7] );
    if( nResult < 0 )
        return E_FAIL;
    return S_OK;
}

CArrayList::CArrayList( ArrayListType Type, UINT BytesPerEntry )
{
    if( Type == AL_REFERENCE )
        BytesPerEntry = sizeof(void*);
    m_ArrayListType = Type;
    m_pData = NULL;
    m_BytesPerEntry = BytesPerEntry;
    m_NumEntries = 0;
    m_NumEntriesAllocated = 0;
}

CArrayList::~CArrayList( void )
{
    if( m_pData != NULL )
        delete[] m_pData;
}

//목록에 pEntry를 추가합니다.
HRESULT CArrayList::Add( void* pEntry )
{
    if( m_BytesPerEntry == 0 )
        return E_FAIL;
    if( m_pData == NULL || m_NumEntries + 1 > m_NumEntriesAllocated )
    {
        void* pDataNew;
        UINT NumEntriesAllocatedNew;
        if( m_NumEntriesAllocated == 0 )
            NumEntriesAllocatedNew = 16;
        else
            NumEntriesAllocatedNew = m_NumEntriesAllocated * 2;
        pDataNew = new BYTE[NumEntriesAllocatedNew * m_BytesPerEntry];
        if( pDataNew == NULL )
            return E_OUTOFMEMORY;
        if( m_pData != NULL )
        {
            CopyMemory( pDataNew, m_pData, m_NumEntries * m_BytesPerEntry );
            delete[] m_pData;
        }
        m_pData = pDataNew;
        m_NumEntriesAllocated = NumEntriesAllocatedNew;
    }

    if( m_ArrayListType == AL_VALUE )
        CopyMemory( (BYTE*)m_pData + (m_NumEntries * m_BytesPerEntry), pEntry, m_BytesPerEntry );
    else
        *(((void**)m_pData) + m_NumEntries) = pEntry;
    m_NumEntries++;

    return S_OK;
}

//목록의 항목에서 항목을 제거하고 배열을 축소합니다.
void CArrayList::Remove( UINT Entry )
{
    // 감소 카운트
    m_NumEntries--;

    // Find the entry address
    BYTE* pData = (BYTE*)m_pData + (Entry * m_BytesPerEntry);

    // Collapse the array
    MoveMemory( pData, pData + m_BytesPerEntry, ( m_NumEntries - Entry ) * m_BytesPerEntry );
}

// 목록의 항목 번째 항목에 대한 포인터를 반환합니다.
void* CArrayList::GetPtr( UINT Entry )
{
    if( m_ArrayListType == AL_VALUE )
        return (BYTE*)m_pData + (Entry * m_BytesPerEntry);
    else
        return *(((void**)m_pData) + Entry);
}

// 목록에 항목과 동일한 항목이 포함되어 있는지 여부를 반환합니다. 
// 지정된 항목 데이터.
bool CArrayList::Contains( void* pEntryData )
{
    for( UINT iEntry = 0; iEntry < m_NumEntries; iEntry++ )
    {
        if( m_ArrayListType == AL_VALUE )
        {
            if( memcmp( GetPtr(iEntry), pEntryData, m_BytesPerEntry ) == 0 )
                return true;
        }
        else
        {
            if( GetPtr(iEntry) == pEntryData )
                return true;
        }
    }
    return false;
}

// cchDestChar는 strDest의 바이트 단위 크기입니다.  
// strDest가 문자열 포인터인 경우 sizeof() 사용을 전달합니다.  
// 예시) TCHAR* sz = 새로운 TCHAR[100]; 크기(sz) == 4
// TCHAR sz2[100]; 크기(sz2) == 200
HRESULT DXUtil_ConvertAnsiStringToWideCb( WCHAR* wstrDestination, const CHAR* strSource, int cbDestChar )
{
    return DXUtil_ConvertAnsiStringToWideCch( wstrDestination, strSource, cbDestChar / sizeof(WCHAR) );
}

HRESULT DXUtil_ConvertWideStringToAnsiCb( CHAR* strDestination, const WCHAR* wstrSource, int cbDestChar )
{
    return DXUtil_ConvertWideStringToAnsiCch( strDestination, wstrSource, cbDestChar / sizeof(CHAR) );
}

HRESULT DXUtil_ConvertGenericStringToAnsiCb( CHAR* strDestination, const TCHAR* tstrSource, int cbDestChar )
{
    return DXUtil_ConvertGenericStringToAnsiCch( strDestination, tstrSource, cbDestChar / sizeof(CHAR) );
}

HRESULT DXUtil_ConvertGenericStringToWideCb( WCHAR* wstrDestination, const TCHAR* tstrSource, int cbDestChar )
{
    return DXUtil_ConvertGenericStringToWideCch( wstrDestination, tstrSource, cbDestChar / sizeof(WCHAR) );
}

HRESULT DXUtil_ConvertAnsiStringToGenericCb( TCHAR* tstrDestination, const CHAR* strSource, int cbDestChar )
{
    return DXUtil_ConvertAnsiStringToGenericCch( tstrDestination, strSource, cbDestChar / sizeof(TCHAR) );
}

HRESULT DXUtil_ConvertWideStringToGenericCb( TCHAR* tstrDestination, const WCHAR* wstrSource, int cbDestChar )
{
    return DXUtil_ConvertWideStringToGenericCch( tstrDestination, wstrSource, cbDestChar / sizeof(TCHAR) );
}

HRESULT DXUtil_ReadStringRegKeyCb( HKEY hKey, TCHAR* strRegName, TCHAR* strDest, DWORD cbDest, TCHAR* strDefault )
{
    return DXUtil_ReadStringRegKeyCch( hKey, strRegName, strDest, cbDest / sizeof(TCHAR), strDefault );
}

HRESULT DXUtil_ConvertGUIDToStringCb( const GUID* pGuidSrc, TCHAR* strDest, int cbDestChar )
{
    return DXUtil_ConvertGUIDToStringCch( pGuidSrc, strDest, cbDestChar / sizeof(TCHAR) );
}

#ifndef UNDER_CE
HRESULT DXUtil_GetDXSDKMediaPathCb( TCHAR* szDest, int cbDest )
{
    return DXUtil_GetDXSDKMediaPathCch( szDest, cbDest / sizeof(TCHAR) );
}

HRESULT DXUtil_FindMediaFileCb( TCHAR* szDestPath, int cbDest, TCHAR* strFilename )
{
    return DXUtil_FindMediaFileCch( szDestPath, cbDest / sizeof(TCHAR), strFilename );
}
#endif // !UNDER_CE
