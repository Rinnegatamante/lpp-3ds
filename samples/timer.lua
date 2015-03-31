red = Color.new(255,0,0)
timer = Timer.new()
while true do
	Screen.waitVblankStart()
	pad = Controls.read()
	Screen.refresh()
	Screen.clear(BOTTOM_SCREEN)
	if (Controls.check(pad,KEY_A)) then
		Timer.destroy(timer)
		System.exit()
	end
	Screen.debugPrint(0,30,"You lost " .. math.ceil(Timer.getTime(timer)/1000) .. " seconds of your life.",red,BOTTOM_SCREEN)
	Screen.flip()
end