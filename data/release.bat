@echo off
setlocal
pushd "%~dp0" || exit /b 1
if not exist "III.VC.SA.WindowedMode.asi" goto :error
if not exist "III.VC.SA.WindowedMode.pdb" goto :error
"..\tools\EmbedPDB\EmbedPDB.exe" "III.VC.SA.WindowedMode.asi"
if errorlevel 1 goto :error
powershell -NoProfile -ExecutionPolicy Bypass -File "..\sign.ps1" -Path "%CD%\III.VC.SA.WindowedMode.asi"
if errorlevel 1 goto :error
if exist "III.VC.SA.WindowedMode.zip" del "III.VC.SA.WindowedMode.zip"
".\7za.exe" a -tzip "III.VC.SA.WindowedMode.zip" "III.VC.SA.WindowedMode.asi" "..\README.md" "..\CHANGELOG.md"
if errorlevel 1 goto :error
popd
exit /b 0
:error
popd
exit /b 1

7-Zip Extra
~~~~~~~~~~~
License for use and distribution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Copyright (C) 1999-2016 Igor Pavlov.

7-Zip Extra files are under the GNU LGPL license.


Notes: 
  You can use 7-Zip Extra on any computer, including a computer in a commercial 
  organization. You don't need to register or pay for 7-Zip.


GNU LGPL information
--------------------

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You can receive a copy of the GNU Lesser General Public License from 
  http://www.gnu.org/

