@echo off
setlocal
rem Controlled native environment; no inherited compiler/Perl/config switches.
set "PATH=%SystemRoot%\system32;%SystemRoot%;%SystemRoot%\System32\Wbem"
set "CC="
set "CXX="
set "AS="
set "AR="
set "LD="
set "CFLAGS="
set "CPPFLAGS="
set "LDFLAGS="
set "CL="
set "_CL_="
set "LINK="
set "_LINK_="
set "CROSS_COMPILE="
set "OPENSSL_LOCAL_CONFIG_DIR="
set "PERL5OPT="
set "PERL5LIB="
set "PERLLIB="
set "PERL5DB="
set "PERL_MM_OPT="
set "PERL_MB_OPT="
set "PERL_JSON_BACKEND="
set "PERL_YAML_BACKEND="
set "HASHBANGPERL="
set "SOURCE_DATE_EPOCH="
set "INCLUDE="
set "LIB="
set "LIBPATH="
set "EXTERNAL_INCLUDE="
set "VSINSTALLDIR="
set "VCINSTALLDIR="
set "DevEnvDir="
set "VCToolsVersion="
set "VCToolsInstallDir="
set "WindowsSdkDir="
set "WindowsSDKVersion="
set "UCRTVersion="
set "UniversalCRTSdkDir="
for /f "tokens=1 delims==" %%V in ('set __VSCMD 2^>nul') do set "%%V="
for /f "tokens=1 delims==" %%V in ('set __VCVARS 2^>nul') do set "%%V="
for /f "tokens=1 delims==" %%V in ('set VSCMD_ 2^>nul') do set "%%V="
set "VSCMD_SKIP_SENDTELEMETRY=1"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=14.44
if errorlevel 1 exit /b %errorlevel%
if not "%VCToolsVersion%"=="14.44.35207" exit /b 2
set "PATH=F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\tool-bin;C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64;F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable\perl\bin;%PATH%"
set "CC=cl"
set "CXX=cl"
set "AS=nasm"
set "PERL=F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable\perl\bin\perl.exe"
set "PERL5LIB=F:\JericoEngine\Dependencies\tools\strawberry-perl-5.42.3.1-64bit-portable\perl\lib"
> "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\native-build-environment.txt" echo VCToolsVersion=%VCToolsVersion%
>> "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\native-build-environment.txt" echo WindowsSDKVersion=%WindowsSDKVersion%
>> "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\native-build-environment.txt" echo VCToolsInstallDir=%VCToolsInstallDir%
>> "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\native-build-environment.txt" echo WindowsSdkDir=%WindowsSdkDir%
>> "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\native-build-environment.txt" echo PATH=%PATH%
cd /d "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\source-work\openssl-1.1.1o"
if errorlevel 1 exit /b %errorlevel%
"%PERL%" Configure VC-WIN64A enable-static-engine enable-capieng no-ssl2 no-tests -utf-8 no-shared -FS --prefix="F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\install" --openssldir="F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\install"
if errorlevel 1 exit /b %errorlevel%
"%PERL%" configdata.pm --dump > "F:\JericoEngine\JericoEngine\build\n0-validation\local-packages\openssl-build-rev3\configdata-dump.txt"
if errorlevel 1 exit /b %errorlevel%
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\nmake.exe" /nologo build_libs
if errorlevel 1 exit /b %errorlevel%
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\nmake.exe" /nologo install_dev
exit /b %errorlevel%
