local ParticleSystem = require("scripts.ParticleSystem")

local MouseClick = {}

function MouseClick:OnCreate()
    ParticleSystem:OnCreate()
    print("MouseClick script created")
end

function MouseClick:OnUpdate(ts)
    ParticleSystem:OnUpdate(ts)
    if Input.IsMouseButtonPressed(Mouse.ButtonLeft) then
        -- local worldPos = Vec2.new(0, 0)
        local viewportLocation = Scene.GetViewportLocation()
        -- print("Viewport location: (" .. viewportLocation.x .. ", " .. viewportLocation.y .. ")")
        local mousePos = Input.GetMousePosition()
        local viewportSize = Vec2.new(Scene.GetViewportWidth(), Scene.GetViewportHeight())
        mousePos = mousePos - viewportLocation
        -- print("Mouse at screen position: (" .. mousePos.x .. ", " .. mousePos.y .. ")")
        local camera = Scene.GetPrimaryCamera().CameraComponent.Camera
        local orthoSize = camera.OrthographicSize
        local aspectRatio = camera.AspectRatio
        local worldHeight = orthoSize * 1.0
        local worldWidth = worldHeight * aspectRatio

        local worldPos = Vec2.new((mousePos.x / viewportSize.x) * worldWidth - worldWidth / 2, -((mousePos.y / viewportSize.y) * worldHeight - worldHeight / 2))
        -- print("Mouse clicked at world position: (" .. worldPos.x .. ", " .. worldPos.y .. ")")
        for i = 1, 3 do
            ParticleSystem:Emit(Vec3.new(worldPos.x, worldPos.y, 0), Vec2.new((Random:Float()*2-1) * 5, (Random:Float()*2-1) * 5))
        end
    end
end

return MouseClick