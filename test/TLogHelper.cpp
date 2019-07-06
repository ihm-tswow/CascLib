/*****************************************************************************/
/* TLogHelper.cpp                         Copyright (c) Ladislav Zezula 2013 */
/*---------------------------------------------------------------------------*/
/* Helper class for reporting StormLib tests                                 */
/* This file should be included directly from StormTest.cpp using #include   */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 26.11.13  1.00  Lad  The first version of TLogHelper.cpp                  */
/*****************************************************************************/

//-----------------------------------------------------------------------------
// String replacements for format strings

#ifdef _MSC_VER
#define fmt_I64u "%I64u"
#define fmt_I64X "%I64X"
#else
#define fmt_I64u "%llu"
#define fmt_I64X "%llX"
#endif

//-----------------------------------------------------------------------------
// Definition of the TLogHelper class

class TLogHelper
{
    public:

    //
    //  Constructor and destructor
    //

    TLogHelper(const char * szNewMainTitle = NULL, const char * /* szNewSubTitle */ = NULL)
    {
        // Fill the variables
        memset(this, 0, sizeof(TLogHelper));

        // Remember the startup time
        StartTime = GetCurrentThreadTime();
        TotalFiles = 1;

        // Print the initial information
        if(szNewMainTitle != NULL)
        {
            char szTitle[101];
            size_t nLength;

            nLength = CascStrPrintf(szTitle, _countof(szTitle), "-- \"%s\" --", szNewMainTitle);
            while(nLength < 90)
                szTitle[nLength++] = '-';
            if(nLength < sizeof(szTitle))
                szTitle[nLength++] = 0;

            printf("%s\n", szTitle);
        }
    }

    ~TLogHelper()
    {
        const char * szSaveMainTitle = szMainTitle;
        const char * szSaveSubTitle = szSubTitle;

        // Set both to NULL so the won't be printed
        szMainTitle = NULL;
        szSubTitle = NULL;

        // Print the final information
        if(szSaveMainTitle != NULL && bMessagePrinted == false)
        {
            if(DontPrintResult == false)
            {
                if(szSaveSubTitle != NULL)
                    PrintMessage("%s (%s) succeeded.", szSaveMainTitle, szSaveSubTitle);
                else
                    PrintMessage("%s succeeded.", szSaveMainTitle);
            }
            else
            {
                PrintProgress(" ");
                printf("\r");
            }
        }

        printf("\n");
    }

    //
    //  Printing functions
    //

    template <typename XCHAR>
    DWORD PrintWithClreol(const XCHAR * szFormat, va_list argList, bool bPrintPrefix, bool bPrintLastError, bool bPrintEndOfLine)
    {
        char * szBufferPtr;
        char * szBufferEnd;
        char szMessage[0x200];
        size_t nRemainingWidth;
        size_t nConsoleWidth = GetConsoleWidth();
        size_t nLength = 0;
        DWORD dwErrCode = GetLastError();

        // Always start the buffer with '\r'
        szBufferEnd = szMessage + sizeof(szMessage);
        szBufferPtr = szMessage;
        *szBufferPtr++ = '\r';

        // Print the prefix, if needed
        if(szMainTitle != NULL && bPrintPrefix)
        {
            while(szMainTitle[nLength] != 0)
                *szBufferPtr++ = szMainTitle[nLength++];
            
            *szBufferPtr++ = ':';
            *szBufferPtr++ = ' ';
        }

        // Construct the message
        while(szFormat[0] != 0)
        {
            // Sanity check
            assert(szBufferPtr < szBufferEnd);

            // Is there a format character?
            if(szFormat[0] == '%')
            {
                if(szFormat[1] == '%')
                {
                    *szBufferPtr++ = '%';
                    szFormat += 2;
                    continue;
                }

                // String argument
                if(IsFormatSpecifier(szFormat, "%s"))
                {
                    const XCHAR * szArg = va_arg(argList, XCHAR *);
                    szBufferPtr = StringCopy(szBufferPtr, szArg);
                    szFormat += 2;
                    continue;
                }

                // 64-bit integer argument
                if(IsFormatSpecifier(szFormat, "%llu"))
                {
                    szBufferPtr += CascStrPrintf(szBufferPtr, (szBufferEnd - szBufferPtr), fmt_I64u, va_arg(argList, ULONGLONG));
                    szFormat += 4;
                    continue;
                }

                // 32-bit integer argument
                if(IsFormatSpecifier(szFormat, "%u"))
                {
                    szBufferPtr += CascStrPrintf(szBufferPtr, (szBufferEnd - szBufferPtr), "%u", va_arg(argList, DWORD));
                    szFormat += 2;
                    continue;
                }

                // 32-bit integer argument
                if (IsFormatSpecifier(szFormat, "%08X"))
                {
                    szBufferPtr += CascStrPrintf(szBufferPtr, (szBufferEnd - szBufferPtr), "%08X", va_arg(argList, DWORD));
                    szFormat += 4;
                    continue;
                }

                // Unknown format specifier
                assert(false);
            }
            else
            {
                *szBufferPtr++ = *szFormat++;
            }
        }

        // Append the last error
        if(bPrintLastError)
        {
            nLength = CascStrPrintf(szBufferPtr, (szBufferEnd - szBufferPtr), " (error code: %u)", dwErrCode);
            szBufferPtr += nLength;
        }

        // Shall we pad the string?
        nLength = szBufferPtr - szMessage;
        if(nLength < nConsoleWidth)
        {
            // Pad the string with spaces to fill it up to the end of the line
            nRemainingWidth = nConsoleWidth - nLength - 1;
            memset(szBufferPtr, 0x20, nRemainingWidth);
            szBufferPtr += nRemainingWidth;
        }

        // Put the newline, if requested
        *szBufferPtr++ = bPrintEndOfLine ? '\n' : 0;
        *szBufferPtr = 0;

        // Remember if we printed a message
        if(bPrintEndOfLine)
            bMessagePrinted = true;

        // Spit out the text in one single printf
        printf("%s", szMessage);
        nMessageCounter++;
        return dwErrCode;
    }

    template <typename XCHAR>
    void PrintProgress(const XCHAR * szFormat, ...)
    {
        va_list argList;

        va_start(argList, szFormat);
        PrintWithClreol(szFormat, argList, true, false, false);
        va_end(argList);
    }

    template <typename XCHAR>
    void PrintMessage(const XCHAR * szFormat, ...)
    {
        va_list argList;

        va_start(argList, szFormat);
        PrintWithClreol(szFormat, argList, true, false, true);
        va_end(argList);
    }

    void PrintTotalTime()
    {
        DWORD TotalTime = SetEndTime();

        if(TotalTime != 0)
            PrintMessage("TotalTime: %u.%u second(s)", (TotalTime / 1000), (TotalTime % 1000));
        PrintMessage("Work complete.");
    }

    template <typename XCHAR>
    int PrintErrorVa(const XCHAR * szFormat, ...)
    {
        va_list argList;
        int nResult;

        va_start(argList, szFormat);
        nResult = PrintWithClreol(szFormat, argList, true, true, true);
        va_end(argList);

        return nResult;
    }

    template <typename XCHAR>
    int PrintError(const XCHAR * szFormat, const XCHAR * szFileName)
    {
        return PrintErrorVa(szFormat, szFileName);
    }

    //
    //  Time functions
    //

    ULONGLONG GetCurrentThreadTime()
    {
#ifdef _WIN32
        ULONGLONG KernelTime = 0;
        ULONGLONG UserTime = 0;
        ULONGLONG TempTime = 0;

        GetThreadTimes(GetCurrentThread(), (LPFILETIME)&TempTime, (LPFILETIME)&TempTime, (LPFILETIME)&KernelTime, (LPFILETIME)&UserTime);
        return ((KernelTime + UserTime) / 10 / 1000);
#else
        return time(NULL) * 1000;
#endif
    }

    bool ProgressCooldown()
    {
        DWORD dwTickCount = GetTickCount();
        bool bResult = true;

        if(dwTickCount > (dwPrevTickCount + 100))
        {
            dwPrevTickCount = dwTickCount;
            bResult = false;
        }

        return bResult;
    }

    DWORD SetEndTime()
    {
        EndTime = GetCurrentThreadTime();
        return (DWORD)(EndTime - StartTime);
    }

    void FormatTotalBytes(char * szBuffer, size_t ccBuffer)
    {
        ULONGLONG Bytes = TotalBytes;
        ULONGLONG Divider = 1000000000;
        char * szBufferEnd = szBuffer + ccBuffer;
        bool bDividingOn = false;

        while((szBuffer + 4) < szBufferEnd && Divider > 0)
        {
            // Are we already dividing?
            if(bDividingOn)
            {
                szBuffer += CascStrPrintf(szBuffer, ccBuffer, " %03u", (DWORD)(Bytes / Divider));
                Bytes = Bytes % Divider;
            }
            else if(Bytes > Divider)
            {
                szBuffer += CascStrPrintf(szBuffer, ccBuffer, "%u", (DWORD)(Bytes / Divider));
                Bytes = Bytes % Divider;
                bDividingOn = true;
            }
            Divider /= 1000;
        }
    }

    //
    //  Hashing functions
    //

    void InitHashers()
    {
        MD5_Init(&MD5State_Name);
        MD5_Init(&MD5State_Data);
        HasherReady = true;
    }

    void HashName(const char * name)
    {
        if(HasherReady)
        {
            MD5_Update(&MD5State_Name, name, (unsigned long)(strlen(name) + 1));
        }
    }

    void HashData(const unsigned char * data, size_t length)
    {
        if(HasherReady)
        {
            MD5_Update(&MD5State_Data, data, (unsigned long)length);
        }
    }

    const char * GetNameHash()
    {
        // If we are in the hashing process, we get the hash and convert to string
        if(HasherReady)
        {
            unsigned char md5_binary[MD5_HASH_SIZE];

            MD5_Final(md5_binary, &MD5State_Name);
            StringFromBinary(md5_binary, MD5_HASH_SIZE, szHashString_Name);
            return szHashString_Name;
        }
        else
        {
            return NULL;
        }
    }

    const char * GetDataHash()
    {
        // If we are in the hashing process, we get the hash and convert to string
        if(HasherReady)
        {
            unsigned char md5_binary[MD5_HASH_SIZE];

            MD5_Final(md5_binary, &MD5State_Data);
            StringFromBinary(md5_binary, MD5_HASH_SIZE, szHashString_Data);
            return szHashString_Data;
        }
        else
        {
            return NULL;
        }
    }

    ULONGLONG TotalBytes;                           // For user's convenience: Total number of bytes
    ULONGLONG ByteCount;                            // For user's convenience: Current number of bytes
    ULONGLONG StartTime;                            // Start time of an operation, in milliseconds
    ULONGLONG EndTime;                              // End time of an operation, in milliseconds
    MD5_CTX MD5State_Name;                          // For user's convenience: Md5 state of the file name hasher
    MD5_CTX MD5State_Data;                          // For user's convenience: Md5 state of the file data hasher
    DWORD TotalFiles;                               // For user's convenience: Total number of files
    DWORD FileCount;                                // For user's convenience: Curernt number of files
    char  szHashString_Name[MD5_STRING_SIZE+1];     // Final hash of the file names
    char  szHashString_Data[MD5_STRING_SIZE+1];     // Final hash of the file data
    DWORD DontPrintResult:1;                        // If true, supresset pringing result from the destructor
    DWORD HasherReady:1;                            // If true, supresset pringing result from the destructor

    protected:

    template <typename XCHAR>
    bool IsFormatSpecifier(const XCHAR * szFormat, const char * szSpecifier)
    {
        for(size_t i = 0; szSpecifier[i] != 0; i++)
        {
            if(szFormat[i] != szSpecifier[i])
                return false;
        }
        return true;
    }

    template <typename XCHAR>
    char * StringCopy(char * szBuffer, const XCHAR * szSource)
    {
        while(szSource[0] != 0)
            *szBuffer++ = (char)(*szSource++);
        return szBuffer;
    }

    size_t GetConsoleWidth()
    {
        // Only check this once per 100 messages
        if(nSaveConsoleWidth == 0 || (nMessageCounter % 100) == 0)
        {
#ifdef PLATFORM_WINDOWS
            CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ScreenInfo);
            nSaveConsoleWidth = (ScreenInfo.srWindow.Right - ScreenInfo.srWindow.Left);
#else
            // On non-Windows platforms, we assume that width of the console line
            // is 100 characters
            nSaveConsoleWidth = 100;
#endif
        }

        return nSaveConsoleWidth;
    }

    const char * szMainTitle;                       // Title of the text (usually name)
    const char * szSubTitle;                        // Title of the text (can be name of the tested file)
    size_t nSaveConsoleWidth;                       // Saved width of the console window, in chars
    size_t nMessageCounter;
    size_t nTextLength;                             // Length of the previous progress message
    DWORD dwPrevTickCount;
    bool bMessagePrinted;
};
