# ClipWatch
Win32 clipboard extender/history/utility

ClipWatch is a simple utility that enhances the Win32 clipboard (for text content only). 
The app loads when you login but is minimized to the task tray. 
Text that you copy or cut from other applications is automatically added to the clip list in the ClipWatch window. 
When you want to paste something that you previously copied but is no longer in the Windows clipboard, use the hotkey to display ClipWatch and then click on the text item in the clip list (or use up/down arrow keys followed by enter) to put that text back into the Windows clipboard. 
ClipWatch will then hide itself and attempt to paste the restored text into the window that was active before the ClipWatch window was displayed.

## Features
- Manage clipboard text items
- Hotkey to display window: Ctrl+Shift+V
- Pin items in clipboard history so that they are never purged (ctrl+p)
- Search for items (ctrl+f)
- Clippings restored after logout/login
- Single command insert of text to active application (on mouse-click or press of enter) after list displayed via hotkey
- Application loads in task tray, stays out of the way. Display via hotkey. It automatically goes away upon item selection.
- Define size (count of items) of clipboard history via .ini file (pinned items do not count against size)
