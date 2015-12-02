-- This sample can be viewed in action here: https://www.youtube.com/watch?v=NsRG-f-okI0
test = JPGV.load("/output.jpgv")
tot_time_sec = math.ceil(JPGV.getSize(test) / JPGV.getFPS(test))
tot_time_min = 0
while (tot_time_sec >= 60) do
	tot_time_sec = tot_time_sec - 60
	tot_time_min = tot_time_min + 1
end
Sound.init()
JPGV.start(test,NO_LOOP)
oldpad = Controls.read()
timer = Timer.new()
white = Color.new(255,255,255)
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	JPGV.draw(0,0,test,BOTTOM_SCREEN)
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		Sound.term()
		System.exit()
	end
	if (Timer.getTime(timer) <= 3500) then
		Screen.debugPrint(0,14,"Seid ihr das Essen?",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"Nein, wir sind der Jaegar",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 14500 and Timer.getTime(timer) <= 20500) then
		Screen.debugPrint(0,14,"Fumareta hana no",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"namae mo shirazu ni",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 20500 and Timer.getTime(timer) <= 25500) then
		Screen.debugPrint(0,14,"Chi ni ochita tori wa",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"kaze wo machiwabiru",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 25500 and Timer.getTime(timer) <= 31500) then
		Screen.debugPrint(0,14,"Inotta tokoro de",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"nani mo kawaranai",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 31500 and Timer.getTime(timer) <= 36500) then
		Screen.debugPrint(0,14,"Ima wo kaeru no wa",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"tatakau kakugo da",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 36500 and Timer.getTime(timer) <= 41500) then
		Screen.debugPrint(0,14,"Shikabane fumikoete",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"susumu ishi wo",white,TOP_SCREEN)
		Screen.debugPrint(0,42,"warau buta yo ",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 41500 and Timer.getTime(timer) <= 47500) then
		Screen.debugPrint(0,14,"Kachiku no annei",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"kyogi no han'ei",white,TOP_SCREEN)
		Screen.debugPrint(0,42,"shiseru garou Jiyuu wo!",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 47500 and Timer.getTime(timer) <= 57500) then
		Screen.debugPrint(0,14,"Torawareta kutsujoku wa",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"hangeki no Koushi da",white,TOP_SCREEN)
		Screen.debugPrint(0,42,"jouheki no sono kanata",white,TOP_SCREEN)
		Screen.debugPrint(0,56,"emono wo hofuru Jaeger ",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 57500 and Timer.getTime(timer) <= 65500) then
		Screen.debugPrint(0,14,"Hotobashiru Shoudou ni",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"sono mi wo yakinagara",white,TOP_SCREEN)
		Screen.debugPrint(0,42,"tasogare ni hi wo ugatsu",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 65500 and Timer.getTime(timer) <= 70500) then
		Screen.debugPrint(0,14,"GUREN NO YUMIYA!",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 75000) then
		Screen.debugPrint(0,14,"Shingeki no Kyojin Opening",white,TOP_SCREEN)
		Screen.debugPrint(0,28,"Title: Guren no Yumiya",white,TOP_SCREEN)
		Screen.debugPrint(0,42,"Song by Linked Horizon",white,TOP_SCREEN)
	else
		Screen.debugPrint(0,0,"Lyrics:",white,TOP_SCREEN)
	end
	if (Timer.getTime(timer) > 78000) then
		Screen.debugPrint(0,60,"Achieved with LPP!",white,TOP_SCREEN)
	end
	
	Screen.debugPrint(0,100,"Infos:",white,TOP_SCREEN)
	Screen.debugPrint(0,114,"FPS: "..JPGV.getFPS(test),white,TOP_SCREEN)
	cur_time_sec = math.ceil(JPGV.getFrame(test) / JPGV.getFPS(test))
	cur_time_min = 0
	while (cur_time_sec >= 60) do
		cur_time_sec = cur_time_sec - 60
		cur_time_min = cur_time_min + 1
	end
	if (cur_time_sec < 10) then
		Screen.debugPrint(0,128,"Time: " .. cur_time_min .. ":0" .. cur_time_sec .. " / " .. tot_time_min .. ":" .. tot_time_sec,white,TOP_SCREEN)
	else
		Screen.debugPrint(0,128,"Time: " .. cur_time_min .. ":" .. cur_time_sec .. " / " .. tot_time_min .. ":" .. tot_time_sec,white,TOP_SCREEN)
	end
	Screen.debugPrint(0,142,"Samplerate: "..JPGV.getSrate(test),white,TOP_SCREEN)
	
	percentage = math.ceil((JPGV.getFrame(test) * 100) / JPGV.getSize(test))
	Screen.debugPrint(0,200,"Percentage: " ..percentage .. "%",white,TOP_SCREEN)
	Screen.fillEmptyRect(2,398,214,234,white,TOP_SCREEN)
	move = ((394 * percentage) / 100)
	Screen.fillRect(3,3 + move,215,233,white,TOP_SCREEN)
	
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if (JPGV.isPlaying(test)) then
			JPGV.pause(test)
		else
			JPGV.resume(test)
		end
	end
	Screen.flip()
	oldpad = pad
end