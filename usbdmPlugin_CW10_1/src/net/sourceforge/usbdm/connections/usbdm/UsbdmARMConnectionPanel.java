package net.sourceforge.usbdm.connections.usbdm;

//import org.eclipse.debug.core.DebugPlugin;
import org.eclipse.debug.core.ILaunchConfiguration;
import org.eclipse.debug.core.ILaunchConfigurationWorkingCopy;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;

import com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsListener;
import com.freescale.cdt.debug.cw.core.ui.settings.PrefException;

import net.sourceforge.usbdm.connections.usbdm.UsbdmCommon;
import net.sourceforge.usbdm.connections.usbdm.JTAGInterfaceData.ClockSpeed;
/**
 * @author podonoghue
 *
 */
public class UsbdmARMConnectionPanel 
extends UsbdmConnectionPanel
{
//   private CFVxDeviceData deviceData;

   private Button btnAutomaticallyReconnect;
   private Combo  comboConnectionSpeed;
   private Button btnUsePstSignals;
   
//   private Label lblGdiVersion;
   private Label lblTargetId;
//   private String configFileName;
//   private DeviceDatabase<CFVxDeviceData> deviceDatabase = null;

   /**
    * Dummy constructor for WindowBuilder Pro.
    * 
    * @param parent
    * @param style
    */
   public UsbdmARMConnectionPanel(Composite parent, int style) {
      super(parent, style);

      init();
      //      create();
      //      deviceName = "XX";
      defaultBdmOptions = new DefaultBdmOptions();
   }

   /**
    * Actual constructor used by factory
    * 
    * @param listener
    * @param parent
    * @param swtstyle
    * @param protocolPlugin    Indicates the connection type e.g. "HCS08 GDI"
    * @param connectionTypeId  A long string indicating the architecture??
    */
   public UsbdmARMConnectionPanel(ISettingsListener listener, 
         Composite         parent,
         int               swtstyle, 
         String            protocolPlugin,  // "CF - GDI", "HC08 GDI" etc
         String            connectionTypeId) {

      super(listener, parent, swtstyle, protocolPlugin, connectionTypeId);
//      System.err.println("UsbdmCFVxConnectionPanel::UsbdmCFVxConnectionPanel(protocolPlugin="+protocolPlugin+", connectionTypeId = "+connectionTypeId+")");

      init();
   }

   private void init() {
//      configFileName  = null;
      deviceNameId    = UsbdmCommon.ARM_DeviceNameAttributeKey;
      gdiDllName      = UsbdmCommon.ARM_GdiWrapperLib;
      gdiDebugDllName = UsbdmCommon.ARM_DebugGdiWrapperLib;
   }

   public void create() {
      createContents(this);
      addSettingsChangedListeners();
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#loadSettings(org.eclipse.debug.core.ILaunchConfiguration)
    */
   protected void restoreDefaultSettings() {
      if (defaultBdmOptions == null) {
         defaultBdmOptions = new DefaultBdmOptions();
      }
      super.restoreDefaultSettings();
      try {
         btnAutomaticallyReconnect.setSelection(defaultBdmOptions.autoReconnect != 0);
         comboConnectionSpeed.select(JTAGInterfaceData.ClockSpeed.defFrequency.ordinal());
         btnUsePstSignals.setSelection(defaultBdmOptions.usePSTSignals != 0);
      } catch (Exception e) {
         e.printStackTrace();
      }
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#loadSettings(org.eclipse.debug.core.ILaunchConfiguration)
    */
   @Override
   public void loadSettings(ILaunchConfiguration iLaunchConfiguration) {
//      System.err.println("UsbdmCFVxConnectionPanel.loadSettings()");

      launchConfiguration = iLaunchConfiguration;

      super.loadSettings(iLaunchConfiguration);

      try {
         btnAutomaticallyReconnect.setSelection(    getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyAutomaticReconnect),      defaultBdmOptions.autoReconnect) != 0);
         btnUseDebugBuild.setSelection(getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyUseDebugBuild),   0) != 0);
         int connectionFrequency = getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyConnectionSpeed), ClockSpeed.defFrequency.toFrequency());
//         System.err.println("UsbdmCFVxConnectionPanel.loadSettings() - connectionFrequency = "+connectionFrequency);
         ClockSpeed clockSpeed = JTAGInterfaceData.ClockSpeed.lookup(connectionFrequency);
         if (clockSpeed == null) {
            // Not recognised - use default
//            System.err.println("UsbdmCFVxConnectionPanel.loadSettings() - using default");
            clockSpeed = ClockSpeed.defFrequency;
         }
//         System.err.println("UsbdmCFVxConnectionPanel.loadSettings() - ConnectionSpeed = "+clockSpeed.toFrequency());
         comboConnectionSpeed.select(clockSpeed.ordinal());
      } catch (Exception e) {
         e.printStackTrace();
      }
      lblTargetId.setText(GetGdiLibrary());
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#saveSettings(org.eclipse.debug.core.ILaunchConfigurationWorkingCopy)
    */
   @Override
   public void saveSettings(ILaunchConfigurationWorkingCopy paramILaunchConfigurationWorkingCopy) throws PrefException {
//      System.err.println("UsbdmCFVxConnectionPanel.saveSettings()");

      super.saveSettings(paramILaunchConfigurationWorkingCopy);
      
      int connectionSpeed = JTAGInterfaceData.ClockSpeed.defFrequency.toFrequency();
      int index = comboConnectionSpeed.getSelectionIndex();
      if (index >= 0) {
         connectionSpeed = JTAGInterfaceData.ClockSpeed.values()[index].toFrequency();
      }
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyConnectionSpeed),    connectionSpeed);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyAutomaticReconnect), btnAutomaticallyReconnect.getSelection()?"1":"0");
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseDebugBuild),      btnUseDebugBuild.getSelection()?"1":"0");

      // Unused on this target
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyClockTrimNVAddress),      (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyClockTrimFrequency),      (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseResetSignal),          (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyTrimTargetClock),         (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseAltBDMClock),          (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyConnectionSpeed),         (String)null);
//    paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyGuessSpeedIfNoSYNC),      (String)null);
   }

   @Override
   protected void addSettingsChangedListeners() {
      super.addSettingsChangedListeners();
      if (fListener != null) {
         btnAutomaticallyReconnect.addSelectionListener(fListener.getSelectionListener());
         comboConnectionSpeed.addSelectionListener(fListener.getSelectionListener());
         btnUseDebugBuild.addSelectionListener(fListener.getSelectionListener());
      }
   }

   /**
    * @param comp
    */
   @Override
   protected void createContents(Composite comp) {
      super.createContents(comp);
      
      Group grpConnectionControl = new Group(this, SWT.NONE);
      RowLayout rl_grpConnectionControl = new RowLayout(SWT.VERTICAL);
      rl_grpConnectionControl.marginHeight = 3;
      grpConnectionControl.setLayout(rl_grpConnectionControl);
      grpConnectionControl.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, false, false, 1, 2));
      grpConnectionControl.setText("Connection Control"); //$NON-NLS-1$
      
      btnAutomaticallyReconnect = new Button(grpConnectionControl, SWT.CHECK);
      btnAutomaticallyReconnect.setToolTipText("Automatically re-sync with the target whenever target state is polled.");
      btnAutomaticallyReconnect.setText("Automatically re-connect");
      
      Composite composite_3 = new Composite(grpConnectionControl, SWT.NONE);
      composite_3.setLayout(new RowLayout(SWT.HORIZONTAL));
      
      Label lblConnectionSpeed = new Label(composite_3, SWT.NONE);
      lblConnectionSpeed.setToolTipText("Connection speed to use for JTAG communications.");
      lblConnectionSpeed.setText("Connection\r\nSpeed");
      
      comboConnectionSpeed = new Combo(composite_3, SWT.READ_ONLY);
      comboConnectionSpeed.setItems(JTAGInterfaceData.getConnectionSpeeds());
      comboConnectionSpeed.setToolTipText("");
      comboConnectionSpeed.setLayoutData(new RowData(SWT.DEFAULT, 29));
      comboConnectionSpeed.select(4);

      Composite composite_2 = new Composite(this, SWT.NONE);
      composite_2.setLayout(new FillLayout(SWT.VERTICAL));
      GridData gd_composite_2 = new GridData(SWT.FILL, SWT.TOP, false, false, 1, 1);
      gd_composite_2.horizontalIndent = 5;
      gd_composite_2.verticalIndent = 5;
      composite_2.setLayoutData(gd_composite_2);
      toolkit.adapt(composite_2);
      toolkit.paintBordersFor(composite_2);
      
      new Label(this, SWT.NONE);
      new Label(this, SWT.NONE);

      Group grpDebuggingOptions = new Group(this, SWT.NONE);
      grpDebuggingOptions.setText("Debugging Options");
      grpDebuggingOptions.setLayout(new RowLayout(SWT.HORIZONTAL));
      grpDebuggingOptions.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 1));
      toolkit.adapt(grpDebuggingOptions);
      toolkit.paintBordersFor(grpDebuggingOptions);

      btnUseDebugBuild = new Button(grpDebuggingOptions, SWT.CHECK);
      btnUseDebugBuild.setToolTipText("Used for debugging USBDM drivers - don't enable");
      toolkit.adapt(btnUseDebugBuild, true, true);
      btnUseDebugBuild.setText("Use debug build");
      btnUseDebugBuild.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            lblTargetId.setText(GetGdiLibrary());
         }
      });

//      lblGdiVersion = new Label(composite_2, SWT.RIGHT);
//      toolkit.adapt(lblGdiVersion, true, true);
//      lblGdiVersion.setText("GDI Version");

      lblTargetId = new Label(composite_2, SWT.RIGHT);
      toolkit.adapt(lblTargetId, true, true);
      lblTargetId.setText("Target ID");
      
      super.appendContents(this);
   }
}
