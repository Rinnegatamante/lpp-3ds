-- Init 3D Rendering
Render.init(400, 240, Color.new(255, 255, 255, 255))

-- Set vertex shader light
light_color = Render.createColor(1.0, 1.0, 1.0, 1.0)
Render.setLightColor(light_color)
Render.setLightSource(0.5, 0.5, -0.5)

-- Set model material attributes
ambient = Render.createColor(0.2, 0.2, 0.2, 0.0)
diffuse = Render.createColor(0.4, 0.4, 0.4, 0.0)
specular = Render.createColor(0.8, 0.8, 0.8, 0.0)

-- Load a 3D Model with a proper texture
mod1 = Render.loadObject("/Monkey.obj", "/Monkey.png", ambient, diffuse, specular, 1.0)

-- Set default angle and z position
z = -5.0
angleX = 1.0
angleY = 1.0

-- Main loop
while true do
 
	-- Rotate the model
    angleX = angleX + 0.017
    angleY = angleY + 0.034
	
	-- Move the model
    x = math.sin(angleX)
    y = math.sin(angleY)
	
	-- Blend the model on top screen
    Render.initBlend(TOP_SCREEN)
    Render.drawModel(mod1, x, y, z, angleX, angleY)
    Render.termBlend()
   
   -- Exit sample
    if Controls.check(Controls.read(), KEY_START) then
        Render.unloadModel(mod1)
        Render.term()
        System.exit()
    end
   
end