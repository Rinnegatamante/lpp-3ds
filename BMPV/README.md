<b>Introduction:</b><br>
BMPV (Bitmap Video) video format is a RAW video format ideated with only one purpose: make the video loading the easiest possible.<br>
In the future i'll add various compression algorithm for audio and video buffers but, for now, they will be in RAW format.<br>
The bad thing is the eccessive size of a BMPV video (i suggest to use them only for very short videos).<br>
Actually, audio buffer is coded in WAV format with PCM16 codec and video buffer is a sequence of BMP for every frame.<br>
Headers from WAV and BMPs original files are deleted.<br>
You can find a simple converter from various video format to BMPV in this folder. (BMPV Creator is the GUI-style version but require Microsoft .NET Framework 3.5 ; vid2bmpv is the command-line style version wrote in C. Only vid2bmpv is opensource.<br>
<br>
<b>File infos:</b>
<table>
<tr>
<td><b>Offset</b></td>
<td><b>Size</b></td>
<td><b>Info</b></td>
</tr>
<tr>
<td>0x00</td>
<td>4</td>
<td>Magic (the word "BMPV" in ASCII)</td>
</tr>
<tr>
<tr>
<td>0x04</td>
<td>4</td>
<td>Video framerate</td>
</tr>
<tr>
<td>0x08</td>
<td>4</td>
<td>Video buffer width</td>
</tr>
<tr>
<td>0x0C</td>
<td>4</td>
<td>Video buffer height</td>
</tr>
<tr>
<td>0x10</td>
<td>2</td>
<td>Audio type (1 = Mono, 2 = Stereo)</td>
</tr>
<tr>
<td>0x12</td>
<td>2</td>
<td>Audio buffer bytepersample</td>
</tr>
<tr>
<td>0x14</td>
<td>4</td>
<td>Audio buffer samplerate</td>
</tr>
<tr>
<td>0x18</td>
<td>4</td>
<td>Audio buffer size</td>
</tr>
<tr>
<td>0x1C</td>
<td>X</td>
<td>Audio buffer</td>
</tr>
<tr>
<td>X</td>
<td>X</td>
<td>Video buffer</td>
</tr></table><br><br>
<b>Credits:</b><br>
BMPV, vid2bmpv and BMPV Creator made by Rinnegatamante<br>
Special thanks to Aurelio for helping during BMPV creation
