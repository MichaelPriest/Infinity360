@echo off
setlocal
if "%XEDK%"=="" (
  echo ERRO: XEDK nao foi encontrado. Abra um Xbox 360 Developer Command Prompt autorizado.
  exit /b 1
)
set "MSBUILD=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
if not exist "%MSBUILD%" (
  echo ERRO: MSBuild 4.0 nao foi encontrado.
  exit /b 1
)
if not exist "..\..\dist\Xbox360" mkdir "..\..\dist\Xbox360"
"%MSBUILD%" AuroraX-Xbox360.vcxproj /t:Build /p:Configuration=Release /p:Platform="Xbox 360"
if errorlevel 1 exit /b %errorlevel%
if not exist "..\..\dist\Xbox360\default.xex" (
  echo ERRO: a compilacao terminou sem gerar default.xex.
  exit /b 1
)
echo Build pronta: dist\Xbox360\default.xex
