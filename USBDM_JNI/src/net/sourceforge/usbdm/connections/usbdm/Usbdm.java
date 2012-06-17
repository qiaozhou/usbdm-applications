package net.sourceforge.usbdm.connections.usbdm;

import java.util.ArrayList;
import java.util.ListIterator;

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
   private static native int init();
   private static native int exit();
   private static native int findDevices(int[] deviceCount);
   private static native int releaseDevices();
   private static native int openBDM(int deviceNum);
   private static native int closeBDM();
   private static native int getBDMDescription(char[] description);
   private static native int getBDMSerialNumber(char[] serialNumber);
   private static native int getBDMFirwareVersion(BdmInformation bdmInfo);
   private static native String getErrorString( int errorNum);

   private static native int getUsbdmPath(char[] serialNumber);

   private static final int BDM_RC_OK = 0;
   
   static {
      System.loadLibrary("libusb-1.0");
      System.loadLibrary("usbdm.4");
      System.loadLibrary("UsbdmJniWrapper");
   }

   // Class describing the USBDM interface capabilities
   //
   public static class BdmInformation {
      int  BDMsoftwareVersion;   //!< BDM Firmware version
      int  BDMhardwareVersion;   //!< Hardware version reported by BDM firmware
      int  ICPsoftwareVersion;   //!< ICP Firmware version
      int  ICPhardwareVersion;   //!< Hardware version reported by ICP firmware
      int  capabilities;         //!< BDM Capabilities
      int  commandBufferSize;    //!< Size of BDM Communication buffer
      int  jtagBufferSize;       //!< Size of JTAG buffer (if supported)
      
      public BdmInformation() {
         super();
      }
      public BdmInformation(int bdmSV, int bdmHV, int icpSV, int icpHV, int cap, int comm, int jtag) {
         BDMsoftwareVersion = bdmSV;
         BDMhardwareVersion = bdmHV;
         ICPsoftwareVersion = icpSV;
         ICPhardwareVersion = icpHV;
         capabilities       = cap;
         jtagBufferSize     = jtag;
      }
      public String toString() {
         return "("+BDMsoftwareVersion+","+BDMhardwareVersion+","+
                    ICPsoftwareVersion+","+ICPhardwareVersion+","+
                    capabilities+","+jtagBufferSize+")";
      }
   };

   // Class describing the bdm
   //
   public static class DeviceInfo {
      String         deviceDescription;
      String         deviceSerialNumber;
      BdmInformation bdmInfo;
      public DeviceInfo(String desc, String ser, BdmInformation bdmI) {
         deviceDescription  = desc;
         deviceSerialNumber = ser;
         bdmInfo            = bdmI;
      }
      public String toString() {
         return "Desc="+deviceDescription+"; Ser="+deviceSerialNumber+"; info:"+bdmInfo.toString();
      }
   };
   
   public static ArrayList<DeviceInfo> getDeviceList() throws Exception {
      ArrayList<DeviceInfo> deviceList;
      int rc;
      int deviceCount[] = new int[1];
      int deviceNum;
      char[] description = new char[200];
      char[] serialNum   = new char[200];
      BdmInformation  bdmInfo= new Usbdm.BdmInformation();
      DeviceInfo      deviceInfo;
      
      deviceList = new ArrayList<DeviceInfo>();
      rc = Usbdm.findDevices(deviceCount);
      if (rc != BDM_RC_OK)
         throw new Exception(Usbdm.getErrorString(rc));
      System.err.println("Number of BDMs found = "+deviceCount[0]);
      for (deviceNum=0; deviceNum < deviceCount[0]; deviceNum++) {
         rc = Usbdm.openBDM(deviceNum);
         if (rc != BDM_RC_OK)
            break;
         
         rc = Usbdm.getBDMDescription(description);
         if (rc != BDM_RC_OK)
            break;

         rc = Usbdm.getBDMSerialNumber(serialNum);
         if (rc != BDM_RC_OK)
            break;
      
         rc = Usbdm.getBDMFirwareVersion(bdmInfo);
         if (rc != BDM_RC_OK)
            break;
         deviceInfo = new DeviceInfo(new String(description, 1, (int)description[0]), 
                                     new String(serialNum, 1, (int)serialNum[0]),
                                     bdmInfo);
         deviceList.add(deviceInfo);
   
         rc = Usbdm.closeBDM();
         if (rc != BDM_RC_OK)
            break;
      }
      Usbdm.closeBDM();       // Just in case
      Usbdm.releaseDevices(); // Release device list
      if (rc != BDM_RC_OK)
         throw new Exception(Usbdm.getErrorString(rc));
      return deviceList;
   }
   
   public static void main(String[] args) {
      BdmInformation bdmInfo = new BdmInformation();
      Usbdm.init();

      try {
         ArrayList<DeviceInfo> deviceList = Usbdm.getDeviceList();
         ListIterator<Usbdm.DeviceInfo> it = deviceList.listIterator();
         
         while (it.hasNext()) {
            DeviceInfo di = it.next();
            System.err.println(di);
         }
      } catch (Exception opps) {
         System.err.println("Opps exception :"+opps.toString());
      }
      Usbdm.getBDMFirwareVersion(bdmInfo);
      System.err.println("info:"+bdmInfo.toString());
      Usbdm.exit();
    }
   
}
