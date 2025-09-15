#ifndef LOGGING_H
#define LOGGING_H

OSErr InitializeLogFile(void);
void LogToFile(const char* message);
void CloseLogFile(void);
void DualLogFunc(const char* message);
void DirectLogMessage(const char* message);

#endif