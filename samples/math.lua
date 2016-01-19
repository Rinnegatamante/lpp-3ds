-- Initializing some colors
red = Color.new(255,0,0)
blue = Color.new(0,0,255)
green = Color.new(0,255,0)

-- Main Loop
while true do

	-- Updating screens
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- Reading controls input
	pad = Controls.read()
	
	-- Exit sample
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	
	-- Using math.random from math library
	Screen.debugPrint(0,math.random(5,50),"Test math.random",red,TOP_SCREEN)
	
	-- Drawing something on screen
	Screen.debugPrint(0,0,"LPP 3DS Test Script",red,BOTTOM_SCREEN)
	Screen.debugPrint(0,15,"math Library - OK",red,BOTTOM_SCREEN)
	
	-- Flipping screens
	Screen.flip()
end