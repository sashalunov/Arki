#pragma once
#include <mutex> // REQUIRED for thread safety

// Optional: Log Levels for better filtering
enum eLogLevel {
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};
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
	void AttachConsole();

private:
	FILE*		m_pFile;
	TCHAR*		m_szFileName;
	// THE MAGIC: A mutex to protect the file pointer
	std::mutex  m_mutex;
};

extern CLogger *Log;
#define _log(...) Log->Log(__VA_ARGS__)
#define _print(...) Log->Print(__VA_ARGS__)
