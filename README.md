# A9NC - ARM9 Netload Companion
A simple companion app to receive and run ARM9 payloads via CIA & GM9 bootloader

To send a firm, use:
3dslink -a [IP] [payload name]

3dslink is included in devKitARM and also available [from here](http://davejmurphy.com/3dslink/).

In default _gm9 mode_, the firm will be written to a specific location in FCRAM, where it can then be loaded from via the GodMode9 bootloader (see below). A9NC also has an _ask mode_, where the firm will be written to a place of your choice on your SD card.

To setup your system for _gm9 mode_, use [GodMode9](https://github.com/d0k3/GodMode9/releases) to install the GodMode9 firm to FIRM0. GodMode9 will then act as a bootloader for typical boot.firm locations unless R+LEFT is pressed on boot. It will also - without any required keypresses - automatically detect the firm in FCRAM and boot into it.

Requires devKitARM, libctru and zlib to compile. For easy zlib installation [go here](https://github.com/devkitPro/3ds_portlibs).
