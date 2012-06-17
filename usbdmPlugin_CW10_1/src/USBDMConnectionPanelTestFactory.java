

import net.sourceforge.usbdm.connections.usbdm.UsbdmARMConnectionPanel;
import net.sourceforge.usbdm.connections.usbdm.UsbdmCFV1ConnectionPanel;
import net.sourceforge.usbdm.connections.usbdm.UsbdmCFVxConnectionPanel;
import net.sourceforge.usbdm.connections.usbdm.UsbdmConnectionPanel;
import net.sourceforge.usbdm.connections.usbdm.UsbdmHCS08ConnectionPanel;
import net.sourceforge.usbdm.connections.usbdm.UsbdmRS08ConnectionPanel;

import org.eclipse.swt.widgets.Composite;

import com.freescale.cdt.debug.cw.core.ui.publicintf.AbstractPhysicalConnectionPanel;

@SuppressWarnings("unused")
public final class USBDMConnectionPanelTestFactory {
   
   public static AbstractPhysicalConnectionPanel createComposite(Composite parent, int swtstyle)
   {
//      UsbdmConnectionPanel panel = new UsbdmARMConnectionPanel(parent, swtstyle);
      UsbdmConnectionPanel panel = new UsbdmCFVxConnectionPanel(parent, swtstyle);
//      UsbdmConnectionPanel panel = new UsbdmCFV1ConnectionPanel(parent, swtstyle);
//      UsbdmConnectionPanel panel = new UsbdmRS08ConnectionPanel(parent, swtstyle);
//      UsbdmConnectionPanel panel = new UsbdmHCS08ConnectionPanel(parent, swtstyle);
//      UsbdmConnectionPanel panel = new UsbdmConnectionPanel(parent, swtstyle);
      panel.create();
      return panel;
//      return new UsbdmConnectionPanel(parent, swtstyle);
   }
}
