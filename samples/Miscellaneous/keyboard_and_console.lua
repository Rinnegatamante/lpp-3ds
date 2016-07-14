-- Initializing a console for top screen
cns = Console.new(TOP_SCREEN)

-- Main Loop
while true do

	-- Updating screens
	Screen.waitVblankStart()
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- Showing console
	Console.show(cns)
	
	-- Check if user tapped the return button in the keyboard
	local kbState = Keyboard.getState()
	if kbState ~= FINISHED then
	
		-- Print keyboard graphics
		Keyboard.show()
		
		-- Check if at least one button has been tapped
		if kbState ~= NOT_PRESSED then
			
			-- Purge console text
			Console.clear(cns)
			
			-- Check if user pressed a button different to the clear one
			if kbState == PRESSED then
			
				-- Append the user input	
				Console.append(cns, Keyboard.getInput())
			
			end
			
		end
	
	else
		
		-- If user tapped return button, exit sample
		Console.destroy(cns)
		System.exit()
			
	end
	
	-- Flipping screens
	Screen.flip()
	
end