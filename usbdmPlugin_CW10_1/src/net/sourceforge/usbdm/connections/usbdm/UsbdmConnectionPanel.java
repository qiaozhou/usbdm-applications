package net.sourceforge.usbdm.connections.usbdm;

import java.util.ArrayList;
import java.util.ListIterator;

import net.sourceforge.usbdm.connections.usbdm.Usbdm.BdmInformation;
import net.sourceforge.usbdm.connections.usbdm.Usbdm.DeviceInfo;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.debug.core.ILaunchConfiguration;
import org.eclipse.debug.core.ILaunchConfigurationWorkingCopy;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.widgets.FormToolkit;

import com.freescale.cdt.debug.cw.core.ui.publicintf.AbstractPhysicalConnectionPanel;
import com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsListener;
import com.freescale.cdt.debug.cw.core.ui.settings.PrefException;
import com.freescale.cdt.debug.cw.mcu.common.publicintf.ICWGdiInitializationData;
import com.swtdesigner.SWTResourceManager;

public class UsbdmConnectionPanel 
extends AbstractPhysicalConnectionPanel
implements ICWGdiInitializationData {

   final class DefaultBdmOptions extends UsbdmCommon.BdmOptions {
      public DefaultBdmOptions() {
         autoReconnect      = 1;
         guessSpeed         = 1;
         cycleVddOnConnect  = 0;
         cycleVddOnReset    = 0;
         leaveTargetPowered = 0;
         targetClockFreq    = 0;
         targetVdd          = UsbdmCommon.BDM_TARGET_VDD_OFF;
         useAltBDMClock     = UsbdmCommon.BDM_CLK_DEFAULT;
         usePSTSignals      = 0;
         useResetSignal     = 0;
         maskinterrupts     = 0;
         derivative_type    = 0;
         clockTrimFrequency = 0;
         clockTrimNVAddress = 0;
         doClockTrim        = false;
      }
   };

   protected final FormToolkit toolkit = new FormToolkit(Display.getCurrent());

   protected DefaultBdmOptions defaultBdmOptions = new DefaultBdmOptions();
   protected ArrayList<DeviceInfo> deviceList;
   protected ILaunchConfiguration launchConfiguration;

   protected String gdiDllName;
   protected String gdiDebugDllName;
   protected String connectionTypeId;
   protected String deviceNameId;
   protected String deviceNote = "";
   protected String attributeKey = UsbdmCommon.BaseAttributeName+".";

   // GUI Elements
   protected Combo  comboSelectBDM;
   protected Button btnRefreshBDMs;
   protected Label  lblBDMInformation;

   protected Button btnDefault;

   protected HexTextAdapter txtNVTRIMAddressAdapter;
   protected Label lblNvtrimAddress;
   protected Text txtTrimFrequency;
   protected DoubleTextAdapter txtTrimFrequencyAdapter;
   protected Text txtNVTRIMAddress;
   protected Label lblTrimFrequency;
   protected Label lblKhz;
   protected Label lblHex;
   protected Button btnTrimTargetClock;

   protected Group  grpClockTrim;
   protected Button btnTargetVddOff;
   protected Button btnTargetVdd3V3;
   protected Button btnTargetVdd5V;
   protected Button btnCycleTargetVddOnReset;
   protected Button btnCycleTargetVddOnConnect;
   protected Button btnLeaveTargetPowered;

   protected Button btnUseDebugBuild;

//   protected Label lblDeviceNote;

   /**
    * Dummy listener for WindowBuilder Pro. & debugging
    * 
    * @param parent
    * @param style
    */
   protected static class dummyISettingsListener implements ISettingsListener {
      public ModifyListener getModifyListener() {
         return new ModifyListener() {
            public void modifyText(ModifyEvent e) {
               System.err.println("dummyISettingsListener.ModifyListener.modifyText");
            }
         };
      }
      public SelectionListener getSelectionListener() {
         return new SelectionListener() {
            public void widgetSelected(SelectionEvent e) {
               System.err.println("dummyISettingsListener.SelectionListener.widgetSelected");
            }
            public void widgetDefaultSelected(SelectionEvent e) {
               System.err.println("dummyISettingsListener.SelectionListener.widgetDefaultSelected");
            }
         };
      }
      public void settingsChanged() {
      }
   }

   /**
    * Dummy constructor for WindowBuilder Pro. & debugging
    * 
    * @param parent
    * @param style
    */
   public UsbdmConnectionPanel(Composite parent, int style) {
      super(new dummyISettingsListener(), parent, style, "None"); //$NON-NLS-1$
      //            create();
   }

   /**
    * Actual constructor 
    * 
    * @param listener
    * @param parent
    * @param swtstyle
    * @param protocolPlugin    Indicates the connection type e.g. "HCS08 GDI"
    * @param connectionTypeId  A long string indicating the architecture??
    */
   public UsbdmConnectionPanel( 
         ISettingsListener listener, 
         Composite         parent,
         int               swtstyle, 
         String            protocolPlugin,
         String            connectionTypeId) {

      super(listener, parent, swtstyle, connectionTypeId);

      //      System.err.println("USBDMConnectionPanel::USBDMConnectionPanel(protocolPlugin="+protocolPlugin+", connectionTypeId = "+connectionTypeId+")");

      this.connectionTypeId = connectionTypeId;
      addDisposeListener(new DisposeListener() {
         public void widgetDisposed(DisposeEvent e) {
            toolkit.dispose();
         }
      });
   }

   public void create() {
      createContents(this);
      addSettingsChangedListeners();
   }

   protected void appendContents(Composite comp) {
//      System.err.println("UsbdmConnectionPanel::appendContents(msg = '"+message+"')");
//      Group noteGrp = new Group(comp, SWT.NONE);
//      noteGrp.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 3, 1));
//      toolkit.adapt(noteGrp);
//      toolkit.paintBordersFor(noteGrp);
//      lblDeviceNote = new Label(noteGrp, SWT.NONE);
//      lblDeviceNote.setBounds(10, 10, 515, 40);
//      System.err.println("UsbdmConnectionPanel::appendContents()");
//      lblDeviceNote = new Label(comp, SWT.NONE);
//      lblDeviceNote.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 3, 1));
//      System.err.println("UsbdmConnectionPanel::appendContents(note = '"+deviceNote+"')");
//      lblDeviceNote.setText("Note: "+deviceNote);
   }
   
   protected void createContents(Composite comp) {
      toolkit.setBackground(SWTResourceManager.getColor(SWT.COLOR_WIDGET_BACKGROUND));
      toolkit.adapt(comp);
      toolkit.paintBordersFor(comp);
      GridLayout gridLayout = new GridLayout(3, false);
      setLayout(gridLayout);

      Group grpSelectBdm = new Group(comp, SWT.NONE);
      grpSelectBdm.setText("Preferred BDM");
      grpSelectBdm.setLayout(new GridLayout(2, false));
      grpSelectBdm.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 2, 1));
      toolkit.adapt(grpSelectBdm);
      toolkit.paintBordersFor(grpSelectBdm);

      comboSelectBDM = new Combo(grpSelectBdm, SWT.READ_ONLY);
      comboSelectBDM.setToolTipText("Allows selection of  preferred BDM from those currently connected.\r\nOnly used if multiple BDMs are attached when debugging.");
      comboSelectBDM.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            updateBdmDescription();
         }
      });
      GridData gd_comboSelectBDM = new GridData(SWT.FILL, SWT.CENTER, true, false, 1, 1);
      gd_comboSelectBDM.minimumWidth = 200;
      comboSelectBDM.setLayoutData(gd_comboSelectBDM);
      toolkit.adapt(comboSelectBDM);
      toolkit.paintBordersFor(comboSelectBDM);

      populateBdmChoices(null, false);

      btnRefreshBDMs = new Button(grpSelectBdm, SWT.NONE);
      btnRefreshBDMs.setToolTipText("Check for connected BDMs");
      btnRefreshBDMs.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            populateBdmChoices(null, true);
         }
      });
      toolkit.adapt(btnRefreshBDMs, true, true);
      btnRefreshBDMs.setText("Refresh");

      lblBDMInformation = new Label(grpSelectBdm, SWT.NONE);
      lblBDMInformation.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false, 1, 1));
      lblBDMInformation.setToolTipText("Description of selected BDM");
      toolkit.adapt(lblBDMInformation, true, true);
      lblBDMInformation.setText("USBDM-CF");
      new Label(grpSelectBdm, SWT.NONE);

      Composite composite_1 = new Composite(comp, SWT.NONE);
      composite_1.setLayout(new FillLayout(SWT.HORIZONTAL));
      GridData gd_composite_1 = new GridData(SWT.FILL, SWT.TOP, false, false, 1, 1);
      gd_composite_1.verticalIndent = 10;
      gd_composite_1.horizontalIndent = 5;
      composite_1.setLayoutData(gd_composite_1);
      toolkit.adapt(composite_1);
      toolkit.paintBordersFor(composite_1);

      btnDefault = new Button(composite_1, SWT.NONE);
      btnDefault.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            restoreDefaultSettings();
         }
      });
      btnDefault.setToolTipText("Restore dialogue to default values"); //$NON-NLS-1$
      toolkit.adapt(btnDefault, true, true);
      btnDefault.setText("Restore Default"); //$NON-NLS-1$

      Group grpTargetVddSupply = new Group(comp, SWT.NONE);
      grpTargetVddSupply.setText("Target Vdd Supply"); //$NON-NLS-1$
      RowLayout rl_grpTargetVddSupply = new RowLayout(SWT.VERTICAL);
      rl_grpTargetVddSupply.marginHeight = 3;
      rl_grpTargetVddSupply.spacing = 5;
      rl_grpTargetVddSupply.fill = true;
      grpTargetVddSupply.setLayout(rl_grpTargetVddSupply);
      grpTargetVddSupply.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 2));
      toolkit.adapt(grpTargetVddSupply);
      toolkit.paintBordersFor(grpTargetVddSupply);

      Composite composite = new Composite(grpTargetVddSupply, SWT.NONE);
      toolkit.adapt(composite);
      toolkit.paintBordersFor(composite);
      composite.setLayout(new FillLayout(SWT.HORIZONTAL));

      btnTargetVddOff = new Button(composite, SWT.RADIO);
      btnTargetVddOff.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            setTargetVdd(UsbdmCommon.BDM_TARGET_VDD_OFF);
         }
      });
      btnTargetVddOff.setToolTipText("Do not supply power to the target.  An external target supply is required."); //$NON-NLS-1$
      toolkit.adapt(btnTargetVddOff, true, true);
      btnTargetVddOff.setText("Off"); //$NON-NLS-1$

      btnTargetVdd3V3 = new Button(composite, SWT.RADIO);
      btnTargetVdd3V3.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            setTargetVdd(UsbdmCommon.BDM_TARGET_VDD_3V3);
         }
      });
      btnTargetVdd3V3.setToolTipText("Supply 3.3V to the target through the BDM connection."); //$NON-NLS-1$
      toolkit.adapt(btnTargetVdd3V3, true, true);
      btnTargetVdd3V3.setText("3V3"); //$NON-NLS-1$

      btnTargetVdd5V = new Button(composite, SWT.RADIO);
      btnTargetVdd5V.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            setTargetVdd(UsbdmCommon.BDM_TARGET_VDD_5V);
         }
      });
      btnTargetVdd5V.setToolTipText("Supply 5V to the target through the BDM connection."); //$NON-NLS-1$
      toolkit.adapt(btnTargetVdd5V, true, true);
      btnTargetVdd5V.setText("5V"); //$NON-NLS-1$

      btnCycleTargetVddOnReset = new Button(grpTargetVddSupply, SWT.CHECK);
      btnCycleTargetVddOnReset.setToolTipText("Cycle target supply when resetting."); //$NON-NLS-1$
      toolkit.adapt(btnCycleTargetVddOnReset, true, true);
      btnCycleTargetVddOnReset.setText("Cycle target Vdd on reset"); //$NON-NLS-1$

      btnCycleTargetVddOnConnect = new Button(grpTargetVddSupply, SWT.CHECK);
      btnCycleTargetVddOnConnect.setToolTipText("Cycle target Vdd if having trouble connecting to the target."); //$NON-NLS-1$
      btnCycleTargetVddOnConnect.setText("Cycle target Vdd on connection problems"); //$NON-NLS-1$
      btnCycleTargetVddOnConnect.setBounds(0, 0, 433, 16);
      toolkit.adapt(btnCycleTargetVddOnConnect, true, true);

      btnLeaveTargetPowered = new Button(grpTargetVddSupply, SWT.CHECK);
      btnLeaveTargetPowered.setToolTipText("Leave target powered when leaving the debugger");
      btnLeaveTargetPowered.setText("Leave target powered on exit"); //$NON-NLS-1$
      btnLeaveTargetPowered.setBounds(0, 0, 433, 16);
      toolkit.adapt(btnLeaveTargetPowered, true, true);
   }

   protected void restoreDefaultSettings() {
      if (defaultBdmOptions == null) {
         defaultBdmOptions = new DefaultBdmOptions();
      }
      try {
         // Handle target Vdd control interactions
         int targetVdd = defaultBdmOptions.targetVdd;
         btnTargetVddOff.setSelection(targetVdd == UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnTargetVdd3V3.setSelection(targetVdd == UsbdmCommon.BDM_TARGET_VDD_3V3);
         btnTargetVdd5V.setSelection( targetVdd == UsbdmCommon.BDM_TARGET_VDD_5V);
         btnCycleTargetVddOnConnect.setSelection(defaultBdmOptions.cycleVddOnReset != 0);
         btnCycleTargetVddOnReset.setSelection(defaultBdmOptions.cycleVddOnConnect != 0);
         btnLeaveTargetPowered.setSelection(defaultBdmOptions.leaveTargetPowered != 0);
         btnCycleTargetVddOnConnect.setEnabled(targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnCycleTargetVddOnReset.setEnabled(   targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnLeaveTargetPowered.setEnabled(      targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnUseDebugBuild.setSelection(false);
//         lblDeviceNote.setText("Note: "+deviceNote);
      } catch (Exception e) {
         e.printStackTrace();
      }
   }

   public void loadSettings(ILaunchConfiguration iLaunchConfiguration) {

//      System.err.println("UsbdmConnectionPanel.loadSettings()");
      launchConfiguration = iLaunchConfiguration;
      try {
         String preferredBDM = iLaunchConfiguration.getAttribute(attrib(UsbdmCommon.KeyDefaultBdmSerialNumber),    (String)null);
         if (preferredBDM != null)
            populateBdmChoices(preferredBDM, false);

         // Handle target Vdd control interactions
         int targetVdd = getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeySetTargetVdd), defaultBdmOptions.targetVdd);
         btnTargetVddOff.setSelection(targetVdd == UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnTargetVdd3V3.setSelection(targetVdd == UsbdmCommon.BDM_TARGET_VDD_3V3);
         btnTargetVdd5V.setSelection( targetVdd == UsbdmCommon.BDM_TARGET_VDD_5V);
         btnCycleTargetVddOnReset.setSelection(getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyCycleTargetVddOnReset),   defaultBdmOptions.cycleVddOnReset) != 0);
         btnCycleTargetVddOnConnect.setSelection(   getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyCycleTargetVddonConnect), defaultBdmOptions.cycleVddOnConnect) != 0);
         btnLeaveTargetPowered.setSelection(      getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyLeaveTargetPowered),      defaultBdmOptions.leaveTargetPowered) != 0);
         btnCycleTargetVddOnConnect.setEnabled(targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnCycleTargetVddOnReset.setEnabled(  targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
         btnLeaveTargetPowered.setEnabled(     targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
//         lblDeviceNote.setText("Note: "+deviceNote);
      } catch (CoreException e) {
         e.printStackTrace();
      }
   }

   public void saveSettings(ILaunchConfigurationWorkingCopy paramILaunchConfigurationWorkingCopy) throws PrefException {
      //      System.err.println("UsbdmConnectionPanel.saveSettings()");
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyDefaultBdmSerialNumber),   comboSelectBDM.getText());

      int targetVdd = UsbdmCommon.BDM_TARGET_VDD_OFF;
      if (btnTargetVdd3V3.getSelection())
         targetVdd = UsbdmCommon.BDM_TARGET_VDD_3V3;
      else if (btnTargetVdd5V.getSelection())
         targetVdd = UsbdmCommon.BDM_TARGET_VDD_5V;
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeySetTargetVdd),            String.format("%d", targetVdd));
      if (targetVdd == UsbdmCommon.BDM_TARGET_VDD_OFF) {
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyCycleTargetVddonConnect), "0");
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyCycleTargetVddOnReset),   "0");
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyLeaveTargetPowered),      "0");
      }
      else {
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyCycleTargetVddonConnect), btnCycleTargetVddOnConnect.getSelection()?"1":"0");
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyCycleTargetVddOnReset),   btnCycleTargetVddOnReset.getSelection()?"1":"0");
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyLeaveTargetPowered),      btnLeaveTargetPowered.getSelection()?"1":"0");
      }
   }

   protected void addSettingsChangedListeners() {
      if (fListener != null) {
         comboSelectBDM.addModifyListener(fListener.getModifyListener());
         btnDefault.addSelectionListener(fListener.getSelectionListener());

         btnTargetVddOff.addSelectionListener(fListener.getSelectionListener());
         btnTargetVdd3V3.addSelectionListener(fListener.getSelectionListener());
         btnTargetVdd5V.addSelectionListener(fListener.getSelectionListener());
         btnCycleTargetVddOnConnect.addSelectionListener(fListener.getSelectionListener());
         btnCycleTargetVddOnReset.addSelectionListener(fListener.getSelectionListener());
         btnLeaveTargetPowered.addSelectionListener(fListener.getSelectionListener());
      }
   }

   /**
    * @param paramILaunchConfiguration  Configuration object to load from
    * @param key                        Key to use for retrieval
    * @param defaultValue               Default value if not found
    * @return                           The value (as an int) or the default if unfound    
    * @throws CoreException
    */
   protected int getIntAttribute(ILaunchConfiguration paramILaunchConfiguration, String key, int defaultValue)
   throws CoreException {
      String sValue = paramILaunchConfiguration.getAttribute(key,  String.format("%d", defaultValue));
      return Integer.decode(sValue);
   }

   /**
    * @param  suffix the last (rightmost) element to use as an attribute key
    * 
    * @return complete attribute key
    */
   protected String attrib(String suffix) {
      return attributeKey+suffix;
   }

   protected void updateBdmDescription() {
      if (lblBDMInformation != null) {
         int index = comboSelectBDM.getSelectionIndex();
         if (index >= 0) {
            String deviceDescription = deviceList.get(index).deviceDescription;
            lblBDMInformation.setText(deviceDescription);
         }
      }
   }

   /**
    * @param previousDevice A String representing the serial number of a previously selected device.
    *                       This will be made the currently selected device (even if not connected).
    * @param scanForBdms    If true a scan is made for currently connected BDMs
    */
   protected void populateBdmChoices(String previousDevice, boolean scanForBdms) {
      //    System.err.println("populateBdmChoices("+previousDevice+") :");
      String preferredDevice;
      final DeviceInfo nullDevice = new DeviceInfo("Generic BDM", "Any connected USBDM", new BdmInformation());

      if (scanForBdms) {
         // scan for connected BDMs
//         System.err.println("populateBdmChoices() - looking for BDMs...");
         deviceList = Usbdm.getDeviceList(this.getShell());
      }
      else {
         // Don't scan for BDMs - use an empty list
         deviceList = new ArrayList<DeviceInfo>(); 
      }
      // Always add a null device
//      System.err.println("populateBdmChoices() - Adding nullDevice");
      deviceList.add(0, nullDevice);

      if ((previousDevice != null) &&
            (!previousDevice.equals(nullDevice.deviceSerialNumber))) {
         // Add dummy device representing previously used device, make preferred
         deviceList.add(1, new DeviceInfo("Previously selected device", previousDevice, new BdmInformation()));
         preferredDevice = previousDevice;
      }
      else {
         // Use currently selected device (if any) as preferred
         preferredDevice = comboSelectBDM.getText();
      }

      //      System.err.println("populateBdmChoices(), preferred = "+preferredDevice);

      // Add devices to combo
      comboSelectBDM.removeAll();
      ListIterator<Usbdm.DeviceInfo> it = deviceList.listIterator();
      while (it.hasNext()) {
         DeviceInfo di = it.next();
         //         System.err.println( " BDM = " + di.toString());
         comboSelectBDM.add(di.deviceSerialNumber);
      }

      int index = comboSelectBDM.indexOf(preferredDevice);
      if (index >= 0)
         comboSelectBDM.select(index);
      else
         comboSelectBDM.select(0);
      updateBdmDescription();
   }

   // Interface: com.freescale.cdt.debug.cw.mcu.common.publicintf.ICWGdiInitializationData.GetConfigFile 
   public String GetConfigFile() { 
      return ""; 
   }

   /**
    * @see com.freescale.cdt.debug.cw.mcu.common.publicintf.ICWGdiInitializationData#GetGdiLibrary()
    * 
    * @return Name of GDI DLL e.g. "USBDM_GDI_HCS08.dll"
    */
   @Override
   public String GetGdiLibrary() {
      if ((btnUseDebugBuild != null) &&(btnUseDebugBuild.getSelection())) {
         //         System.err.println("GetGdiLibrary() - Using debug DLL : " + gdiDebugDllName);
         return gdiDebugDllName;
      }
      else {
         //         System.err.println("GetGdiLibrary() - Using non-debug DLL : " + gdiDllName);
         return gdiDllName;
      }
   }                                             

   // Interface: com.freescale.cdt.debug.cw.mcu.common.publicintf.ICWGdiInitializationData.GetConfigFile 
   public String[] GetGdiOpenCmdLineArgs() {     
      return new String[0];                       
   }

   protected void enableTrim(boolean enabled) {
//      if (true) {
//         txtNVTRIMAddressAdapter.setHexValue(0);
//         txtTrimFrequencyAdapter.setDoubleValue(0.0);
//         grpClockTrim.setEnabled(false);
//         btnTrimTargetClock.setEnabled(false);
//         enabled = false; 
//      }
//      else {
//         grpClockTrim.setEnabled(true);
//         btnTrimTargetClock.setEnabled(true);
//      }
      btnTrimTargetClock.setSelection(enabled);
      lblNvtrimAddress.setEnabled(enabled);
      lblHex.setEnabled(enabled);
      lblKhz.setEnabled(enabled);
      lblTrimFrequency.setEnabled(enabled);
      txtNVTRIMAddress.setEnabled(enabled);
      txtTrimFrequency.setEnabled(enabled);
      txtNVTRIMAddress.setText("default");
      txtTrimFrequency.setText("default");
   }

   protected void setTargetVdd(int targetVdd) {
      btnCycleTargetVddOnConnect.setEnabled(targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
      btnCycleTargetVddOnReset.setEnabled(targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
      btnLeaveTargetPowered.setEnabled(targetVdd != UsbdmCommon.BDM_TARGET_VDD_OFF);
   }

}