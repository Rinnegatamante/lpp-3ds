REM Change input.mp4 with your own file
REM Change 240x400 with your own resolution (NOTE: HEIGHTxWIDTH !!!)
REM Change -r 30 with your own framerate
REM Change -ac 2 with your own audiochannel
REM Change -ar 44100 with your own audio samplerate
ffmpeg -i "input.mp4" -r 30 -vf "transpose=1" -s 240x400 "temp\output%%1d.jpg"
ffmpeg.exe -i "input.mp4" -acodec pcm_s16le -ac 2 -ar 44100 temp\audio.wav"
REM Change 30 with your own framerate
jpgv_encoder.exe 30
del /q ".\temp\*.*"