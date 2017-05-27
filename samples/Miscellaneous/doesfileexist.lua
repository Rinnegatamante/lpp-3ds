white = Color.new(255, 255, 255)
pathExist = System.doesPathExist("/3ds/")

while System.mainLoop() do
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)

	pad = Controls.read()

	if Controls.check(pad, KEY_A) then
		System.exit()
	end

	Screen.debugPrint(0, 0, "/3ds exists: "..tostring(pathExist), white, TOP_SCREEN)

	Screen.flip()
end
