#pragma once

#include "Common.h"
#include <cstdio>
#include <cstdarg>
#include "Logger.h"
#include <sstream>
#include <stdarg.h>

#include <ctime>
#include <iomanip>

#define UAB_BREAK

#define UAB_ASSERT_ALL


namespace MyBot
{
    namespace Assert
    {
        void ShutDown();

        extern std::string lastErrorMessage;

        const std::string currentDateTime();

        void ReportFailure(const char * condition, const char * file, int line, const char * msg, ...);
    }
}

#ifdef UAB_ASSERT_ALL
#define UAB_ASSERT(cond, msg, ...) \
do \
{ \
if (!(cond)) \
{ \
	MyBot::Assert::ReportFailure(#cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
	UAB_BREAK \
} \
} while (0)

#define UAB_ASSERT_WARNING(cond, msg, ...) \
do \
{ \
if (!(cond)) \
{ \
	MyBot::Assert::ReportFailure(#cond, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
} \
} while (0)
#else
#define UAB_ASSERT(cond, msg, ...) 
#endif