-- Main Loop
while true do

	-- Updating screens
	Screen.waitVblankStart()
	Screen.refresh()
	
	-- Writing something on screen
	Screen.debugPrint(0,0,"Hello World",Color.new(255,255,255),TOP_SCREEN)
	
	-- Flipping screen
	Screen.flip()
	
end
