// GESetup.cpp
// Simple DLL project for use with NSIS
// Allows you to register a game with Vista Games Explorer via the SetupGameExplorerRequirements function

#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include <stdio.h>
#include <gameux.h>
#include <shlobj.h>
#include <shellapi.h>
#include <objbase.h>
#include <windows.h>
#include <strsafe.h>
#include <wbemidl.h>
#include "..\..\Contrib\ExDLL\exdll.h"

#define SAVE_EXTENTION L".dsg"
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

void StringToGuid(GUID* guid, const char* buffer) {
	int a, b, c, d,
		e, f, g, h;

	sscanf(buffer, "{%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
		&guid->Data1, &guid->Data2, &guid->Data3, 
		&a, &b, &c, &d, &e, &f, &g, &h);

	guid->Data4[0] = a;
	guid->Data4[1] = b;
	guid->Data4[2] = c;
	guid->Data4[3] = d;
	guid->Data4[4] = e;
	guid->Data4[5] = f;
	guid->Data4[6] = g;
	guid->Data4[7] = h;
}

STDAPI RetrieveGUIDForApplication( WCHAR* szPathToGDFdll, GUID* pGUID )
{
    HRESULT hr;
    IWbemLocator*  pIWbemLocator = NULL;
    IWbemServices* pIWbemServices = NULL;
    BSTR           pNamespace = NULL;
    IEnumWbemClassObject* pEnum = NULL;
    bool bFound = false;

    CoInitialize(0);

    hr = CoCreateInstance( __uuidof(WbemLocator), NULL, CLSCTX_INPROC_SERVER, 
                           __uuidof(IWbemLocator), (LPVOID*) &pIWbemLocator );
    if( SUCCEEDED(hr) && pIWbemLocator )
    {
        // Using the locator, connect to WMI in the given namespace.
        pNamespace = SysAllocString( L"\\\\.\\root\\cimv2\\Applications\\Games" );

        hr = pIWbemLocator->ConnectServer( pNamespace, NULL, NULL, 0L,
                                        0L, NULL, NULL, &pIWbemServices );
        if( SUCCEEDED(hr) && pIWbemServices != NULL )
        {
            // Switch security level to IMPERSONATE. 
            CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
                               RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );

            BSTR bstrQueryType = SysAllocString( L"WQL" );

            // Double up the '\' marks for the WQL query
            WCHAR szDoubleSlash[2048];
            int iDest = 0, iSource = 0;
            for( ;; )
            {
                if( szPathToGDFdll[iSource] == 0 || iDest > 2000 )
                    break;
                szDoubleSlash[iDest] = szPathToGDFdll[iSource];
                if( szPathToGDFdll[iSource] == L'\\' )
                {
                    iDest++; szDoubleSlash[iDest] = L'\\';
                }
                iDest++;
                iSource++;
            }
            szDoubleSlash[iDest] = 0;

            WCHAR szQuery[1024];
            StringCchPrintf( szQuery, 1024, L"SELECT * FROM GAME WHERE GDFBinaryPath = \"%s\"", szDoubleSlash );
            BSTR bstrQuery = SysAllocString( szQuery );

            hr = pIWbemServices->ExecQuery( bstrQueryType, bstrQuery, 
                                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pEnum );
            if( SUCCEEDED(hr) )
            {
                IWbemClassObject* pGameClass = NULL;
                DWORD uReturned = 0;
                BSTR pPropName = NULL;

                // Get the first one in the list
                hr = pEnum->Next( 5000, 1, &pGameClass, &uReturned );
                if( SUCCEEDED(hr) && uReturned != 0 && pGameClass != NULL )
                {
                    VARIANT var;

                    // Get the InstanceID string
                    pPropName = SysAllocString( L"InstanceID" );
                    hr = pGameClass->Get( pPropName, 0L, &var, NULL, NULL );
                    if( SUCCEEDED(hr) && var.vt == VT_BSTR )
                    {
                        bFound = true;
                        if( pGUID ) StringToGuid( pGUID, (const char *)var.cVal );
                    }

                    if( pPropName ) SysFreeString( pPropName );
                }

                SAFE_RELEASE( pGameClass );
            }

            SAFE_RELEASE( pEnum );
        }

        if( pNamespace ) SysFreeString( pNamespace );
        SAFE_RELEASE( pIWbemServices );
    }

    SAFE_RELEASE( pIWbemLocator );

#ifdef SHOW_DEBUG_MSGBOXES
    WCHAR sz[1024];
    StringCchPrintf( sz, 1024, L"szPathToGDFdll=%s bFound=%d", szPathToGDFdll, bFound );
    MessageBox( NULL, sz, L"RetrieveGUIDForApplicationW", MB_OK );
#endif

    return (bFound) ? S_OK : E_FAIL;
}


HRESULT SetupRichSavedGamesW( WCHAR* strSavedGameExtension, WCHAR* strLaunchPath, 
                             WCHAR* strCommandLineToLaunchSavedGame ) 
{
    HKEY hKey = NULL;
    LONG lResult;
    DWORD dwDisposition;
    WCHAR strExt[256];
    WCHAR strType[256];
    WCHAR strCmdLine[256];
    WCHAR strTemp[512];
    size_t nStrLength = 0;

    // Validate args 
    if( strLaunchPath == NULL || strSavedGameExtension == NULL )
    {
        return E_INVALIDARG;
    }
    
    // Setup saved game extension arg - make sure there's a period at the start
    if( strSavedGameExtension[0] == L'.' )
    {
        StringCchCopy( strExt, 256, strSavedGameExtension );
        StringCchPrintf( strType, 256, L"%sType", strSavedGameExtension+1 );
    }
    else
    {
        StringCchPrintf( strExt, 256, L".%s", strSavedGameExtension );
        StringCchPrintf( strType, 256, L"%sType", strSavedGameExtension );
    }

    // Create default command line arg if none supplied
    if( strCommandLineToLaunchSavedGame )
        StringCchCopy( strCmdLine, 256, strCommandLineToLaunchSavedGame );
    else
        StringCchCopy( strCmdLine, 256, L"\"%1\"" );

    // Create file association & metadata regkeys
    lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, strExt, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition );
    if( ERROR_SUCCESS == lResult ) 
    {
        // Create the following regkeys:
        //
        // [HKEY_CLASSES_ROOT\.ExampleGameSave]
        // (Default)="ExampleGameSaveFileType"
        //
        StringCchLength( strType, 256, &nStrLength );
        RegSetValueEx( hKey, L"", 0, REG_SZ, (BYTE*)strType, (DWORD)((nStrLength + 1)*sizeof(WCHAR)) );

        // Create the following regkeys:
        //
        // [HKEY_CLASSES_ROOT\.ExampleGameSave\ShellEx\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}]
        // (Default)="{4E5BFBF8-F59A-4e87-9805-1F9B42CC254A}"
        //
        HKEY hSubKey = NULL;
        lResult = RegCreateKeyEx( hKey, L"ShellEx\\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, &dwDisposition );
        if( ERROR_SUCCESS == lResult ) 
        {
            StringCchPrintf( strTemp, 512, L"{4E5BFBF8-F59A-4e87-9805-1F9B42CC254A}" );
            StringCchLength( strTemp, 256, &nStrLength );
            RegSetValueEx( hSubKey, L"", 0, REG_SZ, (BYTE*)strTemp, (DWORD)((nStrLength + 1)*sizeof(WCHAR)) );
        }
        if( hSubKey ) RegCloseKey( hSubKey );
    }
    if( hKey ) RegCloseKey( hKey );

    lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT, strType, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition );
    if( ERROR_SUCCESS == lResult ) 
    {
        // Create the following regkeys:
        //
        // [HKEY_CLASSES_ROOT\ExampleGameSaveFileType]
        // PreviewTitle="prop:System.Game.RichSaveName;System.Game.RichApplicationName"
        // PreviewDetails="prop:System.Game.RichLevel;System.DateChanged;System.Game.RichComment;System.DisplayName;System.DisplayType"
        //
        size_t nPreviewDetails = 0, nPreviewTitle = 0;
        WCHAR* strPreviewTitle = L"prop:System.Game.RichSaveName;System.Game.RichApplicationName";
        WCHAR* strPreviewDetails = L"prop:System.Game.RichLevel;System.DateChanged;System.Game.RichComment;System.ItemNameDisplay;System.ItemType";
        StringCchLength( strPreviewTitle, 256, &nPreviewTitle );
        StringCchLength( strPreviewDetails, 256, &nPreviewDetails );
        RegSetValueEx( hKey, L"PreviewTitle", 0, REG_SZ, (BYTE*)strPreviewTitle, (DWORD)((nPreviewTitle + 1)*sizeof(WCHAR)) );
        RegSetValueEx( hKey, L"PreviewDetails", 0, REG_SZ, (BYTE*)strPreviewDetails, (DWORD)((nPreviewDetails + 1)*sizeof(WCHAR)) );

        // Create the following regkeys:
        //
        // [HKEY_CLASSES_ROOT\ExampleGameSaveFileType\Shell\Open\Command]
        // (Default)=""%ProgramFiles%\ExampleGame.exe" "%1""
        //
        HKEY hSubKey = NULL;
        lResult = RegCreateKeyEx( hKey, L"Shell\\Open\\Command", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, &dwDisposition );
        if( ERROR_SUCCESS == lResult ) 
        {
            StringCchPrintf( strTemp, 512, L"%s %s", strLaunchPath, strCmdLine );
            StringCchLength( strTemp, 256, &nStrLength );
            RegSetValueEx( hSubKey, L"", 0, REG_SZ, (BYTE*)strTemp, (DWORD)((nStrLength + 1)*sizeof(WCHAR)) );
        }
        if( hSubKey ) RegCloseKey( hSubKey );
    }
    if( hKey ) RegCloseKey( hKey );

    // Create the following regkeys:
    //
    // [HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.ExampleGameSave]
    // (Default)="{ECDD6472-2B9B-4b4b-AE36-F316DF3C8D60}"
    //
    StringCchPrintf( strTemp, 512, L"Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s", strExt );
    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, strTemp, 0, NULL, 0, KEY_WRITE, NULL, &hKey, &dwDisposition );
    if( ERROR_SUCCESS == lResult ) 
    {
        StringCchCopy( strTemp, 512, L"{ECDD6472-2B9B-4B4B-AE36-F316DF3C8D60}" );
        StringCchLength( strTemp, 256, &nStrLength );
        RegSetValueEx( hKey, L"", 0, REG_SZ, (BYTE*)strTemp, (DWORD)((nStrLength + 1)*sizeof(WCHAR)) );
    }
    if( hKey ) RegCloseKey( hKey );

    return S_OK;
}

//-----------------------------------------------------------------------------
// Removes the registry keys to enable rich saved games.  
//
// strSavedGameExtension should begin with a period. ex: .ExampleSaveGame
//-----------------------------------------------------------------------------
HRESULT RemoveRichSavedGamesW( WCHAR* strSavedGameExtension ) 
{
    WCHAR strExt[256];
    WCHAR strType[256];
    WCHAR strTemp[512];

    // Validate args 
    if( strSavedGameExtension == NULL )
    {
        return E_INVALIDARG;
    }
    
    // Setup saved game extension arg - make sure there's a period at the start
    if( strSavedGameExtension[0] == L'.' )
    {
        StringCchCopy( strExt, 256, strSavedGameExtension );
        StringCchPrintf( strType, 256, L"%sType", strSavedGameExtension+1 );
    }
    else
    {
        StringCchPrintf( strExt, 256, L".%s", strSavedGameExtension );
        StringCchPrintf( strType, 256, L"%sType", strSavedGameExtension );
    }

    // Delete the following regkeys:
    //
    // [HKEY_CLASSES_ROOT\.ExampleGameSave]
    // [HKEY_CLASSES_ROOT\ExampleGameSaveFileType]
    // [HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.ExampleGameSave]
    RegDeleteKey( HKEY_CLASSES_ROOT, strExt );
    RegDeleteKey( HKEY_CLASSES_ROOT, strType );
    StringCchPrintf( strTemp, 512, L"Software\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s", strExt );
    RegDeleteKey( HKEY_LOCAL_MACHINE, strTemp );

    return S_OK;
}

HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR _fileName, LPCSTR lpszDesc, LPCSTR args = NULL) 
{ 
    HRESULT hres; 
    IShellLink* psl; 
 
    // Get a pointer to the IShellLink interface. 
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLink, (LPVOID*)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
 
        // Set the path to the shortcut target and add the description. 

		char exePath[1024];
		strcpy( exePath, lpszPathObj );
		char *fileName = strrchr( exePath, '\\' ) + 1;
		exePath[ strlen(exePath) - strlen(fileName) ] = '\0';

		WCHAR objectPath[1024];
		WCHAR objectDir[1024];
		MultiByteToWideChar(CP_ACP, 0, lpszPathObj, -1, objectPath, MAX_PATH); 
		MultiByteToWideChar(CP_ACP, 0, exePath, -1, objectDir, MAX_PATH); 

        psl->SetPath(objectPath); 
        psl->SetDescription((LPCWSTR)lpszDesc);
		psl->SetWorkingDirectory(objectDir);
		if( args != NULL )
		{
			WCHAR wArgs[256];
			MultiByteToWideChar(CP_ACP, 0,args, -1, wArgs, MAX_PATH); 
			psl->SetArguments( wArgs );
		}
 
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
 
        if (SUCCEEDED(hres)) 
        { 
            WCHAR wsz[MAX_PATH]; 
			WCHAR fullFilePath[MAX_PATH];
 
            // Ensure that the string is Unicode. 
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH); 
			MultiByteToWideChar(CP_ACP, 0, _fileName, -1, fullFilePath, MAX_PATH); 
			
            // Add code here to check return value from MultiByteWideChar 
            // for success.
 
            // Save the link by calling IPersistFile::Save. 
			SHCreateDirectoryEx(NULL, wsz, NULL );
			hres = ppf->Save(fullFilePath, TRUE); 
            ppf->Release(); 
        }
		else
		{
            MessageBox(NULL, L"Failed to create shortcut", L"Darwinia: Vista Edition", MB_OK );
		}
        psl->Release(); 
    } 
	else
	{
		MessageBox(NULL, L"Failed to create shortcut", L"Darwinia: Vista Edition", MB_OK );
	}
    return hres; 
}

extern "C" void __declspec(dllexport) IsUserAdmin( HWND hwndParent,
												   int string_size,
												   char *variables,
												   stack_t **stacktop)
{
    EXDLL_INIT();
    BOOL b = false;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup; 
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup); 
    if(b) 
    {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
        {
             setuservariable( INST_0, "0" );
        } 
        else
        {
            setuservariable( INST_0, "1" );
        }
        FreeSid(AdministratorsGroup); 
    }
}

extern "C" void __declspec(dllexport) CheckForVista( HWND hwndParent,
													 int string_size,
													 char *variables,
													 stack_t **stacktop)
{
    EXDLL_INIT();
	OSVERSIONINFOEX versionInfo;
	ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOEX));

	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx( (OSVERSIONINFO *) &versionInfo );

	if ( versionInfo.dwMajorVersion < 6 || 
		 versionInfo.wProductType != VER_NT_WORKSTATION )
	{
		setuservariable( INST_0, "0");
		return;
	}

	setuservariable( INST_0, "1");
}

extern "C" void __declspec(dllexport) SetupGameExplorerRequirements( HWND hwndParent,
																	int string_size,
																	char *variables,
																	stack_t **stacktop)
{
	EXDLL_INIT();

	char *cBinPath = NULL, 
		 *cInstallDir = NULL, 
		 *allUsers = NULL;

	cBinPath = (char*)GlobalAlloc(GPTR, string_size);
	cInstallDir = (char*)GlobalAlloc(GPTR, string_size);
	allUsers = (char*)GlobalAlloc(GPTR, string_size);

	popstring( cBinPath );
	popstring( cInstallDir );
	popstring( allUsers );

	WCHAR wBinPath[512],
		  wInstallDir[512];

	mbstowcs(wBinPath, cBinPath, 512 );
	mbstowcs(wInstallDir, cInstallDir, 512 );

	HRESULT hr = CoInitialize(NULL);
	if( FAILED(hr) )
	{
        MessageBox( hwndParent, L"Failed to register Darwinia: Vista Edition with Games Explorer", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		setuservariable( INST_0, "0" );
		return;
	}

	IGameExplorer *gameExplorerInterface = NULL;
	hr = CoCreateInstance(__uuidof(GameExplorer), 0, CLSCTX_INPROC_SERVER, 
                    __uuidof(IGameExplorer), (LPVOID *)&gameExplorerInterface);

	if( FAILED(hr) )
	{
        MessageBox( hwndParent, L"Failed to register Darwinia: Vista Edition with Games Explorer", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		setuservariable( INST_0, "0" );
		return;
	}

	GAME_INSTALL_SCOPE installScope = GIS_CURRENT_USER;
	if( strcmp( allUsers, "1" ) == 0 )
	{
		installScope = GIS_ALL_USERS;
	}

	GUID guid = GUID_NULL;


	// Remove the Game Explorer
	/*StringToGuid( &guid, "{88FE1129-ED85-4674-B245-89AB7B9C267B}");
	gameExplorerInterface->RemoveGame( guid );	
	setuservariable( INST_0, "0" );
	return;*/

    hr = RetrieveGUIDForApplication( wBinPath, NULL );
    if( SUCCEEDED(hr) )
    {
        MessageBox( hwndParent, L"Darwinia: Vista Edition is already registered with Games Explorer", L"Darwinia: Vista Edition", MB_OK|MB_ICONEXCLAMATION);
        setuservariable( INST_0, "0" );
        return;
    }

	hr = gameExplorerInterface->AddGame( wBinPath, wInstallDir, installScope, &guid );
	if( FAILED(hr) )
	{
        MessageBox( hwndParent, L"Failed to register Darwinia: Vista Edition with Games Explorer", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		setuservariable( INST_0, "0" );
		return;
	}

	WCHAR *gameExplorerPath = (WCHAR*)GlobalAlloc(GPTR, string_size);
	if( strcmp( allUsers, "1" ) == 0  )
	{
		hr = SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, gameExplorerPath );
	}
	else
	{
		hr = SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, gameExplorerPath );
	}

	if( FAILED(hr) )
	{
		gameExplorerInterface->RemoveGame(guid);
        MessageBox( hwndParent, L"Failed to register Darwinia: Vista Edition with Games Explorer", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		setuservariable( INST_0, "0" );
		return;
	}

	WCHAR guidString[128];
	hr = StringFromGUID2( guid, guidString, 128 );

	char cExplorerPath[1024];
	char cGuid[128];

	wcstombs( cExplorerPath, gameExplorerPath, 1024 );
	wcstombs( cGuid, guidString, 128 );

	char fullPath[1024];
	sprintf( fullPath, "%s\\Microsoft\\Windows\\GameExplorer\\%s\\PlayTasks\\0", cExplorerPath, cGuid );

	char fileName[1024];
	sprintf( fileName, "%s\\Play.lnk", fullPath );

	CreateLink( cBinPath, fullPath, fileName, NULL );

	//gameExplorerInterface->RemoveGame(guid);
	//pushstring(cGuid);
    SetupRichSavedGamesW( SAVE_EXTENTION, wBinPath, NULL );
	setuservariable( INST_0, cGuid );
}

extern "C" void __declspec(dllexport) RemoveFromGameExplorer( HWND hwndParent,
															  int string_size,
															  char *variables,
															  stack_t **stacktop)
{
	EXDLL_INIT();

	char *cGuid = NULL;
	cGuid = (char*)GlobalAlloc(GPTR, string_size);
	popstring( cGuid );

	HRESULT hr = CoInitialize(NULL);
	if( FAILED(hr) )
	{
        MessageBox( hwndParent, L"Failed to remove Darwinia: Vista Edition from Games Explorer\nCoInitialize Failed", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		return;
	}

	IGameExplorer *gameExplorerInterface = NULL;
	hr = CoCreateInstance(__uuidof(GameExplorer), 0, CLSCTX_INPROC_SERVER, 
                    __uuidof(IGameExplorer), (LPVOID *)&gameExplorerInterface);

	if( FAILED(hr) )
	{
		MessageBox( hwndParent, L"Failed to remove Darwinia: Vista Edition from Games Explorer\nFailed to create IGameExplorer Instance", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
		return;
	}

	GUID guid = GUID_NULL;
	StringToGuid( &guid, cGuid );
    //CoCreateGuid( &guid );

	hr = gameExplorerInterface->RemoveGame( guid );
	if( FAILED( hr ) )
	{
		MessageBox( hwndParent, L"Failed to remove Darwinia: Vista Edition from Games Explorer\nRemoveGame Failed", L"Darwinia: Vista Edition", MB_OK|MB_ICONERROR );
	}

    RemoveRichSavedGamesW(SAVE_EXTENTION);

	return;
}

extern "C" void __declspec(dllexport) CreateMediaCenterShortcut( HWND hwndParent,
																  int string_size,
																  char *variables,
																  stack_t **stacktop)
{
	EXDLL_INIT();

	char *cBinPath = NULL;
	cBinPath = (char*)GlobalAlloc(GPTR, string_size);
	popstring( cBinPath );

	char shortcutLink[MAX_PATH];
	sprintf( shortcutLink, "\"%s\" -media", cBinPath );

	HRESULT hr = CoInitialize(NULL);
	char *mediaCenterPath = (char*)GlobalAlloc(GPTR, string_size);
	hr = SHGetFolderPathA( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, mediaCenterPath );

	char shortcutPath[MAX_PATH];
	char filename[MAX_PATH];
	sprintf( shortcutPath, "%s\\Media Center Programs\\Darwinia", mediaCenterPath );
	sprintf( filename, "%s\\darwinia.lnk", shortcutPath );

	CreateLink( cBinPath, shortcutPath, filename, "", "-mediacenter" );
}

extern "C" void __declspec(dllexport) GetMediaCenterPath( HWND hwndParent,
														  int string_size,
														  char *variables,
														  stack_t **stacktop)
{
	EXDLL_INIT();

	HRESULT hr = CoInitialize(NULL);
	char *mediaCenterPath = (char*)GlobalAlloc(GPTR, string_size);
	hr = SHGetFolderPathA( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, mediaCenterPath );
	setuservariable( INST_0, mediaCenterPath );
}