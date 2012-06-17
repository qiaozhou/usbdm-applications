import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.StatusLineManager;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;



public class TrySWT extends ApplicationWindow {

   /**
    * Create the application window.
    */
   public TrySWT() {
      super(null);
      createActions();
      addToolBar(SWT.FLAT | SWT.WRAP);
      addMenuBar();
      addStatusLine();
   }

   /**
    * Create contents of the application window.
    * @param parent
    */
   @Override
   protected Control createContents(Composite parent) {
      Composite container = USBDMConnectionPanelTestFactory.createComposite(parent, SWT.NONE);
      return container;
   }

   /**
    * Create the actions.
    */
   private void createActions() {
      // Create the actions
   }

   /**
    * Create the menu manager.
    * @return the menu manager
    */
   @Override
   protected MenuManager createMenuManager() {
      MenuManager menuManager = new MenuManager("menu");
      return menuManager;
   }

   /**
    * Create the toolbar manager.
    * @return the toolbar manager
    */
   @Override
   protected ToolBarManager createToolBarManager(int style) {
      ToolBarManager toolBarManager = new ToolBarManager(style);
      return toolBarManager;
   }

   /**
    * Create the status line manager.
    * @return the status line manager
    */
   @Override
   protected StatusLineManager createStatusLineManager() {
      StatusLineManager statusLineManager = new StatusLineManager();
      return statusLineManager;
   }

   /**
    * Launch the application.
    * @param args
    */
   public static void main(String args[]) {
      try {
         TrySWT window = new TrySWT();
         window.setBlockOnOpen(true);
         window.open();
         Display.getCurrent().dispose();
      } catch (Exception e) {
         e.printStackTrace();
      }
   }

   /**
    * Configure the shell.
    * @param newShell
    */
   @Override
   protected void configureShell(Shell newShell) {
      super.configureShell(newShell);
      newShell.setText("New Application");
   }

   /**
    * Return the initial size of the window.
    */
   @Override
   protected Point getInitialSize() {
      return new Point(600, 400);
   }

}
