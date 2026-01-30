local Player = {}

-- Properties
Player.Speed = 5.0

function Player:OnCreate()
    print("Player Created!")
end

function Player:OnUpdate(ts)
    print("Type of self is: " .. type(self))
    -- 'self' accesses the instance table
    -- 'self.Entity' is the C++ object we injected
    local name = self.Entity:GetName()
    print("Updating Player: " .. name .. " with timestep: " .. ts)
end

function Player:OnDestroy()
    print("Player Destroyed!")
end

return Player