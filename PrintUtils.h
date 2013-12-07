#ifndef _PRINT_UTILS_H_
#define _PRINT_UTILS_H_

#include <Windows.h>
#include <iostream>  // include important C/C++ stuff   
#include <fstream>
#include <sstream>   // ostringstream
#include <string>
#include <memory.h>   

#include <stdio.h>   // va_arg
#include <stdarg.h>  // va_arg

using namespace std;



// PROTOTYPES  //////////////////////////////////////////////   

class PrintUtils
{
public:
   PrintUtils();
   ~PrintUtils();

   int initPrintUtils ();
   int closePrintUtils ();

   void printError   (char* errorName);
   void printError   (const char* errorName);
   void printLog     (char* logText);
   void printNumbers (int numargs, ...);

   void printFloat(const float& number);
   void printFloatArray(const float floatArray[], const int& length);

   int printRect (HWND hwnd);
   int printRect (RECT rect);

   int printOutput (const string& output);
   int printOutput (TCHAR output[]);
   int printOutput (const TCHAR output[]);

private:
   PrintUtils(const PrintUtils& printUtils);
   PrintUtils& operator=(const PrintUtils& printUtils);

   int       g_flagOnce;    // checker
   ofstream  g_errorFile;
   ofstream  g_logFile;
   ofstream  g_outputFile;

   char buffer[256];        // general printing buffer   
   char smallBuffer[20];
};

static PrintUtils ptt;



#endif
