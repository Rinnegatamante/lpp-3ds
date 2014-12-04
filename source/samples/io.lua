white = Color.new(255,255,255)
fileStream = System.openFile("/file.txt",FCREATE)
System.writeFile(fileStream,0,"Text written with LPP")
System.closeFile(fileStream)
fileStream = System.openFile("/file.txt",FREAD)
size = System.getFileSize(fileStream)
text = System.readFile(fileStream,0,size)
System.closeFile(fileStream)
while true do
	Screen.waitVblankStart()
	Screen.refresh()
	Controls.init()
	pad = Controls.read()
	if (Controls.check(pad,KEY_A)) then
		System.exit()
	end
	Screen.debugPrint(0,0,text,white,TOP_SCREEN)
	Screen.flip()
end