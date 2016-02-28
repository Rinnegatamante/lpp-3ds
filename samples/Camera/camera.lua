-- 3D Setting
is3D = false

-- Initialize Camera
Camera.init(TOP_SCREEN, OUTER_CAM, PHOTO_MODE_NORMAL, is3D)
scene = OUTER_CAM

-- Main Loop
while true do
	
	-- Check for controls input
	pad = Controls.read()
	
	-- Clear bottom screen
	Screen.refresh()
	Screen.clear(BOTTOM_SCREEN)
	
	-- Print camera output on the screen
	Camera.getOutput()
	
	-- Print controls functions
	Screen.debugPrint(0,0, "L = Take Photo", Color.new(0, 255, 0), BOTTOM_SCREEN)
	Screen.debugPrint(0,15, "X = Swap camera scene", Color.new(0, 255, 0), BOTTOM_SCREEN)
	
	-- Check if camera actually used is the outer ones
	if scene == OUTER_CAM then
	
		-- 3D Management
		Screen.debugPrint(0,30, "R = Enable/Disable 3D effect", Color.new(0, 255, 0), BOTTOM_SCREEN)
		if Controls.check(pad, KEY_R) and not Controls.check(oldpad, KEY_R) then
			Camera.term()
			if is3D then
				Screen.disable3D()
			else
				Screen.enable3D()
			end
			is3D = not is3D
			Camera.init(TOP_SCREEN, scene, PHOTO_MODE_NORMAL, is3D)
		end
		
	end
	
	-- Flipping screen
	Screen.flip()
	Screen.waitVblankStart()
	
	-- Take a Photo
	if Controls.check(pad, KEY_L) then
		Camera.takePhoto("/file.bmp", VGA_RES, false)
		
	-- Swap between inner and outer cam
	elseif Controls.check(pad, KEY_X) and not Controls.check(oldpad, KEY_X) then
		Camera.term()
		if scene == OUTER_CAM then
			Screen.disable3D()
			is3D = false
			scene = INNER_CAM
		else
			scene = OUTER_CAM
		end
		Camera.init(TOP_SCREEN, scene, PHOTO_MODE_NORMAL, false)
		
	-- Exit sample
	elseif Controls.check(pad, KEY_START) then
		Camera.term()
		if is3D then
			Screen.disable3D()
		end
		System.exit()
	end
	
	-- Update oldpad
	oldpad = pad
	
end