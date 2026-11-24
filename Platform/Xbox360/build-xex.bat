@echo off
setlocal
cd /d "%~dp0" || exit /b 1
if "%XEDK%"=="" (
  echo ERRO: XEDK nao foi encontrado. Abra um Xbox 360 Developer Command Prompt autorizado.
  exit /b 1
)
if not exist "%XEDK%\include" (
  echo ERRO: XEDK nao aponta para um SDK instalado.
  exit /b 1
)
set "MSBUILD=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
if not exist "%MSBUILD%" (
  echo ERRO: MSBuild 4.0 nao foi encontrado.
  exit /b 1
)
set "OUT=%~dp0..\..\dist\Xbox360"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%" exit /b 1
rem Never mistake stale binaries for a newly built package.
for %%F in (default.xex home.tga) do (
  if exist "%OUT%\%%F" del /q "%OUT%\%%F"
  if exist "%OUT%\%%F" (
    echo ERRO: nao foi possivel remover %%F antigo.
    exit /b 1
  )
)
"%MSBUILD%" AuroraX-Xbox360.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform="Xbox 360"
if errorlevel 1 exit /b %errorlevel%
if not exist "%OUT%\default.xex" (
  echo ERRO: a compilacao terminou sem gerar default.xex.
  exit /b 1
)
if not exist "%OUT%\home.tga" (
  echo ERRO: recurso home.tga nao foi copiado.
  exit /b 1
)
echo Build pronta: "%OUT%\default.xex"
