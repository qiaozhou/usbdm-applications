@echo off
cls
set VERSION=4_9_5
set VERSIONn=4.9.5

set WIX_DIR="c:\Program Files\Windows Installer XML v3.5\bin\"
set HEAT=%WIX_DIR%heat.exe
set CANDLE=%WIX_DIR%candle.exe
set LIGHT=%WIX_DIR%light.exe
set TORCH=%WIX_DIR%torch.exe
set PYRO=%WIX_DIR%pyro.exe

rem .wxs regenerated Files 
set REBUILT_WXS=FlashImages.wxs WizardPatchData.wxs Device_data.wxs USBDM_Drivers.wxs EclipsePlugin_1.wxs EclipsePlugin_2.wxs

set CANDLE_FILES=*.wxs
set LIGHT_FILES=*.wixobj

set LIGHT_OPTIONS=-ext WixUIExtension -ext WixUtilExtension -sw0204
set LIGHT_DIRS=-b bin\Device_data -b bin\FlashImages -b wizard_patches -b plugins -b USBDM_Drivers 

set HEAT_OPTIONS=-srd -ke -gg -sfrag -template fragment -sw5150
set MSI_FILE=USBDM_%VERSION%_Win
set PATCH=patch_1_1

if "%1"=="patch" goto doPatch
if "%1"=="clean" goto doMake
if "%1"==""      goto doMake
echo "Unknown option %1"
goto finish

:doMake
del *.wixobj
del %REBUILT_WXS%
del %MSI_FILE%.wixpdb
del %MSI_FILE%.msi

if "%1"=="clean" goto finish

%HEAT% dir .\bin\Device_data                %HEAT_OPTIONS% -cg Cg.Device_data     -dr D.Device_data      -out Device_data.wxs
%HEAT% dir .\bin\FlashImages                %HEAT_OPTIONS% -cg Cg.FlashImages     -dr D.FlashImages      -out FlashImages.wxs
%HEAT% dir .\wizard_patches                 %HEAT_OPTIONS% -cg Cg.WizardPatchData -dr D.WizardPatchData  -out WizardPatchData.wxs
%HEAT% dir .\USBDM_Drivers                  %HEAT_OPTIONS% -cg Cg.USBDM_Drivers   -dr D.USBDM_Drivers    -out USBDM_Drivers.wxs
%HEAT% dir .\plugins                        %HEAT_OPTIONS% -cg Cg.EclipsePlugin_1 -dr D.EclipsePlugin_1  -out EclipsePlugin_1.wxs
%HEAT% dir .\plugins                        %HEAT_OPTIONS% -cg Cg.EclipsePlugin_2 -dr D.EclipsePlugin_2  -out EclipsePlugin_2.wxs
%CANDLE% -dProductVersion=%VERSIONn% %CANDLE_FILES%
%LIGHT% %LIGHT_OPTIONS% %LIGHT_DIRS% -out %MSI_FILE% %LIGHT_FILES%
goto finish
:doPatch

%TORCH% -p -xi USBDM_4_9_0_Win.wixpdb %MSI_FILE%.wixpdb -out %PATCH%\diff.wixmst

%CANDLE% %PATCH%.wxs
%LIGHT%  %PATCH%.wixobj -out %PATCH%\%PATCH%.wixmsp
%PYRO%   %PATCH%\%PATCH%.wixmsp -out %PATCH%\%PATCH%.msp -t USBDMPatch %PATCH%\diff.wixmst

:finish
pause