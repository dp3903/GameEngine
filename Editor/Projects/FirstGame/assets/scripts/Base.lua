local AttackEffects = require("scripts.Attack")

local AnimationConfig = {
    Idle    = { Row = 6, Column = 0, Count = 4, Speed = 0.20 },  
    Walk    = { Row = 1, Column = 0, Count = 6, Speed = 0.15 },  
    Jump    = { Row = 5, Column = 3, Count = 4, Speed = 0.25 },
    Attack1 = { Row =11, Column = 0, Count = 4, Speed = 0.15 },
    Attack2 = { Row =10, Column = 0, Count = 6, Speed = 0.15 }
}

local PlayerAnimationStatus = {
    CurrentStates = {
        ["Idle"] = true,
        ["Walk"] = false,
        ["Jump"] = false,
        ["Attack1"] = false,
        ["Attack2"] = false
    },
    AnimationState = "Idle",
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
    
    if self.OnGround then
        if Input.IsKeyPressed(Key.Space) then
            self.CurrentStates["Jump"] = true
            Physics.ApplyLinearImpulse(playerEntity, Vec2.new(0.0, self.Jumpforce), Vec2.new(0.0, 0.0), true)   
        end
        if Input.IsKeyPressed(Key.A) then
            self.FacingDirection = -1
            self.CurrentStates["Walk"] = true
            Physics.SetLinearVelocity(playerEntity, Vec2.new(self.MovementSpeed * self.FacingDirection, Physics.GetLinearVelocity(playerEntity).y))    
        elseif Input.IsKeyPressed(Key.D) then
            self.FacingDirection = 1
            self.CurrentStates["Walk"] = true
            Physics.SetLinearVelocity(playerEntity, Vec2.new(self.MovementSpeed * self.FacingDirection, Physics.GetLinearVelocity(playerEntity).y))    
        else
            self.CurrentStates["Walk"] = false
        end
    end
    if Input.IsMouseButtonPressed(Mouse.ButtonLeft) then
        self.CurrentStates["Attack1"] = true
    elseif Input.IsMouseButtonPressed(Mouse.ButtonRight) then
        self.CurrentStates["Attack2"] = true
    else
        self.CurrentStates["Attack1"] = false
        self.CurrentStates["Attack2"] = false
    end
    

    local priority = {"Attack2", "Attack1", "Jump", "Walk", "Idle"}
    for _, p in ipairs(priority) do
        if self.CurrentStates[p] then
            if self.AnimationState ~= p then
                self.FrameIndex = 0 -- Reset to first frame of new animation
                self.TimeElapsed = 0.0 -- Reset timer for new animation
                print("Switching to animation: " .. p)
            end
            self.AnimationState = p
            break
        end
    end

    local animData = AnimationConfig[self.AnimationState]
    
    -- Accumulate time
    self.TimeElapsed = self.TimeElapsed + ts

    -- If enough time passed, advance the frame
    if self.TimeElapsed >= animData.Speed then
        self.TimeElapsed = self.TimeElapsed - animData.Speed
        self.FrameIndex = self.FrameIndex + 1
        
        -- Loop the animation
        self.FrameIndex = self.FrameIndex % animData.Count
        if self.FrameIndex == 0 then
            self.TimeElapsed = 0.0 -- Reset timer at the end of the animation cycle
        end
    end

    -- X Index = Which Frame (Column)
    playerEntity.SpriteRenderer.XSpriteIndex = self.FrameIndex + animData.Column
    
    -- Y Index = Which Action (Row)
    playerEntity.SpriteRenderer.YSpriteIndex = animData.Row

    -- Optional: FLIP CHARACTER
    playerEntity.Transform.Scale.x = math.abs(playerEntity.Transform.Scale.x) * self.FacingDirection

    -- Frame based logic (e.g. apply damage on specific attack frames)
    if self.CurrentStates["Attack1"] == true and self.FrameIndex == 2 then
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
        self.CurrentStates["Jump"] = false -- Reset jump state when landing
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