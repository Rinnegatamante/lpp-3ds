<p align="center">
	<img src="http://rinnegatamante.it/lpplogo.png"/>
</p>

# Description

**Lua Player Plus 3DS** is the first lua interpreter made for Nintendo 3DS.

The interpreter currently runs under Lua 5.3.1. 
It has also a debug FTP server to correct your errors in runtime.

Official documentation: http://rinnegatamante.it/lpp-3ds_doc.html<br>
Official support board: http://rinnegatamante.it/board/forumdisplay.php?fid=12

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
* SSL connections support

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

# Nightly Builds

We offer an automated builder service for our nightly builds. Nightly builds are automatically updated every 4 hours by a server we own. To take advantage of this service, you should visit [this page](http://rinnegatamante.it/lpp-nightly.php).

# Showcase

Here are some 3DS homebrews made using Lua Player Plus 3DS:

##### Games
* [Labyrinth 3D](https://rinnegatamante.it/board/showthread.php?tid=18)
* [Blackjack 3DS](http://gbatemp.net/threads/release-blackjack-3ds.398776/)
* [Asteroids 3D](https://rinnegatamante.it/board/showthread.php?tid=25)
* [PixelRoad](https://rinnegatamante.it/board/showthread.php?tid=11)
* [3DSudoku](https://rinnegatamante.it/board/showthread.php?tid=19)
* [WANT3D](http://rinnegatamante.it/board/showthread.php?tid=23)
* [4Rows](https://rinnegatamante.it/board/showthread.php?tid=32)
* [3DSHangman](https://rinnegatamante.it/board/showthread.php?tid=33)
* [3DS Briscola](https://rinnegatamante.it/board/showthread.php?tid=17)
* [EUCLIOD](http://gbatemp.net/threads/eucliod-a-homebrew-bullet-hell-for-the-3ds.419553/)
* [RPS 3D](https://rinnegatamante.it/board/showthread.php?tid=43)
* [Cookie Clicker](http://gbatemp.net/threads/w-i-p-cookie-clicker-for-3ds.460836/)
* [TriaAl](https://gbatemp.net/threads/release-triaal-deemo-clone-for-nintendo-3ds.430308/)

##### Utilities
* [CHMM2](https://rinnegatamante.it/board/showthread.php?tid=9)
* [Sunshell](https://rinnegatamante.it/board/showthread.php?tid=8)
* [ORGANIZ3D](https://rinnegatamante.it/board/showthread.php?tid=24)
* [NASA](https://rinnegatamante.it/board/showthread.php?tid=16)
* [Manga Reader](http://gbatemp.net/threads/release-manga-reader.399276/)
* [Alarm Clock](http://gbatemp.net/threads/release-alarm-clock.397624/)
* [SOON!](http://gbatemp.net/threads/soon-super-cool-installer-a-new-cool-legal-open-source-cia-installer.387268/)
* [Bird Whisperer](http://gbatemp.net/threads/release-bird-whisperer-a-notification-creater.404669/)
* [Dice 3DS](https://gbatemp.net/threads/release-dice-3ds-roll-some-dice-pretty-straightforward.400086/)
* [LiveScore 3DS](http://gbatemp.net/threads/release-livescore-3ds.414693/)
* [Tinker](http://gbatemp.net/threads/tinker-shuffle-menuhax-theme-manager.407445/)
* [MCIT](http://gbatemp.net/threads/release-miiverse-custom-image-tool.415505/)
* [Video Channels Creator](https://rinnegatamante.it/board/showthread.php?tid=10)
* [BLEND3R](https://rinnegatamante.it/board/showthread.php?tid=26)
* [Homebr3w](https://github.com/Wolvan/Homebr3w)
* [luaIrc](http://rinnegatamante.it/board/showthread.php?tid=136)
* [FileKong](https://gbatemp.net/threads/release-filekong-ftp-client-for-3ds.429071/)

##### Engines & Interpreters
* [VN3DS](http://gbatemp.net/threads/w-i-p-vn3ds-visual-novel-interpreter-for-3ds-homebrew.396564/)
* [RayCast3D Engine](https://rinnegatamante.it/board/showthread.php?tid=5)
* [Crimson Scripter](http://gbatemp.net/threads/crimson-scripter.415152/)

##### Updaters
* [StarUpdater](https://gbatemp.net/threads/release-starupdater-a-luma3ds-updater.427999/)
* [encTitleKeysUpdater](http://gbatemp.net/threads/release-enctitlekeysupdater-for-freeshop.429912/)
* [Corbenik CFW Updater](http://rinnegatamante.it/board/showthread.php?tid=85)
* [EasyRPG Updater](http://rinnegatamante.it/board/showthread.php?tid=61)

# Compiling the source

The repository will provide you a full set of all of the needed libraries. Take in mind that a lot of them are modified versions of the original ones so if you want to update them, take a look at [this repository](https://github.com/Rinnegatamante/lpp-3ds_libraries) where you can find sourcecodes of every pre-builded library used in this repository. Also take in mind that **libkhax** includes also **libSu** sourcecode. Last but not least, please note that **libjpeg.a** file is a standard compilation of libjpeg-turbo.<br><br>
Lua Player Plus 3DS supports different flags to enables some features:<br>
**-DUSE_MEMCHUNKHAX2** enables memchunkhax2 usage through svchax at startup for firmwares between 9.3 and 10.7.<br>
**-DCITRA3DS_COMPATIBLE** removes romFs support causing Citra3DS emulator crash.<br>
**-DSKIP_ERROR_HANDLING** disables error handling for faster code execution (this is used for Unsafe version).<br>
**-DFORCE_SD** forces interpreter to load main script from SD instead of romFs filesystem.<br>
**-DFORCE_DSP** forces interpreter to use dsp::DSP audio service even if csnd:SND is available.<br>

# Credits

* Everyone involved in ctrulib creation
* Smealum for ftpony source used for debug ftp server
* Myria for libkhax
* xerpi for sf2dlib
* fincs for citro3dlib
* Sean Barrett for stb_truetype
* Everyone involved in lodepng, zlib, mpg123, libogg, vorbisfile libraries
* Everyone involved in Brahma developing
* aliaspider for svchax
* Misledz for the Lua Player Plus 3DS logo
* EasyRPG Team for the AudioDecoder used for MP3 support
* ksanislo for the network module improvements
* Special thanks to Aurelio and ihaveamac for testing interpreter features
