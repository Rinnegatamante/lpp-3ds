<b>Introduction:</b><br>
JPGV (Jpeg Video) video format is a video format ideated from BMPV idea to make an easy-to-load video format with a good filesize.<br>
Supported audiocodecs = PCM16 and Vorbis.<br>
Supported videocodecs = MJPEG<br>
Note that if you use Vorbis audicodec, audio info are truncated in header and you'll find a complete OGG container as Audiobuffer.<br>
You can find a simple converter from various video format to JPGV in this folder.<br>
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
<td>Magic (the word "JPGV" in ASCII)</td>
</tr>
<tr>
<tr>
<td>0x04</td>
<td>4</td>
<td>Video framerate</td>
</tr>
<td>0x08</td>
<td>2</td>
<td>Audio type (1 = Mono, 2 = Stereo)</td>
</tr>
<tr>
<td>0x0A</td>
<td>2</td>
<td>Audio buffer bytepersample</td>
</tr>
<tr>
<td>0x0C</td>
<td>2</td>
<td>Audio buffer samplerate</td>
</tr>
<tr>
<td>0x0E</td>
<td>2</td>
<td>Audio codec (0x00 = PCM16, 0x01 = Vorbis)</td>
</tr>
<tr>
<td>0x10</td>
<td>4</td>
<td>Total frames number</td>
</tr>
<tr>
<td>0x14</td>
<td>4</td>
<td>Audio buffer size</td>
</tr>
<tr>
<td>0x18</td>
<td>X</td>
<td>Audio buffer</td>
</tr>
<tr>
<tr>
<td>X</td>
<td>X</td>
<td>Video buffer offsets table</td>
</tr>
<tr>
<td>X</td>
<td>X</td>
<td>Video buffer</td>
</tr></table><br><br>
<b>Credits:</b><br>
JPGV and vid2jpgv made by Rinnegatamante<br>
Special thanks to Aurelio for helping during decodec creation
