# EmergencyExit

Games refusing to close? Can't Alt+Tab or Alt+F4 out of it? Only option is to reset your computer? Just take the EmergencyExit!

No need to reset your machine and possibly lose any unsaved work, now you can just kill any unresponsive program with a simple keyboard shortcut: CTRL + PauseBreak.

EmergencyExit was born of a need for an easier way to kill broken programs, because for some reason Windows doesn't have a feature like this yet.
In the past I would try and run taskkill.exe via the start menu, but obviously that only works if the start menu is still functional, and also requires you to know the exes' filename in advance.

But no more! With EmergencyExit you just hit that key combination and you can be back to your desktop in no time.

Setup
---
EmergencyExit was designed to be a portable program, and so only requires the VC++ 2013 redist to be installed to use it. If you play PC games you'll likely already have this.

Just extract the exe somewhere and run it, a small red power icon should appear in your system tray (near where the time is) confirming that it's active.

Usage
---
EmergencyExit runs in the background - the only indication that it's active is the red power icon in your system tray.

Just hit CTRL + PauseBreak at any time to kill the currently active process.

Each time you run the program, unless already set it'll ask if you want to run it on startup - I recommend this as you can then use EmergencyExit at any time.  
You can also right click the red power icon and click the "Run on startup" choice to enable/disable running on startup.

Programs run as admin / other users
---
Note that unless you run it as administrator EmergencyExit will only be able to kill processes run under your user account, meaning that it won't be able to kill other programs running as admin or any other users.

If you need that ability just right click the exe and choose "Run as administrator". I haven't looked into how to make it run as admin from startup yet, but that probably isn't that hard to setup.

License
---
EmergencyExit is licensed under GPLv3.