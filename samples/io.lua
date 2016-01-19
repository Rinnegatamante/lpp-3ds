-- Initializing a color
white = Color.new(255,255,255)

-- Creating a file with writing attribute
fileStream = io.open("/file.txt",FCREATE)

-- Writing something in the file
io.write(fileStream,0,"Text written with LPP",21)

-- Closing the file
io.close(fileStream)

-- Opening a file with reading attribute
fileStream = io.open("/file.txt",FREAD)

-- Getting file size
size = io.size(fileStream)

-- Reading the whole file
text = io.read(fileStream,0,size)

-- Closing the file
io.close(fileStream)

-- Main Loop
while true do

	-- Updating screens
	Screen.waitVblankStart()
	Screen.refresh()
	
	-- Reading controls input
	pad = Controls.read()
	
	-- Exiting sample
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	
	-- Drawing readed content on the screen
	Screen.debugPrint(0,0,text,white,TOP_SCREEN)
	
	-- Flipping screens
	Screen.flip()
	
end
