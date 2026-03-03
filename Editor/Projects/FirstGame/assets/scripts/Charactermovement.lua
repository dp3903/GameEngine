local PlayerAnimationStatus = require("scripts.Base")

local player = {}

function MyCallback()
    print("Called callback on player dead.")
end

function player:OnCreate()
    PlayerAnimationStatus:OnCreate()
    PlayerAnimationStatus.OnDeadCallback = MyCallback
    print("Player initialized with Animation System")
    SetGlobal("Score", 0)
end

function player:OnUpdate(ts)
    PlayerAnimationStatus:OnUpdate(ts, self.Entity)
    GetEntityByTag("Score").Text.TextString = "Score: " .. GetGlobal("Score")
end

function player:OnCollisionBegin(otherEntity)
    PlayerAnimationStatus:OnCollisionBegin(self.Entity, otherEntity)
end

function player:OnCollisionEnd(otherEntity)
    PlayerAnimationStatus:OnCollisionEnd(self.Entity, otherEntity)
end

return player