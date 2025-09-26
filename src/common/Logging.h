#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H

#include <Types.h>

/* Logging function pointer type for networking components */
typedef void (*LoggingCallback)(const char* message);

/* Core logging functions that can be used by networking libraries */
OSErr InitializeLogFile(void);
void LogToFile(const char* message);
void CloseLogFile(void);
void DualLogFunc(const char* message);
void DirectLogMessage(const char* message);

#endif