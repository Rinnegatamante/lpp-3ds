<p align="center">
	<img src="http://rinnegatamante.it/lpplogo.png"/>
</p>

# Description

**Lua Player Plus 3DS** is the first lua interpreter made for Nintendo 3DS.

The interpreter currently runs under Lua 5.3.1. 
It has also a debug FTP server to correct your errors in runtime.

Official documentation: http://rinnegatamante.it/lpp-3ds_doc.html

# Features

##### Graphics

* Advanced 2D Graphics Rendering (CPU and GPU)
* Basic 3D Rendering
* Native support to TTF fonts
* Native support to BMP/PNG/JPG images
* Native support to JPGV video files
* Native support to stereoscopic 3D effect

##### Sounds

* Sound system with support for WAV/AIFF/OGG audio files
* Support to different interpolation modes (None, Linear, Polyphase)
* Complete access to Microphone for sound registration

##### Controls

* Controls system with full console support
* Built-in Keyboard system

##### Network

* Easy to use networking basic features
* Built-in Mail Sender system
* Complete sockets system support
* Built-in debug FTP server

##### System

* Access to a lot of Nintendo syscalls
* Native support to CIA files managing
* Native support to 3DSX and SMDH files
* Native support to ZIP archives
* I/O access for SD card and Extdata archives
* Native support to console Camera to take photos
* Native support to ARM9 payloads loading

##### Utilities

* Timer system
* Advanced arithmetical functions support (math library)

# Showcase

Here are some 3DS homebrews made using Lua Player Plus 3DS:

##### Games
* [Labyrinth 3D](http://gbatemp.net/threads/re-release-labyrinth-3d.409007/)
* [Blackjack 3DS](http://gbatemp.net/threads/release-blackjack-3ds.398776/)
* [Asteroids 3D](https://gbatemp.net/threads/release-asteroids-3d.385600/)
* [PixelRoad](http://gbatemp.net/threads/pixelroad-a-stupid-game-hb-to-test-lpp.375292/)
* [3DSudoku](http://gbatemp.net/threads/release-3dsudoku-sudoku-game-for-3ds.388253/)
* [WANT3D](http://gbatemp.net/threads/release-want3d.398822/)
* [4Rows](https://gbatemp.net/threads/release-4rows.384352/)
* [3DSHangman](https://gbatemp.net/threads/release-3dshangman.379285/)
* [3DS Briscola](http://gbatemp.net/threads/3ds-briscola-briscola-card-game-for-3ds.376901/)

##### Utilities
* [CHMM2](http://gbatemp.net/threads/release-chmm2-custom-home-menu-manager-2.397543/)
* [Sunshell](http://gbatemp.net/threads/release-sunshell-a-powerfull-shell-for-3ds.382483/)
* [ORGANIZ3D](http://gbatemp.net/threads/release-organiz3d-filebrowser-for-3ds.377783/)
* [NASA](http://gbatemp.net/threads/release-nasa-universal-cia-manager-for-fw-4-1-10-3.409806/)
* [Manga Reader](http://gbatemp.net/threads/release-manga-reader.399276/)
* [Alarm Clock](http://gbatemp.net/threads/release-alarm-clock.397624/)
* [SOON!](http://gbatemp.net/threads/soon-super-cool-installer-a-new-cool-legal-open-source-cia-installer.387268/)
* [Bird Whisperer](http://gbatemp.net/threads/release-bird-whisperer-a-notification-creater.404669/)
* [Dice 3DS](https://gbatemp.net/threads/release-dice-3ds-roll-some-dice-pretty-straightforward.400086/)
* [VN3DS](http://gbatemp.net/threads/w-i-p-vn3ds-visual-novel-interpreter-for-3ds-homebrew.396564/)
* [LiveScore 3DS](http://gbatemp.net/threads/release-livescore-3ds.414693/)
* [Crimson Scripter](http://gbatemp.net/threads/crimson-scripter.415152/)

# Compiling the source

The repository will provide you a full set of all needed library. Take in mind that a lot of them are modified versions of the original ones so if you want to update them, take a look at [this repository](https://github.com/Rinnegatamante/lpp-3ds_libraries) where you can find sourcecodes of every pre-builded library used in this repository. Also take in mind that **libkhax** includes also **libSu** sourcecode.<br><br>
Lua Player Plus 3DS supports different macro to enables some features:<br>
**-DUSE_MEMCHUNKHAX2** enables memchunkhax2 usage at startup for firmware between 9.3 and 10.3.<br>
**-DCITRA3DS_COMPATIBLE** removes romFs support causing Citra3DS emulator crash.<br>
**-DSKIP_ERROR_HANDLING** disables error handling for faster code execution (this is used for Unsafe version).<br>
**-DFORCE_SD** forces interpreter to load main script from SD instead of romFs filesystem.<br>
**-DFORCE_DSP** forces interpreter to use dsp::DSP audio service even if csnd:SND is available.<br>

# Credits

* Everyone involved in ctrulib creation
* Smealum for ftpony source used for debug ftp server
* Myria for libkhax
* Lode Vandevenne for lodepng
* xerpi for sf2dlib
* fincs for citro3dlib
* Sean Barrett for stb_truetype
* Jean-loup Gailly and Mark Adler for zlib
* Everyone involved in Brahma developing
* Everyone involved in libSu developing
* Misledz for the Lua Player Plus 3DS logo
* Special thanks to Aurelio and ihaveamac for testing interpreter features
