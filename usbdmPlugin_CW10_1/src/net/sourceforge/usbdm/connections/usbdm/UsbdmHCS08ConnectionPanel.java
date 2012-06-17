package net.sourceforge.usbdm.connections.usbdm;

import org.eclipse.debug.core.ILaunchConfiguration;
import org.eclipse.debug.core.ILaunchConfigurationWorkingCopy;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsListener;
import com.freescale.cdt.debug.cw.core.ui.settings.PrefException;
import com.swtdesigner.SWTResourceManager;

/**
 * @author podonoghue
 *
 */
public class UsbdmHCS08ConnectionPanel 
extends UsbdmConnectionPanel
{
   private Button btnAutomaticallyReconnect;
   private Button btnDriveReset;
   private Button btnMaskInterruptsWhenStepping;

   private Button btnBDMClockDefault;
   private Button btnBDMClockBus;
   private Button btnBDMClockAlt;

//   private Label lblGdiVersion;
   private Label lblTargetId;

   /**
    * Dummy constructor for WindowBuilder Pro.
    * 
    * @param parent
    * @param style
    */
   public UsbdmHCS08ConnectionPanel(Composite parent, int style) {
      super(parent, style);
      System.err.println("createComposite()");

      init();
      //      create();
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
   public UsbdmHCS08ConnectionPanel(ISettingsListener listener, 
         Composite         parent,
         int               swtstyle, 
         String            protocolPlugin,  // "CF - GDI", "HC08 GDI" etc
         String            connectionTypeId) {

      super(listener, parent, swtstyle, protocolPlugin, connectionTypeId);
//      System.err.println("UsbdmHCS08ConnectionPanel::UsbdmHCS08ConnectionPanel(protocolPlugin="+protocolPlugin+", connectionTypeId = "+connectionTypeId+")");

      init();
   }

   private void init() {
      deviceNameId    = UsbdmCommon.HCS08_DeviceNameAttributeKey;
      gdiDllName      = UsbdmCommon.HCS08_GdiWrapperLib;
      gdiDebugDllName = UsbdmCommon.HCS08_DebugGdiWrapperLib;
   }

   public void create() {
      createContents(this);
      addSettingsChangedListeners();
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#loadSettings(org.eclipse.debug.core.ILaunchConfiguration)
    */
   protected void restoreDefaultSettings() {
      super.restoreDefaultSettings();
      try {
         btnMaskInterruptsWhenStepping.setSelection(defaultBdmOptions.maskinterrupts != 0);

         btnAutomaticallyReconnect.setSelection(defaultBdmOptions.autoReconnect != 0);
         btnDriveReset.setSelection(defaultBdmOptions.useResetSignal != 0);

         int bdmClockSelect = defaultBdmOptions.useAltBDMClock;
         btnBDMClockDefault.setSelection(bdmClockSelect == UsbdmCommon.BDM_CLK_DEFAULT);
         btnBDMClockBus.setSelection(    bdmClockSelect == UsbdmCommon.BDM_CLK_NORMAL);
         btnBDMClockAlt.setSelection(    bdmClockSelect == UsbdmCommon.BDM_CLK_ALT);

         int clockTrimFrequency = defaultBdmOptions.clockTrimFrequency;
         txtTrimFrequencyAdapter.setDoubleValue(clockTrimFrequency/1000.0);
         txtNVTRIMAddressAdapter.setHexValue(defaultBdmOptions.clockTrimNVAddress);

         enableTrim(defaultBdmOptions.doClockTrim);
      } catch (Exception e) {
         e.printStackTrace();
      }
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#loadSettings(org.eclipse.debug.core.ILaunchConfiguration)
    */
   @Override
   public void loadSettings(ILaunchConfiguration iLaunchConfiguration) {
//      System.err.println("UsbdmHCS08ConnectionPanel.loadSettings()");

      launchConfiguration = iLaunchConfiguration;

      super.loadSettings(iLaunchConfiguration);

      try {
         btnMaskInterruptsWhenStepping.setSelection(getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyMaskInterrupt),          defaultBdmOptions.maskinterrupts) != 0);

         btnAutomaticallyReconnect.setSelection(    getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyAutomaticReconnect),      defaultBdmOptions.autoReconnect) != 0);
         btnDriveReset.setSelection(                getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyUseResetSignal),          defaultBdmOptions.useResetSignal) != 0);

         int bdmClockSelect = getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyUseAltBDMClock),          defaultBdmOptions.useAltBDMClock);
         btnBDMClockDefault.setSelection(bdmClockSelect == UsbdmCommon.BDM_CLK_DEFAULT);
         btnBDMClockBus.setSelection(    bdmClockSelect == UsbdmCommon.BDM_CLK_NORMAL);
         btnBDMClockAlt.setSelection(    bdmClockSelect == UsbdmCommon.BDM_CLK_ALT);

         int clockTrimFrequency = getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyClockTrimFrequency),    defaultBdmOptions.clockTrimFrequency);
         txtTrimFrequencyAdapter.setDoubleValue(clockTrimFrequency/1000.0);
         int clockTrimNVAddress = getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyClockTrimNVAddress),    defaultBdmOptions.clockTrimNVAddress);
         txtNVTRIMAddressAdapter.setHexValue(clockTrimNVAddress);

         enableTrim(getIntAttribute(iLaunchConfiguration, attrib(UsbdmCommon.KeyTrimTargetClock), (defaultBdmOptions.clockTrimFrequency != 0)?1:0) != 0);

         btnUseDebugBuild.setSelection(iLaunchConfiguration.getAttribute(attrib(UsbdmCommon.KeyUseDebugBuild), "false").equalsIgnoreCase("true"));
      } catch (Exception e) {
         e.printStackTrace();
      }
//      lblGdiVersion.setText(connectionTypeId);
      lblTargetId.setText(GetGdiLibrary());
   }

   /* (non-Javadoc)
    * @see com.freescale.cdt.debug.cw.core.ui.publicintf.ISettingsPersistence#saveSettings(org.eclipse.debug.core.ILaunchConfigurationWorkingCopy)
    */
   @Override
   public void saveSettings(ILaunchConfigurationWorkingCopy paramILaunchConfigurationWorkingCopy) throws PrefException {
      //      System.err.println("UsbdmHCS08ConnectionPanel.saveSettings()");

      super.saveSettings(paramILaunchConfigurationWorkingCopy);

      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyMaskInterrupt),          btnMaskInterruptsWhenStepping.getSelection()?"1":"0");

      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyAutomaticReconnect),      btnAutomaticallyReconnect.getSelection()?"1":"0");
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseResetSignal),          btnDriveReset.getSelection()?"1":"0");

      int bdmClock = UsbdmCommon.BDM_CLK_DEFAULT;
      if (btnBDMClockBus.getSelection())
         bdmClock = UsbdmCommon.BDM_CLK_NORMAL;
      else if (btnBDMClockAlt.getSelection())
         bdmClock = UsbdmCommon.BDM_CLK_ALT;
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseAltBDMClock),          String.format("%d", bdmClock));

      if (btnTrimTargetClock.getSelection()) {
         // Trimming clock
         String nvAddress = "0";
         String trimfreq  = "0";
         try {
            trimfreq  = String.format("%d",(int)(txtTrimFrequencyAdapter.getDoubleValue()*1000));
            nvAddress = String.format("%d",txtNVTRIMAddressAdapter.getHexValue());
         } catch (Exception e) {
            //e.printStackTrace();
         }
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyTrimTargetClock),     "1");
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyClockTrimFrequency),  trimfreq);
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyClockTrimNVAddress),  nvAddress);
      }
      else {
         // Not trimming
         paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyTrimTargetClock),     "0");
         paramILaunchConfigurationWorkingCopy.removeAttribute(attrib(UsbdmCommon.KeyClockTrimFrequency));
         paramILaunchConfigurationWorkingCopy.removeAttribute(attrib(UsbdmCommon.KeyClockTrimNVAddress));
      }
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUseDebugBuild),      btnUseDebugBuild.getSelection()?"true":"false");

      // Unused on this target
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyUsePSTSignals),           (String)null);
      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyConnectionSpeed),         (String)null);
//      paramILaunchConfigurationWorkingCopy.setAttribute(attrib(UsbdmCommon.KeyGuessSpeedIfNoSYNC),      (String)null);
   }

   @Override
   protected void addSettingsChangedListeners() {
      super.addSettingsChangedListeners();
      if (fListener != null) {
         btnAutomaticallyReconnect.addSelectionListener(fListener.getSelectionListener());
         btnBDMClockAlt.addSelectionListener(fListener.getSelectionListener());
         btnBDMClockBus.addSelectionListener(fListener.getSelectionListener());
         btnBDMClockDefault.addSelectionListener(fListener.getSelectionListener());
         btnDriveReset.addSelectionListener(fListener.getSelectionListener());
         btnMaskInterruptsWhenStepping.addSelectionListener(fListener.getSelectionListener());
         btnTrimTargetClock.addSelectionListener(fListener.getSelectionListener());
         txtNVTRIMAddress.addModifyListener(fListener.getModifyListener());
         txtTrimFrequency.addModifyListener(fListener.getModifyListener());
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
      grpConnectionControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 1));
      grpConnectionControl.setText("Connection Control"); //$NON-NLS-1$
      toolkit.adapt(grpConnectionControl);
      toolkit.paintBordersFor(grpConnectionControl);

      btnAutomaticallyReconnect = new Button(grpConnectionControl, SWT.CHECK);
      btnAutomaticallyReconnect.setToolTipText("Automatically re-sync with the target whenever target state is polled."); //$NON-NLS-1$
      toolkit.adapt(btnAutomaticallyReconnect, true, true);
      btnAutomaticallyReconnect.setText("Automatically re-connect"); //$NON-NLS-1$

      btnDriveReset = new Button(grpConnectionControl, SWT.CHECK);
      btnDriveReset.setToolTipText("Drive target reset pin when resetting the target."); //$NON-NLS-1$
      btnDriveReset.setText("Drive RESET pin"); //$NON-NLS-1$
      btnDriveReset.setBounds(0, 0, 140, 16);
      toolkit.adapt(btnDriveReset, true, true);

      grpClockTrim = new Group(this, SWT.NONE);
      grpClockTrim.setText("Internal Clock Trim"); //$NON-NLS-1$
      grpClockTrim.setLayout(new GridLayout(2, false));
      grpClockTrim.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 2));
      toolkit.adapt(grpClockTrim);
      toolkit.paintBordersFor(grpClockTrim);

      btnTrimTargetClock = new Button(grpClockTrim, SWT.CHECK);
      btnTrimTargetClock.setText("Enable Clock Trim");
      btnTrimTargetClock.addSelectionListener(new SelectionAdapter() {
         public void widgetSelected(SelectionEvent e) {
            enableTrim(((Button)e.getSource()).getSelection());
         }
      });
      new Label(grpClockTrim, SWT.NONE);

      lblTrimFrequency = new Label(grpClockTrim, SWT.NONE);
      toolkit.adapt(lblTrimFrequency, true, true);
      lblTrimFrequency.setText("Trim Frequency"); //$NON-NLS-1$
      new Label(grpClockTrim, SWT.NONE);

      btnTrimTargetClock.setToolTipText("Enable trimming of target internal clock source."); //$NON-NLS-1$
      toolkit.adapt(btnTrimTargetClock, true, true);

      txtTrimFrequency = new Text(grpClockTrim, SWT.BORDER);
      txtTrimFrequencyAdapter = new DoubleTextAdapter(txtTrimFrequency);
      txtTrimFrequency.setTextLimit(7);
      txtTrimFrequencyAdapter.setDoubleValue(0.0);
      txtTrimFrequency.setToolTipText(""); //$NON-NLS-1$
      txtTrimFrequency.setBackground(SWTResourceManager.getColor(255, 255, 255));
      GridData gd_txtTrimFrequency = new GridData(SWT.LEFT, SWT.CENTER, false, false, 1, 1);
      gd_txtTrimFrequency.widthHint = 65;
      gd_txtTrimFrequency.minimumWidth = 65;
      txtTrimFrequency.setLayoutData(gd_txtTrimFrequency);
      toolkit.adapt(txtTrimFrequency, true, true);

      lblKhz = new Label(grpClockTrim, SWT.NONE);
      lblKhz.setToolTipText(
            "The frequency to trim the internal clock source to.\r\n" +
      		"Note this is NOT the bus clock frequency.\r\n" +
      		"Zero indicates use chip default value"); //$NON-NLS-1$
      toolkit.adapt(lblKhz, true, true);
      lblKhz.setText("kHz"); //$NON-NLS-1$

      lblNvtrimAddress = new Label(grpClockTrim, SWT.NONE);
      toolkit.adapt(lblNvtrimAddress, true, true);
      lblNvtrimAddress.setText("NVTRIM Address"); //$NON-NLS-1$
      new Label(grpClockTrim, SWT.NONE);

      txtNVTRIMAddress = new Text(grpClockTrim, SWT.BORDER);
      txtNVTRIMAddressAdapter = new HexTextAdapter(txtNVTRIMAddress);
      txtNVTRIMAddressAdapter.setHexValue(0);
      GridData gd_txtNVTRIMAddress = new GridData(SWT.LEFT, SWT.CENTER, false, false, 1, 1);
      gd_txtNVTRIMAddress.widthHint = 65;
      gd_txtNVTRIMAddress.minimumWidth = 65;
      txtNVTRIMAddress.setLayoutData(gd_txtNVTRIMAddress);
      toolkit.adapt(txtNVTRIMAddress, true, true);

      lblHex = new Label(grpClockTrim, SWT.NONE);
      lblHex.setToolTipText(
            "Address of non-volatile memory location to write the trim value to.\r\n" +
            "Zero indicates use chip default value"); //$NON-NLS-1$
      toolkit.adapt(lblHex, true, true);
      lblHex.setText("hex"); //$NON-NLS-1$

      Group grpBdmClockSelect = new Group(this, SWT.NONE);
      grpBdmClockSelect.setText("BDM Clock Select"); //$NON-NLS-1$
      RowLayout rl_grpBdmClockSelect = new RowLayout(SWT.HORIZONTAL);
      rl_grpBdmClockSelect.marginHeight = 3;
      rl_grpBdmClockSelect.marginBottom = 0;
      grpBdmClockSelect.setLayout(rl_grpBdmClockSelect);
      grpBdmClockSelect.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 1));
      toolkit.adapt(grpBdmClockSelect);
      toolkit.paintBordersFor(grpBdmClockSelect);

      btnBDMClockDefault = new Button(grpBdmClockSelect, SWT.RADIO);
      btnBDMClockDefault.setToolTipText("Use default BDM clock."); //$NON-NLS-1$
      toolkit.adapt(btnBDMClockDefault, true, true);
      btnBDMClockDefault.setText("Default"); //$NON-NLS-1$

      btnBDMClockBus = new Button(grpBdmClockSelect, SWT.RADIO);
      btnBDMClockBus.setToolTipText("Force use of target Bus Clock as BDM clock."); //$NON-NLS-1$
      toolkit.adapt(btnBDMClockBus, true, true);
      btnBDMClockBus.setText("Bus Clock/2"); //$NON-NLS-1$

      btnBDMClockAlt = new Button(grpBdmClockSelect, SWT.RADIO);
      btnBDMClockAlt.setToolTipText("Force use of alternative  BDM clock (derivative specific source)."); //$NON-NLS-1$
      toolkit.adapt(btnBDMClockAlt, true, true);
      btnBDMClockAlt.setText("Alt"); //$NON-NLS-1$

      Group grpMiscellaneous = new Group(this, SWT.NONE);
      grpMiscellaneous.setText("Miscellaneous"); //$NON-NLS-1$
      RowLayout rl_grpMiscellaneous = new RowLayout(SWT.HORIZONTAL);
      rl_grpMiscellaneous.marginHeight = 3;
      grpMiscellaneous.setLayout(rl_grpMiscellaneous);
      grpMiscellaneous.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 1));
      toolkit.adapt(grpMiscellaneous);
      toolkit.paintBordersFor(grpMiscellaneous);

      btnMaskInterruptsWhenStepping = new Button(grpMiscellaneous, SWT.CHECK);
      btnMaskInterruptsWhenStepping.setToolTipText("The I bit in the CCR will be dynamically modified to mask interrupts when single-stepping.\n" +
      "In most cases interrupts will be ignored."); //$NON-NLS-1$
      btnMaskInterruptsWhenStepping.setText("Mask Interrupts when stepping"); //$NON-NLS-1$
      toolkit.adapt(btnMaskInterruptsWhenStepping, true, true);

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
      
      Composite composite_2 = new Composite(this, SWT.NONE);
      composite_2.setLayout(new FillLayout(SWT.VERTICAL));
      GridData gd_composite_2 = new GridData(SWT.FILL, SWT.TOP, false, false, 1, 1);
      gd_composite_2.horizontalIndent = 5;
      gd_composite_2.verticalIndent = 5;
      composite_2.setLayoutData(gd_composite_2);
      toolkit.adapt(composite_2);
      toolkit.paintBordersFor(composite_2);

//      lblGdiVersion = new Label(composite_2, SWT.RIGHT);
//      toolkit.adapt(lblGdiVersion, true, true);
//      lblGdiVersion.setText("GDI Version");

      lblTargetId = new Label(composite_2, SWT.RIGHT);
      toolkit.adapt(lblTargetId, true, true);
      lblTargetId.setText("Target ID");
      
      super.appendContents(this);
   }
}
