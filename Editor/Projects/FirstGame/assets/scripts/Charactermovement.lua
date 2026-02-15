local PlayerAnimationStatus = require("scripts.Base")

local player = {}

function player:OnCreate()
    print("Player initialized with Animation System")
end

function player:OnUpdate(ts)
    PlayerAnimationStatus:OnUpdate(ts, self.Entity)
end

function player:OnCollisionBegin(otherEntity)
    PlayerAnimationStatus:OnCollisionBegin(self.Entity, otherEntity)
end

function player:OnCollisionEnd(otherEntity)
    PlayerAnimationStatus:OnCollisionEnd(self.Entity, otherEntity)
end

return player