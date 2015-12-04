<b>Description:</b><br>
This is the official repository for Lua Player Plus, the first lua interpreter for 3DS made by Rinnegatamante & NaNNi.
<br><br>
The interpreter runs under Lua 5.3.1 and is a strict porting of our Lua Player Plus for PSP/PSVITA.
<br><br>
Your main script must be in the same directory of 3dsx executable.<br>
If you're using a CIA/3DS build, the main script must be located in the root of your SD/microSD card.
<br><br>
Official temporary documentation: http://rinnegatamante.netsons.org/lpp-3ds_doc.html
<br><br>
<b>Interpreter features:</b>
<br>- 2D Graphics system with different built-in functions for drawing text, creating debug consoles, filling rectangles, drawing pixels, etc.
<br>- GPU rendering support through Graphics module
<br>- Alpha channel support
<br>- Native support to TTF fonts
<br>- Native support to BMP/PNG/JPG images
<br>- Sound system for reproducing audio files and recording audio with microphone
<br>- Native support to WAV/AIFF/OGG audio files
<br>- Complete access to Nintendo 3DS controls
<br>- I/O features for SDMC and EXTDATA
<br>- Video system for reproducing video files with native support to JPGV video files
<br>- Timer system
<br>- Native 3D support
<br>- Advanced arithmetical functions
<br>- Internet connectivity features through Network module (Like downloading files, sending mails)
<br>- Socketing functions (Like FTP server)
<br>- Read access to different 3DS info (Firmware, Mac Address, Kernel Version, Region, etc.)
<br>- Built-in Keyboard system
<br>- Native support to 3DSX homebrews and SMDH homebrew datas
<br>- Native support to CIA archives
<br>- Native support to ZIP archives
<br>- Native support to console cameras
<br>- Access to some 3DS syscalls (Reboot System, Launching Cartridges, etc.)
<br><br>
<b>Credits:</b><br>
- Smealum for ctrulib and ftpony src<br>
- StapleButter for debug font<br>
- Myria for libkhax<br>
- Lode Vandevenne for lodepng<br>
- xerpi for sf2dlib<br>
- Sean Barrett for stb_truetype<br>
- Jean-loup Gailly and Mark Adler for zlib<br>
- Special thanks to Aurelio and ihaveamac for testing, bug-fixing and various help with codes and implementations
<br><br>
NOTE: JPGV video format is made by me with the help of Aurelio.<br>
More info about this format and a simple converter could be found in "JPGV" folder of this repository.<br><br>
