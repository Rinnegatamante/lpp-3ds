Sound.init()
oldpad = Controls.read()
does_reg_exist = false
started = false
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	pad = Controls.read()
	Screen.clear(TOP_SCREEN)
	Screen.debugPrint(0,0,"Press A to exit",Color.new(255,255,255),TOP_SCREEN)
	if started and not does_reg_exist then
		if Mic.isRecording() then
			Screen.debugPrint(0,30,"Press L to stop recording...",Color.new(255,255,255),TOP_SCREEN)
			Screen.debugPrint(0,45,"Recording...",Color.new(255,255,255),TOP_SCREEN)
			if Controls.check(pad, KEY_L) then
				test = Mic.stop()
				does_reg_exist = true
				started = false
			end
		else
			test = Mic.stop()
			does_reg_exist = true
			started = false
		end
	else
		Screen.debugPrint(0,15,"Press B to start recording",Color.new(255,255,255),TOP_SCREEN)
	end
	if does_reg_exist then
		Screen.debugPrint(0,30,"Press X to save audio",Color.new(255,255,255),TOP_SCREEN)
		Screen.debugPrint(0,45,"Press Y to play audio",Color.new(255,255,255),TOP_SCREEN)
		if (Controls.check(pad,KEY_X)) and not (Controls.check(oldpad,KEY_X)) then
			Sound.saveWav(test,"/mic_sample.wav")
		end
		if (Controls.check(pad,KEY_Y)) and not (Controls.check(oldpad,KEY_Y)) then
			Sound.play(test,NO_LOOP)
		end
	end
	if (Controls.check(pad,KEY_A)) and not (Controls.check(oldpad,KEY_A)) then
		if does_reg_exist then
			Sound.close(test)
		end
		Sound.term()
		System.exit()
	end
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if does_reg_exist then
			Sound.close(test)
		end
		Mic.start(10,32730)
		does_reg_exist = false
		started = true
	end
	Screen.flip()
	oldpad = pad
end