#include "StdAfx.h"
#include "Logger.h"


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
	// Open file
	errno_t err = _wfopen_s(&m_pFile, m_szFileName, L"wt");
	if (err != 0 || m_pFile == NULL)
		return FALSE;
	

	// Greeting message
	Log(	L"Log file created ...\n");
	fputws( L"\n", m_pFile);

	return TRUE;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
BOOL CLogger::Shutdown(void)
{
	if(!m_pFile)
		return FALSE;

	// Closing message
	fputws( L"\n", m_pFile);
	Log(	L"Log file closed ...\n");

	// Close file
	fclose(m_pFile);

	return TRUE;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CLogger::Log(TCHAR *fmt, ...)
{
	if(!m_pFile)
		return;

	TCHAR szBuf[1024];
	TCHAR szVa[1024];
	TCHAR szTime[512];
	TCHAR szDate[512];

	_wstrtime_s(szTime);
	_wstrdate_s(szDate);

	va_list body;
	va_start(body, fmt);
	vswprintf_s(szVa, _countof(szVa), fmt, body); // secure variant
	va_end(body);

	swprintf_s(szBuf, L"(%s %s) %s", szDate, szTime, szVa);

	fputws(szBuf, m_pFile);

	fflush(m_pFile);
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
void CLogger::Print(TCHAR *fmt, ...)
{
	TCHAR szVa[1024];

	va_list body;
	va_start(body, fmt);
	vswprintf_s(szVa, _countof(szVa), fmt, body); // secure variant
	va_end(body);

	OutputDebugString(szVa);
}