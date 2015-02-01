This is the official repository for Lua Player Plus, the first lua interpreter for 3DS made by Rinnegatamante & NaNNi.
<br><br>
The interpreter runs under LUA 5.2.3 and is a strict porting of our Lua Player Plus for PSP/PSVITA.
<br><br>
Your main script must be in the same directory of 3dsx executable.<br>
If you're using a Gateway please note that you cannot use Sound Module (it will be replaced by an empty module) and your main script must be in the root of the SD Card.
<br><br>
Official support board: http://luaplayerplus.proboards.com<br>
Official temporary documentation: http://rinnegatamante.netsons.org/lpp-3ds_doc.html
<br><br>
Credits:<br>
Smealum for ctrulib<br>
StapleButter for debug font<br>
Lode Vandevenne for lodepng<br>
Jean-loup Gailly and Mark Adler for zlib<br>
Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations
<br><br>
NOTE: BMPV video format is a RAW video format created by me with the help of Aurelio.<br>
More info about this format and a simple converter coded in VB.NET can be found in "BMPV" folder of this repository.<br><br>
<b>Note: If you get errors during compilation with luaAudio.cpp, just add prototypes of functions which compiler tells you to be not declared in libctru's CSND header.</b>
