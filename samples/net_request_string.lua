last_version = "Turn on Wi-Fi to check version"
checked = false
timer = false
if Network.isWifiEnabled() then
	last_version = "Last version: " .. Network.requestString("http://rinnegatamante.netsons.org/ORGANIZ3D.txt")
	checked = true
end
while true do
	if not checked then
		if Network.isWifiEnabled() then
		--Temp Timer for waiting establishing Internet connection
		--It will be substituted nearly with a proper function
			wait = Timer.new()
			checked = true
			timer = true
		end
	elseif timer then
		if Network.isWifiEnabled() then
			if Timer.getTime(wait) >= 8000 then
				last_version = "Last version: " .. Network.requestString("http://rinnegatamante.netsons.org/ORGANIZ3D.txt")
				timer = false
			end
		else
			checked = false
		end
	end
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	Screen.debugPrint(0,0,last_version,0xFFFFFF,TOP_SCREEN)
	Controls.init()
	if Controls.check(Controls.read(),KEY_A) then
		Sound.term()
		System.exit()
	end
	Screen.flip()
	Screen.waitVblankStart()
end