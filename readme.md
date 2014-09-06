PeelSolve Slate Version 0.1

GNU LESSER GENERAL PUBLIC LICENSE

This software will allow you to type some text in to a small window on one computer,
and that text can be displayed in full screen on with a black background.  This
can be used as part of a system for slating video clips, e.g. with a video mixer
or switcher.


== Keys ==

Alt + O = Options
Alt + Q = Quit
Alt + Space = Toggle Full Screen
Alt + Enter = Toggle Full Screen
Escape = Exit Fullscreen


== Server Instructions ==

The server will display in full screen with white text on a black background.  It
receives data over the network using a simple udp protocol.  You will need to specify
the port that the server listens on.

1. Start Application
2. Check the "Server" option
3. Set the server port to listen on, or use the default
4. Set the Big font size as needed
5. Click ok
6. Type some text in to test the display
7. Press Alt+Enter to switch to full screen


== Client Instructions == 

The client displays as a small text interface of three lines that you can type in.
Any text you type will be sent to the server for display after each keystroke

1. Start Application
2. Check the "Client" option
3. Set the server IP and port
4. Click ok
5. Type some text. 

The text you type should be repeated on the server as you type it.


For more information, email al - at - mocap - dot - ca
