@echo off
setlocal
where cl.exe >nul 2>nul
if errorlevel 1 (
  if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
  ) else (
    echo Open a Visual Studio C++ x64 Native Tools command prompt first.
    exit /b 1
  )
)
cd /d "%~dp0.."
if not exist .build\tests mkdir .build\tests
cl.exe /nologo /std:c++17 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS /Ichecks/host/stubs /Fe:.build/tests/integrated_master.exe /Fo:.build/tests/integrated_master.obj checks/host/integrated_master_tests.cpp
if errorlevel 1 exit /b 1
.build\tests\integrated_master.exe
if errorlevel 1 exit /b 1
for %%N in (1 2 3 4) do (
  cl.exe /nologo /std:c++17 /EHsc /W4 /WX /D_CRT_SECURE_NO_WARNINGS /DTEST_NODE=%%N /Ichecks/host/stubs /Fe:.build/tests/integrated_slave%%N.exe /Fo:.build/tests/integrated_slave%%N.obj checks/host/integrated_receiver_tests.cpp
  if errorlevel 1 exit /b 1
  .build\tests\integrated_slave%%N.exe
  if errorlevel 1 exit /b 1
)
