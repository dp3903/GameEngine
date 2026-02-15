local Environment = {}

function Environment:OnCreate()
    local cameraEntity = GetEntityByTag("camera") 

    if cameraEntity ~= nil then
        -- Get the Camera
        local camera = GetPrimaryCamera()

        if camera then
            -- Calculate World Dimensions
            -- Orthographic Size = "Height"
            local orthoSize = camera.OrthographicSize
            -- local aspectRatio = camera.AspectRatio
            local aspectRatio = 22.0 / 9.0 -- Assuming a fixed aspect ratio for simplicity

            local worldHeight = orthoSize * 1.0
            local worldWidth = worldHeight * aspectRatio
            
            -- Apply Scale (z = 1.0 or whatever thickness you want)
            self.Entity:SetScale(Vec3.new(worldWidth, worldHeight, 1.0))

            -- 6. (Optional) Lock Position to Camera Center
            -- If you want the background to always be behind the camera, even if camera moves
            self.Entity:SetPosition(Vec3.new(cameraEntity.Transform.Translation.x, cameraEntity.Transform.Translation.y, self.Entity.Transform.Translation.z))
        end
    else
        print("Error: Could not find 'MainCamera' entity!")
    end
    print("Environment initialized")
end

function Environment:OnUpdate(ts)
    
end

return Environment