is3D = false
Camera.init(TOP_SCREEN, OUTER_CAM, is3D)
scene = OUTER_CAM
while true do
	pad = Controls.read()
	Screen.refresh()
	Screen.clear(BOTTOM_SCREEN)
	Camera.getOutput()
	Screen.debugPrint(0,0, "L = Take Photo", Color.new(0, 255, 0), BOTTOM_SCREEN)
	Screen.debugPrint(0,15, "X = Swap camera scene", Color.new(0, 255, 0), BOTTOM_SCREEN)
	if scene == OUTER_CAM then
		Screen.debugPrint(0,30, "R = Enable/Disable 3D effect", Color.new(0, 255, 0), BOTTOM_SCREEN)
		if Controls.check(pad, KEY_R) and not Controls.check(oldpad, KEY_R) then
			Camera.term()
			if is3D then
				Screen.disable3D()
			else
				Screen.enable3D()
			end
			is3D = not is3D
			Camera.init(TOP_SCREEN, scene, is3D)
		end
	end
	Screen.flip()
	Screen.waitVblankStart()
	if Controls.check(pad, KEY_L) then
		Camera.takePhoto("/file.bmp", VGA_RES, false)
	elseif Controls.check(pad, KEY_X) and not Controls.check(oldpad, KEY_X) then
		Camera.term()
		if scene == OUTER_CAM then
			Screen.disable3D()
			is3D = false
			scene = INNER_CAM
		else
			scene = OUTER_CAM
		end
		Camera.init(TOP_SCREEN, scene, false)
	elseif Controls.check(pad, KEY_START) then
		Camera.term()
		if is3D then
			Screen.disable3D()
		end
		System.exit()
	end
	oldpad = pad
end