local CameraFollower = {}

function CameraFollower:OnCreate()
    print("CameraFollower Created!")
end

function CameraFollower:OnUpdate(ts)
    -- Get ball entity
    local ball = GetEntityByTag("Ball")
    -- Get Transform components
    local ballPos = ball:GetPosition()
    -- Update this entity's Translation to follow the ball
    local newPos = Vec3.new(ballPos.x, ballPos.y, self.Entity:GetPosition().z)
    self.Entity:SetPosition(newPos)

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