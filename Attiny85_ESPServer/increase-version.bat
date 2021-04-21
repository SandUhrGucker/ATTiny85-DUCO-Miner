@echo off

:: Get Tokens from WINDOWs System
setlocal EnableDelayedExpansion
FOR /F "skip=1 tokens=1-6" %%A IN ('WMIC Path Win32_LocalTime Get Day^,Hour^,Minute^,Month^,Second^,Year /Format:table') DO (
    if "%%B" NEQ "" (
        SET /A MAJOR_VERSION = %%F
		SET /A MINOR_VERSION = %%D
    )
)

:: Get git head information
:: git rev-parse --short HEAD > git-revision.dat
:: set /p GIT_REVISION= < git-revision.dat

:: Read in previous build number from file and increase by 1
set /p BUILD_NUMBER= < increase-version.dat
set /a BUILD_NUMBER= %BUILD_NUMBER%+1

:: Write new build number out to file for next time
echo %BUILD_NUMBER% > increase-version.dat

:: Output values to a c-header-file for program inclusion / access
echo #ifndef _VERSION_INFO_H__ > version-info.h
echo #define _VERSION_INFO_H__ >> version-info.h
echo. >> version-info.h
echo #define MAJOR_VERSION  %MAJOR_VERSION% >> version-info.h
echo #define MINOR_VERSION  %MINOR_VERSION% >> version-info.h
echo #define GIT_REVISION   %GIT_REVISION% >> version-info.h
echo #define BUILD_NUMBER   %BUILD_NUMBER% >> version-info.h
echo. >> version-info.h
echo #define VersionInfo()  String(MAJOR_VERSION) + "." + \ >> version-info.h
echo                        String(MINOR_VERSION) + "." + \ >> version-info.h
echo                        String(BUILD_NUMBER)  >> version-info.h
::echo                        String(BUILD_NUMBER)  + " (" + \ >> version-info.h
::echo                        String(GIT_REVISION) + ")" >> version-info.h
echo. >> version-info.h
echo #endif // _VERSION_INFO_H__ >> version-info.h
