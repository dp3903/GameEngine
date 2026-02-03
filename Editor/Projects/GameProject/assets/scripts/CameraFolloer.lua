local CameraFollower = {}

function CameraFollower:OnCreate()
    print("CameraFollower Created!")
end

function CameraFollower:OnUpdate(ts)
    -- Get ball entity
    local ball = GetEntityByTag("Ball")
    -- Get Transform components
    local ballTransform = ball.Transform
    -- Update this entity's Translation to follow the ball
    self.Entity.Transform.Translation.x = ballTransform.Translation.x
    self.Entity.Transform.Translation.y = ballTransform.Translation.y

    if Input.IsKeyPressed(Key.P) then
        print("Health:", GetGlobal("Health"))
        print("PlayerName:", GetGlobal("PlayerName"))
        print("IsAlive:", GetGlobal("IsAlive"))
        print("Position:", GetGlobal("Position").x, GetGlobal("Position").y, GetGlobal("Position").z)
    end
end

function CameraFollower:OnDestroy()
    print("CameraFollower Destroyed!")
end

return CameraFollower