while true do
Screen.waitVblankStart()
Screen.refresh()
Screen.debugPrint(0,0,"Hello World",0xFFFFFF,TOP_SCREEN)
Screen.flip()
end