white = Color.new(255,255,255)
fileStream = io.open("/file.txt",FCREATE)
io.write(fileStream,0,"Text written with LPP",21)
io.close(fileStream)
fileStream = io.open("/file.txt",FREAD)
size = io.size(fileStream)
text = io.read(fileStream,0,size)
io.close(fileStream)
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	Screen.debugPrint(0,0,text,white,TOP_SCREEN)
	Screen.flip()
end
