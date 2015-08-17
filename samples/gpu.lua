Graphics.init() -- Init GPU

while true do
	Graphics.initBlend(TOP_SCREEN)
	Graphics.drawLine(0, 100, 0, 100, Color.new(255,255,255))
	Graphics.fillRect(20,50,30,40, Color.new(255,0,0))
	Graphics.termBlend()
	Screen.flip()
	Screen.waitVblankStart()
	
	if Controls.check(Controls.read(), KEY_SELECT) then
		Graphics.term() -- Term GPU before exiting
		System.exit()
	end
end