@echo off

:: Read in previous build number from file and increase by 1
set /p BUILD_NUMBER= < increase-version.dat
set /a BUILD_NUMBER= %BUILD_NUMBER%-1

:: Write new build number out to file for next time
echo %BUILD_NUMBER% > increase-version.dat