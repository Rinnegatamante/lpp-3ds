red = Color.new(255,0,0)
blue = Color.new(0,0,255)
green = Color.new(0,255,0)
while true do
	Screen.waitVblankStart()
	pad = Controls.read()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	Screen.debugPrint(0,math.random(5,50),"Test math.random",red,TOP_SCREEN)
	Screen.debugPrint(0,0,"LPP 3DS Test Script",red,BOTTOM_SCREEN)
	Screen.debugPrint(0,15,"math Library - OK",red,BOTTOM_SCREEN)
	Screen.flip()
end