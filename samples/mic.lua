-- Initializing Sound module
Sound.init()

-- Initializing oldpad
oldpad = 0

-- Initializing some flags
does_reg_exist = false
started = false

-- Main Loop
while true do

	-- Updating screens
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- Reading controls input
	pad = Controls.read()
	
	-- Printing controls functions
	Screen.debugPrint(0,0,"Press A to exit",Color.new(255,255,255),TOP_SCREEN)
	
	-- Checking if microphone started recording and if a registration already exists
	if started and not does_reg_exist then
	
		-- Checking if microphone is currently recording
		if Mic.isRecording() then
		
			-- Printing control needed to stop recording
			Screen.debugPrint(0,30,"Press L to stop recording...",Color.new(255,255,255),TOP_SCREEN)
			Screen.debugPrint(0,45,"Recording...",Color.new(255,255,255),TOP_SCREEN)
			
			-- Stop recording
			if Controls.check(pad, KEY_L) then
				test = Mic.stop()
				does_reg_exist = true
				started = false
			end
		
		-- If it started recording and microphone is not recording then it finished recording
		else
		
			-- Saving recorded audio
			test = Mic.stop()
			does_reg_exist = true
			started = false
			
		end
		
	else
	
		-- Printing control needed to start recording
		Screen.debugPrint(0,15,"Press B to start recording",Color.new(255,255,255),TOP_SCREEN)
		
	end
	
	-- Checking if a registration already exists
	if does_reg_exist then
	
		-- Printing controls functions
		Screen.debugPrint(0,30,"Press X to save audio",Color.new(255,255,255),TOP_SCREEN)
		Screen.debugPrint(0,45,"Press Y to play audio",Color.new(255,255,255),TOP_SCREEN)
		
		-- Saving recorded audio to a WAV file
		if (Controls.check(pad,KEY_X)) and not (Controls.check(oldpad,KEY_X)) then
			Sound.saveWav(test,"/mic_sample.wav")
		end
		
		-- Playing recorded audio in non-looping mode
		if (Controls.check(pad,KEY_Y)) and not (Controls.check(oldpad,KEY_Y)) then
			Sound.play(test,NO_LOOP)
		end
		
	end
	
	-- Exit sample
	if (Controls.check(pad,KEY_A)) and not (Controls.check(oldpad,KEY_A)) then
		if does_reg_exist then
			Sound.close(test)
		end
		Sound.term()
		System.exit()
	end
	
	-- Start recording
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if does_reg_exist then
			Sound.close(test)
		end
		Mic.start(10,32730)
		does_reg_exist = false
		started = true
	end
	
	-- Flipping screens
	Screen.flip()
	
	-- Updating oldpad
	oldpad = pad
	
end