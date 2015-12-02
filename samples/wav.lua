test = Sound.openWav("/blood.wav")
Sound.init()
Sound.play(test,NO_LOOP)
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		Sound.close(test)
		Sound.term()
		System.exit()
	end
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		if (Sound.isPlaying(test)) then
			Sound.pause(test)
		else
			Sound.resume(test)
		end
	end
	Screen.flip()
	oldpad = pad
end
