#pragma once


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
class CLogger
{
public:
	CLogger(void);
	~CLogger(void);

	BOOL Initialize(void);
	BOOL Shutdown(void);

	void Log(TCHAR *fmt, ...);		// ������� ��������� � ����
	void Print(TCHAR *fmt, ...);	// ������� ��������� � Output Window

private:
	FILE*		m_pFile;
	TCHAR*		m_szFileName;

};

extern CLogger *Log;
#define _log(...) Log->Log(__VA_ARGS__)
#define _print(...) Log->Print(__VA_ARGS__)
