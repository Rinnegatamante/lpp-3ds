-- Initializing variables and flags
def = "Turn on Wi-Fi to check version"
checked = false

-- Function to request a string to a host
function request(host)

	-- Setting default return value
	ret = def
	
	-- Checking if WiFi is ON
	if Network.isWifiEnabled() then

		-- Requesting a string to the host
		ret = Network.requestString(host)
		checked = true
	
	end
	
	-- Returning result
	return ret
	
end

-- Main Loop
while true do

	-- Checking if a request has already done
	if not checked then
		
		-- Requesting a string to an host
		last_version = "Last version: " .. request("http://rinnegatamante.netsons.org/ORGANIZ3D.txt")

	end
	
	-- Updating screen
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- Writing request result on the screen
	Screen.debugPrint(0,0,last_version,Color.new(255, 255, 255),TOP_SCREEN)
	
	-- Exit sample
	if Controls.check(Controls.read(),KEY_A) then
		System.exit()
	end
	
	-- Flipping screens
	Screen.flip()
	Screen.waitVblankStart()

end