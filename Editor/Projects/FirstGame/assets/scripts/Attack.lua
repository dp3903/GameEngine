local AttackEffects = {
    -- Table to hold our active projectiles
    ActiveProjectiles = {},
    Cooldown = 0.5, -- Time between attacks
    TimeSinceLastAttack = 0.0
}

function AttackEffects:OnCreate()
    print("Projectile Manager Initialized")
end

-- The Player script will call this
function AttackEffects:Emit(startPos, direction, speed, lifetime)
    if self.TimeSinceLastAttack < self.Cooldown then
        return -- Still in cooldown, ignore this attack
    end
    self.TimeSinceLastAttack = 0.0 -- Reset cooldown timer
    -- 1. Ask the C++ engine to create a new entity (Assuming you have this binding)
    local projEntity = Scene.CreateEntity("Fireball")
    
    -- 2. Set initial position
    projEntity:SetPosition(startPos)
    
    -- 3. Set up visual scale/sprite (assuming you have a way to add/set components)
    projEntity:SetScale(Vec3.new(-0.5 * direction.x, 0.5, 1.0)) -- Flip based on direction

    projEntity:AddSpriteRenderer() -- Assuming this function exists
    -- projEntity.SpriteRenderer.Color = Vec4.new(1.0, 0.5, 0.0, 1.0) -- Orange color for visibility
    projEntity.SpriteRenderer.Texture = "textures/Fire-Spell.png" -- Assuming you have a fireball sprite
    projEntity.SpriteRenderer.IsSubTexture = true
    projEntity.SpriteRenderer.SpriteHeight = 1
    projEntity.SpriteRenderer.SpriteWidth = 8
    projEntity.SpriteRenderer.XSpriteIndex = 0
    projEntity.SpriteRenderer.YSpriteIndex = 0

    -- 4. Store it in our tracking table
    table.insert(self.ActiveProjectiles, {
        Entity = projEntity,
        TimeLeft = lifetime,
        Velocity = Vec2.new(direction.x * speed, direction.y * speed)
    })
end

function AttackEffects:OnUpdate(ts)
    -- Iterate BACKWARDS to safely remove items while looping
    self.TimeSinceLastAttack = self.TimeSinceLastAttack + ts

    for i = #self.ActiveProjectiles, 1, -1 do
        local projectile = self.ActiveProjectiles[i]
        
        -- Tick down the timer
        projectile.TimeLeft = projectile.TimeLeft - ts
        
        if projectile.TimeLeft <= 0.0 then
            -- 1. Tell C++ to safely destroy the entity at the end of the frame
            Scene.DestroyEntity(projectile.Entity)
            
            -- 2. Remove it from our Lua tracking table
            table.remove(self.ActiveProjectiles, i)
        else
            -- If using OPTION A (Manual Movement), update position here:
            local currentPos = projectile.Entity.Transform.Translation
            local newPos = Vec3.new(currentPos.x + (projectile.Velocity.x * ts), currentPos.y + (projectile.Velocity.y * ts), currentPos.z)
            projectile.Entity:SetPosition(newPos)

            projectile.Entity.SpriteRenderer.XSpriteIndex = math.floor((projectile.TimeLeft * 10) % projectile.Entity.SpriteRenderer.SpriteWidth) -- Simple animation logic
        end
    end
end

return AttackEffects