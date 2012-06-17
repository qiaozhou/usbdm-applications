package net.sourceforge.usbdm.connections.usbdm;

import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.widgets.Text;

   class HexTextAdapter extends Object {
      Text textField;
      String formatString = "%X";
      
      Text getObject() {
         return textField;
      }
      
      HexTextAdapter(Text textField) {
         this.textField = textField;
//         formatString = new String("%5X");
         textField.setTextLimit(4);
         textField.addVerifyListener(new HexVerifyListener());
      }
      
      HexTextAdapter(Text textField, int width) {
         this.textField = textField;
         textField.setTextLimit(width);
//         formatString = String.format("%%%dX", width);
         textField.addVerifyListener(new HexVerifyListener());
      }
      
      private class HexVerifyListener implements VerifyListener {
         public void verifyText(VerifyEvent e) {
            e.text = e.text.toUpperCase(); // Force display as upper-case
            String string = e.text;
            char[] chars = new char[string.length()];
            string.getChars(0, chars.length, chars, 0);
            for (int i = 0; i < chars.length; i++) {
               if (('0' > chars[i] || chars[i] > '9') &&
                     ('A' > chars[i] || chars[i] > 'F')) {
                  e.doit = false;
                  return;
               }
            }
         }
      };
      
      public int getHexValue() {
         int value = 0;
         try {
            value = Integer.parseInt(textField.getText(), 16);
         } catch (NumberFormatException e) {
            // Quietly default to 0
         }
         return value;
      }
      
      public void setHexValue(int value) {
         if (value == 0)
            textField.setText("default");
         textField.setText(String.format(formatString, value));
      }
   }
   
