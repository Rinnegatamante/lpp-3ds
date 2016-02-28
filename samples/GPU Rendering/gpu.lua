-- Init GPU
Graphics.init()

-- Main Loop
while true do
	
	-- Start printing on top screen
	Screen.refresh()
	Graphics.initBlend(TOP_SCREEN)
	
	-- Draw something on screen
	Graphics.drawLine(0, 100, 0, 100, Color.new(255,255,255))
	Graphics.fillRect(20,50,30,40, Color.new(255,0,0))
	
	-- Terminate printing phase
	Graphics.termBlend()
	
	-- Flip screens
	Screen.flip()
	Screen.waitVblankStart()
	
	-- Exit sample
	if Controls.check(Controls.read(), KEY_SELECT) then
		Graphics.term()
		System.exit()
	end
	
end