-- Initialize some colors
white = Color.new(255,255,255)
black = Color.new(0,0,0)

-- Initialize oldpad
oldpad = 0

-- Loading an image from SD root
bitmap2 = Screen.loadImage("/file.bmp")

-- Creating a full black image
bitmap = Screen.createImage(1,1,black)

-- Saving flipped bitmap2 image on bitmap pointer
Screen.flipImage(bitmap2,bitmap)

-- Drawing something on bitmap2 image
Screen.fillRect(50,150,1,15,white,bitmap2)

-- Writing something on bitmap2 image
Screen.debugPrint(51,3,"Image...OK!",black,bitmap2)

-- Main Loop
while true do
	
	-- Update screens
	Screen.waitVblankStart()
	Screen.refresh()
	
	-- Read controls input
	pad = Controls.read()
	
	-- Exit sample
	if (Controls.check(pad,KEY_A)) then
		Screen.freeImage(bitmap)
		Screen.freeImage(bitmap2)
		System.exit()
	end
	
	-- Save bitmap2 image and take a full screenshot
	if (Controls.check(pad,KEY_B)) and not (Controls.check(oldpad,KEY_B)) then
		Screen.saveImage(bitmap2,"/bitmap.bmp",false)
		System.takeScreenshot("/screenshot.bmp",false)
	end
	
	-- Drawing both images on screens
	Screen.drawImage(100,127,bitmap2,TOP_SCREEN)
	Screen.drawImage(60,0,bitmap,BOTTOM_SCREEN)
	
	-- Flipping screens
	Screen.flip()
	
	-- Updating oldpad
	oldpad = pad
	
end
