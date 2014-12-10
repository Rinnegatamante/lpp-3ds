test = Sound.openWav("/blood.wav")
Sound.init()
Sound.playWav(test,0x09,NO_LOOP)
playing = true
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	Controls.init()
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		Sound.term()
		System.exit()
	end
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if (playing) then
			Sound.pause(0x09)
			playing = false
		else
			Sound.resume(0x09)
			playing = true
		end
	end
	Screen.flip()
	oldpad = pad
end