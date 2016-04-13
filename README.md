# A9LH Netload Companion
A simple companion app to receive and run ARM9 payloads for A9LH

To send a file, use:
3dslink -a [IP] [payload name]

3dslink is included in devKitPro and also available [from here](http://davejmurphy.com/3dslink/). A9LH Netload Companion will automatically reboot on a succesfull receive. [BootCTR9](https://github.com/hartmannaf/BootCtr9/releases) can easily be set up to run the received file at a button press.

Requires devKitPro, ctrulib and zlib to compile. For easy zlib installation [go here](https://github.com/devkitPro/3ds_portlibs).
