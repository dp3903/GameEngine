local AnimationConfig = {
    Walk    = { Row = 0, Column = 0, Count = 9, Speed = 0.15 },
    Attack  = { Row = 9, Column = 0, Count = 5, Speed = 0.20 },
    Hurt    = { Row = 3, Column = 0, Count = 3, Speed = 0.25 },
    Death   = { Row = 5, Column = 0, Count = 2, Speed = 0.5 },
}

local Enemy = {
    AnimationState = "Walk",
    FrameIndex = 0,
    TimeElapsed = 0.0,
    MovementSpeed = 2.0,
    Alive = true,
    FacingDirection = 1,
    MaxHP = 5,
    HP = 5
}

function Enemy:OnCreate()
    self.CurrentStates = {
        ["Walk"] = true,
        ["Hurt"] = false,
        ["Attack"] = false,
        ["Death"] = false
    }
end

function Enemy:OnDeadCallback()

    self.CurrentStates["Death"] = false
    self.Alive = true
    self.HP = self.MaxHP

end

function Enemy:HPSpriteUpdate()
    local hpIndicator = self.Entity.Relation.FirstChild -- Assuming the HP indicator is the first child of the enemy entity
    if hpIndicator then
        local row = math.floor(self.HP / 3)
        hpIndicator.SpriteRenderer.YSpriteIndex = row
        local col = 2 - self.HP % 3
        hpIndicator.SpriteRenderer.XSpriteIndex = col
    end
end

function Enemy:OnUpdate(ts)
    if self.Alive then

        local priority = {"Death", "Attack", "Hurt", "Walk"}
        for _, p in ipairs(priority) do
            if self.CurrentStates[p] then
                if self.AnimationState ~= p then
                    self.FrameIndex = 0 -- Reset to first frame of new animation
                    self.TimeElapsed = 0.0 -- Reset timer for new animation
                    print("Switching to enemy animation: " .. p)
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
                if self.AnimationState == "Death" then
                    self.Alive = false
                    self.FrameIndex = animData.Count - 1
                    print("Player died.")
                    if self.OnDeadCallback and (type(self.OnDeadCallback) == "function") then
                        self:OnDeadCallback()
                    else
                        print("Provided callback on dead is a '"..type(self.OnDeadCallback).."' and not a 'function'.")
                    end
                    self.Entity:SetEnabled(false)
                end
                if self.AnimationState ~= "Walk" then
                    self.CurrentStates[self.AnimationState] = false
                end
            end
        end

        -- X Index = Which Frame (Column)
        self.Entity.SpriteRenderer.XSpriteIndex = self.FrameIndex + animData.Column
        
        -- Y Index = Which Action (Row)
        self.Entity.SpriteRenderer.YSpriteIndex = animData.Row

        -- Frame based logic (e.g. apply damage on specific attack frames)
        if self.AnimationState == "Walk" and (self.FrameIndex == 2 or self.FrameIndex == 6) then
            local direction = CallFunction("GetEnemyDirection", self.Entity:GetUUID())
            Physics.SetLinearVelocity(self.Entity, Vec2.new(direction * self.MovementSpeed, 0))
        end

        self:HPSpriteUpdate()
    end
end

function Enemy:OnCollisionBegin(otherEntity)
    if otherEntity:GetName() == "Fireball" then
        self.Entity.AudioSources["explosion"]:Play()
        self.HP = self.HP - 1
        otherEntity:SetEnabled(false)
        if self.HP == 0 then
            print("Enemy died")
            self.CurrentStates["Death"] = true
            Physics.SetEnabled(self.Entity, false)
            SetGlobal("Score", GetGlobal("Score") + 2)
        else
            print("Enemy " .. self.Entity:GetName() .. " HP reduced to ".. self.HP)
            self.CurrentStates["Hurt"] = true
            SetGlobal("Score", GetGlobal("Score") + 1)
        end
    elseif otherEntity:GetName() == "Player" then
        self.CurrentStates["Attack"] = true
    end
    CallFunction("EnemyCollisionBeginHandler", self.Entity, otherEntity)
end

function Enemy:OnDestroy()

end

return Enemy