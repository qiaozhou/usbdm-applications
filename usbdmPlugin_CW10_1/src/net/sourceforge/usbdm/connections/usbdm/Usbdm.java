package net.sourceforge.usbdm.connections.usbdm;

import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.util.ArrayList;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;

/*
USBDM_API USBDM_ErrorCode  USBDM_Init(void);
USBDM_API USBDM_ErrorCode  USBDM_Exit(void);

USBDM_API USBDM_ErrorCode  USBDM_FindDevices(unsigned int *deviceCount);
USBDM_API USBDM_ErrorCode  USBDM_ReleaseDevices(void);
USBDM_API USBDM_ErrorCode  USBDM_GetBDMDescription(const char **deviceDescription);
USBDM_API USBDM_ErrorCode  USBDM_GetBDMSerialNumber(const char **deviceDescription);
USBDM_API USBDM_ErrorCode  USBDM_Open(unsigned char deviceNo);
USBDM_API USBDM_ErrorCode  USBDM_Close(void);
 */

public class Usbdm {
   private static boolean libraryLoaded     = false;
   private static boolean libraryLoadFailed = false;
   private static native int init();
   private static native int exit();
   private static native int findDevices(int[] deviceCount);
   private static native int releaseDevices();
   private static native int openBDM(int deviceNum);
   private static native int closeBDM();
   private static native int getBDMDescription(byte[] description);
   private static native int getBDMSerialNumber(byte[] serialNumber);
   private static native int getBDMFirmwareVersion(BdmInformation bdmInfo);
   private static native String getErrorString( int errorNum);

   private static final int BDM_RC_OK = 0;

   // Class describing the USBDM interface capabilities
   //
   public static class BdmInformation {
      public int  BDMsoftwareVersion;   //!< BDM Firmware version
      public int  BDMhardwareVersion;   //!< Hardware version reported by BDM firmware
      public int  ICPsoftwareVersion;   //!< ICP Firmware version
      public int  ICPhardwareVersion;   //!< Hardware version reported by ICP firmware
      public int  capabilities;         //!< BDM Capabilities
      public int  commandBufferSize;    //!< Size of BDM Communication buffer
      public int  jtagBufferSize;       //!< Size of JTAG buffer (if supported)

      public BdmInformation() {
         super();
      }

      public BdmInformation(int bdmSV, int bdmHV, int icpSV, int icpHV, int cap, int comm, int jtag) {
         BDMsoftwareVersion = bdmSV;
         BDMhardwareVersion = bdmHV;
         ICPsoftwareVersion = icpSV;
         ICPhardwareVersion = icpHV;
         capabilities       = cap;
         commandBufferSize  = comm;
         jtagBufferSize     = jtag;
      }
      
      public String toString() {
         return 
         String.format("(bdmSWVer=%02X,", BDMsoftwareVersion)+
         String.format(" bdmHWVer=%02X,", BDMhardwareVersion)+
         String.format(" icpSWVer=%02X,", ICPsoftwareVersion)+
         String.format(" icpHWVer=%02X,", ICPhardwareVersion)+
         String.format(" cap=0x%04X,", capabilities)+
         String.format(" cBSize=%d,", commandBufferSize)+
         String.format(" jBSize=%d)", jtagBufferSize);
      }
   };

   // Class describing the BDM
   //
   public static class DeviceInfo {
      public String         deviceDescription;
      public String         deviceSerialNumber;
      public BdmInformation bdmInfo;
      public DeviceInfo(String desc, String ser, BdmInformation bdmI) {
         deviceDescription  = desc;
         deviceSerialNumber = ser;
         bdmInfo            = bdmI;
      }
      public String toString() {
         return "Desc="+deviceDescription+"; Ser="+deviceSerialNumber+"; info:"+bdmInfo.toString();
      }
   };

   private static boolean loadLibrary(Shell parent) {
      if (libraryLoaded)
         return true;
      try {
         String os = System.getProperty("os.name");
         if ((os != null) && os.toUpperCase().contains("LINUX")) {
//            System.err.println("Loading library: "+UsbdmCommon.LibUsbLibraryName_so );
            System.loadLibrary(UsbdmCommon.LibUsbLibraryName_so);
//            System.err.println("Loading library: "+UsbdmCommon.UsbdmLibraryName_so);
            System.loadLibrary(UsbdmCommon.UsbdmLibraryName_so);
         }
         else {
//            System.err.println("Loading library: "+UsbdmCommon.LibUsbLibraryName_dll );
            System.loadLibrary(UsbdmCommon.LibUsbLibraryName_dll);
//            System.err.println("Loading library: "+UsbdmCommon.UsbdmLibraryName_dll);
            System.loadLibrary(UsbdmCommon.UsbdmLibraryName_dll);
         }
//         System.err.println("Loading library: "+UsbdmCommon.UsbdmJniLibraryName);
         System.loadLibrary(UsbdmCommon.UsbdmJniLibraryName);
         init();
         libraryLoaded = true;
//         System.err.println("Libraries successfully loaded");
      } catch (Throwable e) {
         if (!libraryLoadFailed) {
            libraryLoadFailed = true;
            MessageBox msgbox = new MessageBox(parent, SWT.OK);
            msgbox.setText("USBDM Error");
            msgbox.setMessage("Loading of USBDM native library failed.");
            msgbox.open();
            e.printStackTrace();
         }
         System.err.println("USBDM Libraries failed to load");
         return false;
      }
      return true;
   }

   public static ArrayList<DeviceInfo> getDeviceList(Shell parent) {

      ArrayList<DeviceInfo> deviceList = new ArrayList<DeviceInfo>();

      if (! loadLibrary(parent))
         return deviceList;

      int rc;
      int deviceCount[] = new int[1];
      int deviceNum;
      byte[] description = new byte[200];
      byte[] serialNum   = new byte[200];
      Charset utf8Charset = Charset.forName("UTF-16LE");
      CharsetDecoder utf8CharsetDecoder = utf8Charset.newDecoder();

      BdmInformation  bdmInfo= new Usbdm.BdmInformation();
      DeviceInfo      deviceInfo;

      rc = Usbdm.findDevices(deviceCount);
      if (rc != BDM_RC_OK) {
         System.err.println("Usbdm.findDevices() - failed, Reason: "+Usbdm.getErrorString(rc));
         return deviceList;
      }
//      System.err.println("Usbdm.findDevices(): Found  " + deviceCount[0] + " devices");

      for (deviceNum=0; deviceNum < deviceCount[0]; deviceNum++) {
         String desc    = new String("Unresponsive device");;
         String serial  = new String("Unknown");

         rc = Usbdm.openBDM(deviceNum);
//         System.err.println("Usbdm.findDevices(): Opened");

         if (rc == BDM_RC_OK)
            rc = Usbdm.getBDMDescription(description);
         try {
            if (rc == BDM_RC_OK) {
               ByteBuffer buff = ByteBuffer.allocate(description[0]);
               buff.put(description, 1, description[0]);
               buff.rewind();
               desc = utf8CharsetDecoder.decode(buff).toString();
            }
         } catch (CharacterCodingException e) {
            e.printStackTrace();
         }

//         System.err.println("Usbdm.findDevices(): getBDMDescription() done");
         if (rc == BDM_RC_OK)
            rc = Usbdm.getBDMSerialNumber(serialNum);
         try {
            if (rc == BDM_RC_OK) {
               ByteBuffer buff = ByteBuffer.allocate(serialNum[0]);
               buff.put(serialNum, 1, serialNum[0]);
               buff.rewind();
               serial = utf8CharsetDecoder.decode(buff).toString();
            }
         } catch (CharacterCodingException e) {
            e.printStackTrace();
         }

//         System.err.println("Usbdm.findDevices(): getBDMSerialNumber() done");
         if (rc == BDM_RC_OK)
            rc = Usbdm.getBDMFirmwareVersion(bdmInfo);
         if (rc != BDM_RC_OK)
            bdmInfo = new BdmInformation();

//         System.err.println("Usbdm.findDevices(): getBDMFirmwareVersion() done");
//         System.err.println("Usbdm.findDevices(): bdmInfo = " + bdmInfo.toString());
         deviceInfo = new DeviceInfo(desc, serial, bdmInfo);
         deviceList.add(deviceInfo);

         Usbdm.closeBDM();
//         System.err.println("Usbdm.findDevices(): closeBDM() done");
      }
      Usbdm.releaseDevices(); // Release device list
      return deviceList;
   } 

   public void finalize() {
      exit();
      libraryLoaded     = false;
      libraryLoadFailed = false;
//      System.err.println("Usbdm() - garbage collected");
   }
}
