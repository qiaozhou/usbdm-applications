USBDM V4.9
=========================
V4.9.4 (April 2012) -
   CFVx command line programming failed
   Added Extented options to programmers   
V4.9.3 (March 2012) -
    More late bug fixes 
    Command line operation of stand-alone programmer results in disconnected process. 
    Added '-execute' option to above. 
    Fixed bug in Codewarrior 10.1 install
    CW 10.1 & 10.2 may both be installed together now. 

USBDM_Linux_V4.9.tar.gz
    Linux version - minimal testing done.

V4.9.2 (February 2012) -
    Fixed alignment bug in HCS12 programmer for newer targets using PFlash
    Some small fixes

V4.9.0 (February 2012) -
    Extensive changes to HCS12 programmer
    Added programming algorithms for several HCS12 devices (HY,HA,XE,XS,P).
    Miscellaneous small fixes
    New bugs (I'm sure)

	
USBDM_Flash_Images_4_9a.zip
---------------------------
   These are very similar to the Flash images included in the .msi file
   except that they have improved CDC (Serial over USB) code.  BDM code is
   unchanged.  Use of serial port with this version should be more reliable
   (i.e. drop fewer characters when busy) but utilization of USB may be much 
   higher.

Known Issues
====================================
Codewarrior Legacy Versions (WIN32)
- CFV1 Legacy 
- HCS08 Legacy
   Fails on secured target - Use stand-alone programmer to unsecure device.
- RS08 Legacy
   Not supported - Update to Eclipse version
The above issues will not be rectified since the programming isn't done
by USBDM but by codewarrior.

Codewarrior V10 (Eclipse) (WIN32/Linux)
- CFV1+, Kinetis
   Make sure you select the "ALL" or "Selective" option for erase.  The 
   "Mass" option will result in a not-blank programming error since Mass 
   erasing/unsecuring these devices does NOT result in a blank device.

- (Linux) GDI files may crash when changing target type in debugger i.e.
   moving from HCS08 devices to CFV1.  This is due to problems with 
   wxwidgets.

Flash programmers & Utilities
GUI Issues under windows
   - The BDM selection box only display a single item when expanded.  Other
   alternatives are available using up/down arrows or (very careful) 
   clicking on up/down control.

Device support
   - Support for programming individual devices depends on being able to 
   obtain relevent information. Some devices are not supported because of
   this.  If there is a device missing first check that a detailed 
   decription of the device (e.g. Reference Manual) is publicly available.
   If the device is similar to a supported device it may be possible to 
   add support by editing the device files (in XML!).

- Eclipse
   Only RS08, HCS08, CFV1, CFVx devices are supported.

Operating Systems 
================================
The following have been tested and appear to work

   Various utilities provided (XP, WIN7-32/64, Linux-Mint(in progress)
   CodeWarrior Development Studio for Microcontrollers V6.3 (not RS08) (XP, WIN7-32/64)
   CodeWarrior Development Studio for S12(X) V5.0 (XP, WIN7-32/64)
   CodeWarrior for ColdFire V7.2 (XP)
   CodeWarrior for DSC56800E v8.3 (Only limited MC56F80xx) (XP)
   CW for MCU v10.0 (XP, WIN7-32/64, Linux-32)

Testing
=================================================
Hardware:
--------------
USBDM_JMxx, 
USBDM_CF_JMxx, 
USBDM_JS16, 
USBDM_SER_JS16, 
USBDM_SER_CF_JS16, 
USBDM_CF_JS16, 
TBDML(Minimal), 
Witztronics TBDML
MC56F8006 Demo board
TWR-K60N512,
TWR-K40X256,
TWR-MCF51JF,
TWR-RS08/HCS08

Targets
--------------------
Tested device list is provided in the programmer documentation

IDEs
-----------------------
as  above

Platforms
----------------------------------------------------
Linux 32-bit (Mint - in progress)
Windows XP
Windows 7 Ultimate (32-bit)
Windows 7 Ultimate (64-bit but running 32-bit IDE)
