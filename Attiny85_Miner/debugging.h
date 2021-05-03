#ifndef _DEBUGGING_H__
#define _DEBUGGING_H__

#ifndef DEBUG_LOGGER
#define DEBUG_LOGGER    mySerial
#endif

#if defined(_DEBUG) || defined(DEBUG)
# if !defined(DEBUG_DEFINED)
#  define DEBUG_DEFINED
# endif
#endif
  
#ifdef DEBUG_DEFINED
  #define debugPrint(x)             DEBUG_LOGGER.print(x)
  #define debugPrintln(x)           DEBUG_LOGGER.println(x)
#else
  #define debugPrint(x)
  #define debugPrintln(x)
  #define debugPrintf(x,...)
#endif

#endif // _DEBUGGING_H__
