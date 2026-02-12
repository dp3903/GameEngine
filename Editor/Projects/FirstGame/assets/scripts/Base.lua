local AnimationConfig = {
    Idle    = { Row = 6, Count = 4, Speed = 0.20 },  
    Walk    = { Row = 1, Count = 6, Speed = 0.15 },  
    Jump    = { Row = 5, Count = 8, Speed = 0.10 },
    Attack1 = { Row =11, Count = 4, Speed = 0.20 },
    Attack2 = { Row =10, Count = 6, Speed = 0.15 }
}

local PlayerAnimationStatus = {
    CurrentState = "Idle",
    FrameIndex = 0,
    TimeElapsed = 0.0,
    
    FacingDirection = 1
}

function PlayerAnimationStatus:OnUpdate(ts, playerEntity)
    
    local requestedState = "Idle"
    
    if self.CurrentState == "Idle" then
        if Input.IsKeyPressed(Key.Space) then
            requestedState = "Jump"
            
        elseif Input.IsKeyPressed(Key.A) then
            requestedState = "Walk"
            self.FacingDirection = -1
            
        elseif Input.IsKeyPressed(Key.D) then
            requestedState = "Walk"
            self.FacingDirection = 1
        elseif Input.IsMouseButtonPressed(Mouse.ButtonLeft) then
            requestedState = "Attack1"
        elseif Input.IsMouseButtonPressed(Mouse.ButtonRight) then
            requestedState = "Attack2"
        end
    end

    if self.CurrentState == "Idle" and requestedState ~= self.CurrentState then
        self.CurrentState = requestedState
        self.FrameIndex = 0
        self.TimeElapsed = 0.0
    end

    local animData = AnimationConfig[self.CurrentState]
    
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
            self.CurrentState = "Idle"  -- Optional: Return to Idle after a Jump finishes
        end
    end

    -- X Index = Which Frame (Column)
    playerEntity.SpriteRenderer.XSpriteIndex = self.FrameIndex
    
    -- Y Index = Which Action (Row)
    playerEntity.SpriteRenderer.YSpriteIndex = animData.Row

    -- Optional: FLIP CHARACTER
    playerEntity.Transform.Scale.x = math.abs(playerEntity.Transform.Scale.x) * self.FacingDirection

    -- Frame based logic (e.g. apply damage on specific attack frames)
    if self.CurrentState == "Jump" then
        if self.FrameIndex == 3 then
            Physics.ApplyLinearImpulse(playerEntity, Vec2.new(0.0, 0.25), Vec2.new(0.0, 0.0), true)
        end
    end
    if self.CurrentState == "Walk" then
        if self.FrameIndex % 2 == 0 then
            Physics.SetLinearVelocity(playerEntity, Vec2.new(0.75 * self.FacingDirection, 0.0))
        end
    end
end

return PlayerAnimationStatus