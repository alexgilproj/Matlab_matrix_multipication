@Echo off
rem You must use the Windows 7 WDK. Verify it exists!
IF NOT EXIST C:\WinDDK\7600.16385.1\License.rtf GOTO BAD_WDK
ECHO Using Windows 7 WDK 7.1.0 (7600.16385.1)
set DDKROOT=C:\WinDDK\7600.16385.1

set CERT=%1
REM See if we are using the "Test" Certificate or the user specified one.
if (%CERT%) NEQ () GOTO CERT_READY
REM Make sure any existing Test Certificate is deleted and removed from the registry before creating a new certificate
ECHO Creating Test Certification
SET CERT="TEST"
cmd /c %DDKROOT%\bin\x86\certmgr.exe -del -all -s TestCertStore
del TestCert.cer
cmd /c %DDKROOT%\bin\x86\Makecert -r -pe -ss TestCertStore -n "CN=TestCert" TestCert.cer
if %ERRORLEVEL% NEQ 0 GOTO FAILED
:CERT_READY


REM ____________________________________________________________________________________________________________________________________
:BUILD_WIN7_X64_CHECKED
ECHO Building 64-bit Checked Windows 7 (Intel / AMD 64 bit)
cmd /c "pushd . && %DDKROOT%\bin\setenv.bat %DDKROOT% chk x64 WIN7 no_oacr  && popd && build -cZe" 
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "copy %DDKROOT%\redist\wdf\amd64\WdfCoInstaller01009.dll objchk_win7_amd64\amd64"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\selfsign\Inf2Cat.exe /driver:objchk_win7_amd64\amd64 /os:7_X64"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

if %CERT% NEQ "TEST" GOTO SIGN_WIN7_X64_CHECKED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_amd64\amd64\XDMAx64.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_amd64\amd64\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
GOTO BUILD_WIN7_X64_FREE

:SIGN_WIN7_X64_CHECKED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_amd64\amd64\XDMAx64.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_amd64\amd64\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

REM ____________________________________________________________________________________________________________________________________
:BUILD_WIN7_X64_FREE
ECHO Building 64-bit Free Windows 7 (Intel / AMD 64 bit)
cmd /c "pushd . && %DDKROOT%\bin\setenv.bat %DDKROOT% fre x64 WIN7 no_oacr  && popd && build -cZe" 
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "copy %DDKROOT%\redist\wdf\amd64\WdfCoInstaller01009.dll objfre_win7_amd64\amd64"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\selfsign\Inf2Cat.exe /driver:objfre_win7_amd64\amd64 /os:7_X64"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

if %CERT% NEQ "TEST" GOTO SIGN_WIN7_X64_FREE
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_amd64\amd64\XDMAx64.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_amd64\amd64\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
GOTO BUILD_WIN7_X86_CHECKED

:SIGN_WIN7_X64_FREE
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_amd64\amd64\XDMAx64.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_amd64\amd64\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

REM ____________________________________________________________________________________________________________________________________
:BUILD_WIN7_X86_CHECKED
ECHO Building 32-bit Checked Windows 7 (x86)
cmd /c "pushd . && %DDKROOT%\bin\setenv.bat %DDKROOT% chk x86 WIN7 no_oacr  && popd && build -cZe" 
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "copy %DDKROOT%\redist\wdf\x86\WdfCoInstaller01009.dll objchk_win7_x86\i386"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\selfsign\Inf2Cat.exe /driver:objchk_win7_x86\i386 /os:7_X86"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

if %CERT% NEQ "TEST" GOTO SIGN_WIN7_X86_CHECKED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_x86\i386\XDMAx86.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_x86\i386\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
GOTO BUILD_WIN7_X86_FREE

:SIGN_WIN7_X86_CHECKED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_x86\i386\XDMAx86.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objchk_win7_x86\i386\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

REM ____________________________________________________________________________________________________________________________________
:BUILD_WIN7_X86_FREE
ECHO Building 32-bit Free Windows 7 (x86)
cmd /c "pushd . && %DDKROOT%\bin\setenv.bat %DDKROOT% fre x86 WIN7 no_oacr  && popd && build -cZe" 
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "copy %DDKROOT%\redist\wdf\x86\WdfCoInstaller01009.dll objfre_win7_x86\i386"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\selfsign\Inf2Cat.exe /driver:objfre_win7_x86\i386 /os:7_X86"
if %ERRORLEVEL% NEQ 0 GOTO FAILED

if %CERT% NEQ "TEST" GOTO SIGN_WIN7_X86_FREE
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_x86\i386\XDMAx86.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\SignTool.exe sign /v /s TestCertStore /n "TestCert" /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_x86\i386\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
Goto EXIT

:SIGN_WIN7_X86_FREE
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_x86\i386\XDMAx86.cat"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
cmd /c "%DDKROOT%\bin\x86\Signtool.exe sign /v /ac ..\..\MSCV-VSClass3.cer /s MY /n %CERT% /t http://timestamp.verisign.com/scripts/timestamp.dll objfre_win7_x86\i386\XDMA.sys"
if %ERRORLEVEL% NEQ 0 GOTO FAILED
Goto EXIT

:BAD_WDK
ECHO Windows 7 WDK NOT FOUND (7600.16385.1)

:FAILED
ECHO BUILD FAILED!!!!
EXIT /B 255

:EXIT

