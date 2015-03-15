while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.debugPrint(0,0,"Hello World",Color.new(255,255,255),TOP_SCREEN)
	Screen.flip()
end
