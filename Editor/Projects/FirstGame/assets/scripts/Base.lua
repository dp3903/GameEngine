local AttackEffects = require("scripts.Attack")

local AnimationConfig = {
    Idle    = { Row = 6, Count = 4, Speed = 0.20 },  
    Walk    = { Row = 1, Count = 6, Speed = 0.15 },  
    Jump    = { Row = 5, Count = 8, Speed = 0.10 },
    Attack1 = { Row =11, Count = 4, Speed = 0.20 },
    Attack2 = { Row =10, Count = 6, Speed = 0.15 }
}

local PlayerAnimationStatus = {
    CurrentStates = {
        ["Idle"] = true,
        ["Walk"] = false,
        ["Jump"] = false,
        ["Attack1"] = false,
        ["Attack2"] = false
    },
    FrameIndex = 0,
    TimeElapsed = 0.0,
    MovementSpeed = 2.0,
    Jumpforce = 1.5,
    OnGround = true,
    
    FacingDirection = 1
}

function PlayerAnimationStatus:OnUpdate(ts, playerEntity)
    -- print(self.OnGround)
    AttackEffects:OnUpdate(ts) -- Update projectiles
    
    local requestedStates = {}
    for k, v in pairs(self.CurrentStates) do
        requestedStates[k] = v
    end
    
    -- if self.CurrentState == "Idle" then
        if Input.IsKeyPressed(Key.Space) and self.OnGround then
            self.CurrentStates["Jump"] = true
        else
            self.CurrentStates["Jump"] = false
        end
        if Input.IsKeyPressed(Key.A) then
            self.FacingDirection = -1
            self.CurrentStates["Walk"] = true
        elseif Input.IsKeyPressed(Key.D) then
            self.FacingDirection = 1
            self.CurrentStates["Walk"] = true
        else
            self.CurrentStates["Walk"] = false
        end
        if Input.IsMouseButtonPressed(Mouse.ButtonLeft) then
            self.CurrentStates["Attack1"] = true
        elseif Input.IsMouseButtonPressed(Mouse.ButtonRight) then
            self.CurrentStates["Attack2"] = true
        else
            self.CurrentStates["Attack1"] = false
            self.CurrentStates["Attack2"] = false
        end
    -- end

    local state = "Idle"
    if self.CurrentStates["Jump"] then
        state = "Jump"
    elseif self.CurrentStates["Walk"] then
        state = "Walk"
    elseif self.CurrentStates["Attack1"] then
        state = "Attack1"
    elseif self.CurrentStates["Attack2"] then
        state = "Attack2"
    end
    local animData = AnimationConfig[state]
    
    -- Accumulate time
    self.TimeElapsed = self.TimeElapsed + ts

    -- If enough time passed, advance the frame
    if self.TimeElapsed >= animData.Speed then
        self.TimeElapsed = self.TimeElapsed - animData.Speed
        self.FrameIndex = self.FrameIndex + 1
        
        -- Loop the animation
        self.FrameIndex = self.FrameIndex % animData.Count
        if self.FrameIndex == 0 then
            self.TimeElapsed = 0.0
            -- self.CurrentState = "Idle"  -- Optional: Return to Idle after a Jump finishes
        end
    end

    -- X Index = Which Frame (Column)
    playerEntity.SpriteRenderer.XSpriteIndex = self.FrameIndex
    
    -- Y Index = Which Action (Row)
    playerEntity.SpriteRenderer.YSpriteIndex = animData.Row

    -- Optional: FLIP CHARACTER
    playerEntity.Transform.Scale.x = math.abs(playerEntity.Transform.Scale.x) * self.FacingDirection

    -- Frame based logic (e.g. apply damage on specific attack frames)
    if self.CurrentStates["Jump"] == true and self.OnGround then
        Physics.ApplyLinearImpulse(playerEntity, Vec2.new(0.0, self.Jumpforce), Vec2.new(0.0, 0.0), true)
    end
    if self.CurrentStates["Walk"] == true then
        Physics.SetLinearVelocity(playerEntity, Vec2.new(self.MovementSpeed * self.FacingDirection, Physics.GetLinearVelocity(playerEntity).y))    
    end
    if self.CurrentStates["Attack1"] == true then
        local playerPos = playerEntity.Transform.Translation
        local direction = Vec2.new(self.FacingDirection, 0.0)
        AttackEffects:Emit(Vec3.new(playerPos.x + (self.FacingDirection * 0.5), playerPos.y, playerPos.z), direction, 5.0, 2.0)
    end
end

function PlayerAnimationStatus:OnCollisionBegin(playerEntity, otherEntity)
    -- print("Collision with: " .. otherEntity:GetName())
    if otherEntity:GetName() == "ground" then
        -- print("Player is on the ground")
        self.OnGround = true
    end
end

function PlayerAnimationStatus:OnCollisionEnd(playerEntity, otherEntity)
    -- print("Collision ended with: " .. otherEntity:GetName())
    if otherEntity:GetName() == "ground" then
        -- print("Player left the ground")
        self.OnGround = false
    end
end

return PlayerAnimationStatus