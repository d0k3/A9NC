# A9NC - ARM9 Netload Companion
A simple companion app to receive and run ARM9 payloads via CIA & A9LH

To send a file, use:
3dslink -a [IP] [payload name]

3dslink is included in devKitPro and also available [from here](http://davejmurphy.com/3dslink/).

The file will be written to a place of your choice on your SD card. At the moment, most places to choose from are in in the [Luma3DS](https://github.com/AuroraWright/Luma3DS) payloads folder. __Developers please contact me if you need A9NC to write to a different location__. 

Requires devKitPro, ctrulib and zlib to compile. For easy zlib installation [go here](https://github.com/devkitPro/3ds_portlibs).
