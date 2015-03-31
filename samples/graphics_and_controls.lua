red = Color.new(255,0,0)
blue = Color.new(0,0,255)
green = Color.new(0,255,0)
while true do
	Screen.waitVblankStart()
	pad = Controls.read()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	Screen.fillRect(5,150,5,20,green,TOP_SCREEN)
	if (Controls.check(pad,KEY_A)) then
		Screen.debugPrint(6,6,"Button A pressed",blue,TOP_SCREEN)
	else
		Screen.debugPrint(6,6,"Button A not pressed",blue,TOP_SCREEN)
	end
	Screen.debugPrint(0,0,"LPP 3DS Test Script",red,BOTTOM_SCREEN)
	Screen.debugPrint(0,15,"Screen & Color Modules - OK",red,BOTTOM_SCREEN)
	Screen.debugPrint(0,30,"Controls Module - OK",red,BOTTOM_SCREEN)
	Screen.flip()
end