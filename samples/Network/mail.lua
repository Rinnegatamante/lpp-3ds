-- Initializing some variables
checked = false
timer = false

-- Initializing a new console
tmp = Console.new(TOP_SCREEN)

-- Sending mail function
function sendMail(to, title, body)
	
	-- Checking if Wifi is ON
	if Network.isWifiEnabled() then
	
		-- Sending email and getting result
		if Network.sendMail(to, title, body) then
			Console.append(tmp,"Mail successfully sent!")
		else
			Console.append(tmp,"Error sending mail...")
		end
		checked = true
	
	else
		checked = false	
	end
	
end

-- Main Loop
while true do

	-- Trying sending a mail
	if not checked then
		sendMail("rinnegatamante@gmail.com", "Sample mail with lpp-3ds", "This is the body of my sample mail.")
	end
	
	-- Updating screens
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- Showing console text
	Console.show(tmp)
	
	-- Exit sample
	if Controls.check(Controls.read(),KEY_A) then
		Console.destroy(tmp)
		System.exit()
	end
	
	-- Flipping screens
	Screen.flip()
	Screen.waitVblankStart()
	
end