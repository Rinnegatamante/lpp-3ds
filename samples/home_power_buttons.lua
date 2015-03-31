-- NOTE: This sample currently works only with 3DS/CIA builds

while true do
	
	-- Draw something on screen
	Screen.refresh()
	Screen.debugPrint(5,5,"Hello World!",Color.new(255,255,255),TOP_SCREEN)
	Screen.waitVblankStart()
	Screen.flip()
	
	-- Sets up HomeMenu syscall
	if Controls.check(Controls.read(),KEY_HOME) or Controls.check(Controls.read(),KEY_POWER) then
		System.showHomeMenu()
	end
	
	-- Exit if HomeMenu calls APP_EXITING
	if System.checkStatus() == APP_EXITING then
		System.exit()
	end
	
end