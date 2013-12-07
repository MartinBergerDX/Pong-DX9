#include "PrintUtils.h"



// FUNCTIONS ////////////////////////////////////////////////   

PrintUtils::PrintUtils()
{
   g_flagOnce = 0;
   initPrintUtils();
}


PrintUtils::~PrintUtils()
{
   closePrintUtils();
}


int PrintUtils::initPrintUtils()
{
   g_errorFile = ofstream ("error.txt", std::ios_base::app);
   if (! g_errorFile.is_open ())
      return -1;

   g_logFile = ofstream ("log.txt", std::ios_base::app);
   if (! g_logFile.is_open ())
      return -2;

   g_outputFile = ofstream ("output.txt", std::ios_base::trunc);
   if (! g_outputFile.is_open ())
      return -3;

   return 1;
}


int PrintUtils::closePrintUtils()
{
   if (g_errorFile.is_open ())
      g_errorFile.close ();

   if (g_logFile.is_open ())
      g_logFile.close ();

   if (g_outputFile.is_open ())
      g_outputFile.close ();

   return 1;
}


void PrintUtils::printError (char* errorName)
{
	// print error to log.txt
   //ofstream errorFile = ofstream ("error.txt", std::ios_base::app);
	if (g_errorFile.is_open ())
	{
		g_errorFile << errorName << endl;
	}
	//errorFile.close ();
}


void PrintUtils::printError (const char* errorName)
{
	// print error to log.txt
	if (g_errorFile.is_open ())
	{
		g_errorFile << errorName << endl;
	}
}


void PrintUtils::printLog (char* logText)
{
	if (g_logFile.is_open ())
	{
		g_logFile << logText << endl;
	}
   else
   {
      ofstream logFile = ofstream ("log.txt", std::ios_base::app);
      logFile << "warning: log file was not open." << endl;
      logFile << logText << endl;
      logFile.close ();
   }
}


// uses: printLog function.
void PrintUtils::printNumbers (int numargs, ...) // long type only
{
   va_list listPointer;
   va_start (listPointer, numargs);

   long lng_1 = 0;

   memset (buffer, ' ', _countof (buffer));
   buffer[0] = '\0';
   memset (smallBuffer, ' ', _countof (smallBuffer));
   smallBuffer[0] = '\0';

   for (int i = 0; i < numargs; i++)
   {
      lng_1 = va_arg (listPointer, long);
      _ltoa_s (lng_1, smallBuffer, _countof (smallBuffer), 10);
      strcat_s (smallBuffer, _countof (smallBuffer), "\n");
      strcat_s (buffer, _countof (buffer), smallBuffer);
      memset (smallBuffer, ' ', _countof (smallBuffer));
      smallBuffer[0] = '\0';
   }

   printOutput (buffer);

   va_end (listPointer);
}


void PrintUtils::printFloat(const float& number)
{
	if (g_outputFile.is_open ())
	{
		g_outputFile << number << endl;
	}
   else
   {
      ofstream outputFile = ofstream ("output.txt", std::ios_base::trunc);
      outputFile << "warning: output file was not open." << endl;
      outputFile << number << endl;
      outputFile.close ();
   }
}


void PrintUtils::printFloatArray(const float floatArray[], const int& length)
{
	if (g_outputFile.is_open())
	{
      for (int i = 0; i < length; i++)
      {
         if (i > 4) g_outputFile << endl;
		   g_outputFile << floatArray[i];
         g_outputFile << " ";
      }
      g_outputFile << endl;
	}
   else
   {
      ofstream outputFile = ofstream ("output.txt", std::ios_base::trunc);
      outputFile << "warning: output file was not open." << endl;
      outputFile.close();
   }
}


// uses: printLog(), printNumbers().
int PrintUtils::printRect (HWND hwnd)
{
   RECT rect;

   GetClientRect(hwnd, &rect);   

   printLog ("printRect(HWND hwnd): ");
   printNumbers (4, rect.left, rect.top, rect.right, rect.bottom);

   return 1;
}


// uses: printLog(), printNumbers().
int PrintUtils::printRect (RECT rect)
{
   printLog ("printRect(RECT rect): ");
   printNumbers (4, rect.left, rect.top, rect.right, rect.bottom);

   return 1;
}


int PrintUtils::printOutput (const string& output)
{
	if (g_outputFile.is_open ())
	{
		g_outputFile << output.c_str() << endl;
	}
   else
   {
      ofstream outputFile = ofstream ("output.txt", std::ios_base::trunc);
      outputFile << "warning: output file was not open." << endl;
      outputFile << output.c_str() << endl;
      outputFile.close ();
   }

   return 1;
}


int PrintUtils::printOutput (TCHAR output[])
{
   /*wostringstream wss;
   wss << output << endl;
   string str (wss.str().c_str());*/
#ifdef UNICODE
   wstring wstr_1(output);
   string str_1(wstr_1.begin(), wstr_1.end());
#else
   string str_1(output);
#endif

	if (g_outputFile.is_open ())
	{
      g_outputFile << str_1 << endl;
	}
   else
   {
      ofstream outputFile = ofstream ("output.txt", std::ios_base::app);
      outputFile << "function: PrintUtils::printOutput (TCHAR output[])" << endl;
      outputFile << "warning: output file was not open." << endl;
      outputFile << str_1 << endl;
      outputFile.close ();
   }

   return 1;
}


int PrintUtils::printOutput (const TCHAR output[])
{
#ifdef UNICODE
   wstring wstr_1(output);
   string str_1(wstr_1.begin(), wstr_1.end());
#else
   string str_1(output);
#endif

	if (g_outputFile.is_open ())
	{
      g_outputFile << str_1 << endl;
	}
   else
   {
      ofstream outputFile = ofstream ("output.txt", std::ios_base::app);
      outputFile << "function: PrintUtils::printOutput (const TCHAR output[])" << endl;
      outputFile << "warning: output file was not open." << endl;
      outputFile << str_1 << endl;
      outputFile.close ();
   }

   return 1;
}