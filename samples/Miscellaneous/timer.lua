-- Initializing a color
red = Color.new(255,0,0)

-- Starting a new timer
timer = Timer.new()

-- Main Loop
while true do

	-- Updating screen
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(BOTTOM_SCREEN)
	
	-- Reading controls input
	pad = Controls.read()
	
	-- Exit sample
	if (Controls.check(pad,KEY_A)) then
		Timer.destroy(timer)
		System.exit()
	end
	
	-- Writing current timer time
	Screen.debugPrint(0,30,"You lost " .. math.ceil(Timer.getTime(timer)/1000) .. " seconds of your life.",red,BOTTOM_SCREEN)
	
	-- Flipping screen
	Screen.flip()
	
end