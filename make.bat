@ECHO OFF
set PROJECT_DIR=build
set INSTALL_DIR=_bin

if "%~1"=="" goto BLANK
if "%~1"=="debug" goto DEBUG
if "%~1"=="release" goto RELEASE
if "%~1"=="install" goto INSTALL
if "%~1"=="clean" goto CLEAN
if "%~1"=="rebuild" goto REBUILD
echo Invalid option "%~1"
goto :DONE
@ECHO ON

:BLANK
cmake -H. -B %PROJECT_DIR% -A "x64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
GOTO DONE

:DEBUG
cmake -H. -B %PROJECT_DIR% -A "x64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Debug --target install
GOTO DONE

:RELEASE
cmake -H. -B %PROJECT_DIR% -A "x64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Release --target install
GOTO DONE

:INSTALL
cmake -H. -B %PROJECT_DIR% -A "x64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Debug --target install
cmake --build %PROJECT_DIR% --config Release --target install
GOTO DONE

:CLEAN
rmdir /Q /S %PROJECT_DIR% 2>NUL
rmdir /Q /S %INSTALL_DIR% 2>NUL
GOTO DONE

:REBUILD
rmdir /Q /S "build\Debug\" 2>NUL
rmdir /Q /S "build\PTVC_Project_GL.dir\" 2>NUL
del /q "build\PTVC_Project_GL.*" 2>NUL
cmake -H. -B %PROJECT_DIR% -A "x64" -DCMAKE_INSTALL_PREFIX=%INSTALL_DIR%
cmake --build %PROJECT_DIR% --config Debug --target ALL_BUILD
GOTO DONE

:DONE
