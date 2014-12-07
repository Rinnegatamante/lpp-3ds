white = Color.new(255,255,255)
oldpad = Controls.read()
bitmap = Screen.loadBitmap("/file.bmp")
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Controls.init()
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		System.takeScreenshot("/file.bmp")
	end
	Screen.printBitmap(0,0,bitmap,BOTTOM_SCREEN)
	Screen.debugPrint(0,0,"Bitmap size: " .. bitmap["width"] .. "x" .. bitmap["height"],white,TOP_SCREEN)
	Screen.flip()
	oldpad = pad
end