#include "StdAfx.h"
#include "Logger.h"
#include <iostream> 


static CLogger g_logger;
CLogger *Log = (CLogger*)&g_logger;

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CLogger::CLogger(void)
{
	m_pFile = NULL;
	m_szFileName = L"ARKI.log";

}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
CLogger::~CLogger(void)
{
	m_pFile = NULL;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
BOOL CLogger::Initialize(void)
{
	// THREAD SAFETY: Lock mutex during initialization
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_pFile) return TRUE; // Already open
	// Open file
	// FIX 1: USE _wfsopen TO ALLOW SHARED READING
	// "wt, ccs=UTF-8" -> Write Text, Force UTF-8 encoding (better for international chars)
	// _SH_DENYNO     -> Deny None (Allows other apps to read the file while we write)
	m_pFile = _wfsopen(m_szFileName, L"wt, ccs=UTF-8", _SH_DENYNO);
	if( m_pFile == NULL)
		return FALSE;
	
	// Greeting message
	fputws(L"------------------------------------------\n", m_pFile);
	fputws(	L"Log file initialized (Thread-Safe)\n", m_pFile);
	fputws(L"------------------------------------------\n", m_pFile);
	fflush(m_pFile);

	return TRUE;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
BOOL CLogger::Shutdown(void)
{
	// THREAD SAFETY: Lock mutex so we don't close the file while another thread is writing
	std::lock_guard<std::mutex> lock(m_mutex);

	if(!m_pFile)return FALSE;

	// Closing message
	fputws( L"\n", m_pFile);
	fputws( L"Log file closed ...\n", m_pFile);
	// Close file
	fclose(m_pFile);
	m_pFile = NULL;
	return TRUE;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CLogger::Log(TCHAR *fmt, ...)
{
	eLogLevel level = LOG_INFO;
	if(!m_pFile)return;
	// 1. Prepare buffers on stack (Thread local storage, safe)
	//wchar_t szBuf[2048];
	wchar_t szVa[2048];
	wchar_t szTime[64];
	wchar_t szDate[64];

	_wstrtime_s(szTime);
	_wstrdate_s(szDate);

	va_list body;
	va_start(body, fmt);
	_vsnwprintf_s(szVa, _countof(szVa), _TRUNCATE, fmt, body); // secure variant
	va_end(body);

	// 3. Determine Tag based on Level
	const wchar_t* szTag = L"INFO";
	if (level == LOG_WARNING) szTag = L"WARN";
	else if (level == LOG_ERROR) szTag = L"ERROR";

	// 4. CRITICAL: Lock access to the file
	// The lock is released automatically when this function ends
	std::lock_guard<std::mutex> lock(m_mutex);

	// 5. Write formatted string: [DATE TIME] [LEVEL] Message
	fwprintf(m_pFile, L"[%s %s] [%s] %s", szDate, szTime, szTag, szVa);


	//swprintf_s(szBuf, L"(%s %s) %s", szDate, szTime, szVa);

	//fputws(szBuf, m_pFile);

	// Also print to Visual Studio Output window for convenience
	// (Combining Print and Log is often a good idea)
	wchar_t szDebug[2048];
	swprintf_s(szDebug, L"[%s] %s\n", szTag, szVa);
	OutputDebugStringW(szDebug);

	fflush(m_pFile);
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CLogger::Print(TCHAR *fmt, ...)
{
	wchar_t szVa[2048];

	va_list body;
	va_start(body, fmt);
	_vsnwprintf_s(szVa, _countof(szVa), _TRUNCATE, fmt, body); // secure variant
	va_end(body);

	OutputDebugString(szVa);
}

void CLogger::AttachConsole()
{
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	std::wcout.clear();
}