local AttackEffects = require("scripts.Attack")

local AnimationConfig = {
    Idle    = { Row = 6, Column = 0, Count = 4, Speed = 0.20, StartFrame = 0 },  
    Walk    = { Row = 1, Column = 0, Count = 6, Speed = 0.15, StartFrame = 0 },  
    Jump    = { Row = 5, Column = 3, Count = 4, Speed = 0.25, StartFrame = 0 },
    Attack  = { Row =11, Column = 0, Count = 4, Speed = 0.15, StartFrame = 2 },
    Death   = { Row = 8, Column = 0, Count = 8, Speed = 0.10, StartFrame = 0 },
}

local PlayerAnimationStatus = {
    CurrentStates = {
        ["Idle"] = true,
        ["Walk"] = false,
        ["Jump"] = false,
        ["Attack"] = false,
        ["Death"] = false
    },
    AnimationState = "Idle",
    FrameIndex = 0,
    TimeElapsed = 0.0,
    MovementSpeed = 2.0,
    Jumpforce = 1.5,
    OnGround = true,
    Alive = true,
    OnDeadCallback = nil,
    FacingDirection = 1
}

function PlayerAnimationStatus:OnCreate()
    AttackEffects:OnCreate()
end

function PlayerAnimationStatus:OnUpdate(ts, playerEntity)
    if self.Alive then

        -- print(self.OnGround)
        AttackEffects:OnUpdate(ts) -- Update projectiles
        
        if self.OnGround then
            if Input.IsKeyPressed(Key.Space) then
                self.CurrentStates["Jump"] = true
                Physics.ApplyLinearImpulse(playerEntity, Vec2.new(0.0, self.Jumpforce), Vec2.new(0.0, 0.0), true)
                playerEntity.AudioSources["jump"]:Play() 
            end
            if Input.IsKeyPressed(Key.A) then
                if self.FacingDirection == 1 then
                    playerEntity.SpriteRenderer.FlipX = true;
                end
                self.FacingDirection = -1
                self.CurrentStates["Walk"] = true
                Physics.SetLinearVelocity(playerEntity, Vec2.new(self.MovementSpeed * self.FacingDirection, Physics.GetLinearVelocity(playerEntity).y))    
            elseif Input.IsKeyPressed(Key.D) then
                if self.FacingDirection == -1 then
                    playerEntity.SpriteRenderer.FlipX = false;
                end
                self.FacingDirection = 1
                self.CurrentStates["Walk"] = true
                Physics.SetLinearVelocity(playerEntity, Vec2.new(self.MovementSpeed * self.FacingDirection, Physics.GetLinearVelocity(playerEntity).y))    
            else
                self.CurrentStates["Walk"] = false
            end
        end
        -- if Input.IsKeyPressed(Key.X) then
        --     self.CurrentStates["Death"] = true
        -- end
        if Input.IsMouseButtonPressed(Mouse.ButtonLeft) then
            self.CurrentStates["Attack"] = true
        else
            self.CurrentStates["Attack"] = false
        end
        
        local priority = {"Death", "Attack", "Jump", "Walk", "Idle"}
        for _, p in ipairs(priority) do
            if self.CurrentStates[p] then
                if self.AnimationState ~= p then
                    self.FrameIndex = AnimationConfig[p].StartFrame -- Reset to first frame of new animation
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

        
        -- X Index = Which Frame (Column)
        playerEntity.SpriteRenderer.XSpriteIndex = self.FrameIndex + animData.Column
        
        -- Y Index = Which Action (Row)
        playerEntity.SpriteRenderer.YSpriteIndex = animData.Row
        
        -- Frame based logic (e.g. apply damage on specific attack frames)
        if self.CurrentStates["Attack"] == true and self.FrameIndex == 2 then
            if playerEntity.AudioSources["attack"].IsPlaying == false then
                playerEntity.AudioSources["attack"]:Play()
            end
            local playerPos = playerEntity:GetPosition()
            local direction = Vec2.new(self.FacingDirection, 0.0)
            AttackEffects:Emit(Vec3.new(playerPos.x + (self.FacingDirection * 0.5), playerPos.y, playerPos.z), direction, 5.0, 2.0)
        end
        
        -- If enough time passed, advance the frame
        if self.TimeElapsed >= animData.Speed then
            self.TimeElapsed = self.TimeElapsed - animData.Speed
            self.FrameIndex = self.FrameIndex + 1
            
            -- Loop the animation
            self.FrameIndex = self.FrameIndex % animData.Count
            if self.FrameIndex == 0 then
                self.TimeElapsed = 0.0 -- Reset timer at the end of the animation cycle
                if self.AnimationState == "Death" then
                    self.Alive = false
                    self.FrameIndex = animData.Count - 1
                    print("Player died.")
                    if self.OnDeadCallback and (type(self.OnDeadCallback) == "function") then
                        self.OnDeadCallback()
                    else
                        print("Provided callback on dead is a '"..type(self.OnDeadCallback).."' and not a 'function'.")
                    end
                    playerEntity:SetEnabled(false)
                end
            end
        end
    else
        -- print("Player is dead")
    end
end

function PlayerAnimationStatus:OnCollisionBegin(playerEntity, otherEntity)
    print("Collision with: " .. otherEntity:GetName())
    if otherEntity:GetName() == "ground" then
        -- print("Player is on the ground")
        self.OnGround = true
        self.CurrentStates["Jump"] = false -- Reset jump state when landing
    elseif Scene.IsDescendant(GetEntityByTag("Enemy Manager"), otherEntity) then
        self.CurrentStates["Death"] = true
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