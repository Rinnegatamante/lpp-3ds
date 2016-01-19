-- Opening a WAV file in non-streaming mode
test = Sound.openWav("/blood.wav", false)

-- Initializing Sound module
Sound.init()

-- Starting playing sound in non-looping mode
Sound.play(test,NO_LOOP)

-- Initializing oldpad
oldpad = 0

-- Main Loop
while true do
	
	-- Reading controls input
	pad = Controls.read()
	
	-- Exiting sample
	if (Controls.check(pad,KEY_A)) then
		Sound.close(test)
		Sound.term()
		System.exit()
	end
	
	-- Pause / Resume sound
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if (Sound.isPlaying(test)) then
			Sound.pause(test)
		else
			Sound.resume(test)
		end
	end
	
	-- Updating oldpad
	oldpad = pad
	
end
