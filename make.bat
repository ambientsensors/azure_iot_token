@echo off

IF NOT EXIST "%ZENTRIOS_SDK_PATH%" (
ECHO .
ECHO --------------------------------------------
ECHO ERROR: ZentriOS SDK NOT FOUND
ECHO --------------------------------------------
ECHO The ZentriOS SDK directory was not found at "%ZENTRIOS_SDK_PATH%".
ECHO Update the 'ZENTRIOS_SDK_PATH' enviroment variable to the correct path.
ECHO Note: You may need to restart your computer for changes to take effect.
ECHO .
EXIT -1
)

set "PROJECT_DIR=%~dp0"

pushd "%~dp0" 
cd /d "%ZENTRIOS_SDK_PATH%"
call .\make.exe external.azure_iot_token %* 2>&1 | .\tools\common\Win32\tee.exe "%PROJECT_DIR%build.log"
%SYSTEMROOT%\System32\find "Error 2" "%PROJECT_DIR%build.log" >nul
IF %errorlevel% equ 1 GOTO finished

:report_build_error
call .\tools\common\Win32\dev_connect.exe --action report_error --path "%PROJECT_DIR%/build.log" --tag build

:finished
popd