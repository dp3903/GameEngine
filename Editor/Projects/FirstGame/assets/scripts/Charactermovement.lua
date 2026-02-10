local PlayerAnimationStatus = require("scripts.Base")

local player = {}

function player:OnCreate()
    print("Player initialized with Animation System")
end

function player:OnUpdate(ts)
    PlayerAnimationStatus:OnUpdate(ts, self.Entity)
end

return player