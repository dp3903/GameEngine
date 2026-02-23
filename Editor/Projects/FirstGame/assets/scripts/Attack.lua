local AttackEffects = {
    -- Table to hold our active projectiles
    ActiveProjectiles = {},
    Cooldown = 0.5, -- Time between attacks
    TimeSinceLastAttack = 0.0
}

local AttackMetaData = {
    Parent = nil,
    Entities = {},
    NextEntityIndex = 0,
    EntityCount = 10
}

function AttackEffects:OnCreate()
    AttackMetaData.Parent = Scene.CreateEntity("Attack1Parent")
    for i = 0, AttackMetaData.EntityCount-1 do
        AttackMetaData.Entities[i] = Scene.CreateEntity("Fireball-"..i, AttackMetaData.Parent)
        AttackMetaData.Entities[i]:AddSpriteRenderer()
        AttackMetaData.Entities[i].SpriteRenderer.Texture = "textures/Fire-Spell.png" -- Assuming you have a fireball sprite
        AttackMetaData.Entities[i].SpriteRenderer.IsSubTexture = true
        AttackMetaData.Entities[i].SpriteRenderer.SpriteHeight = 1
        AttackMetaData.Entities[i].SpriteRenderer.SpriteWidth = 8
        AttackMetaData.Entities[i].SpriteRenderer.XSpriteIndex = 0
        AttackMetaData.Entities[i].SpriteRenderer.YSpriteIndex = 0
        AttackMetaData.Entities[i]:SetEnabled(false)
    end
    print("Projectile Manager Initialized")
end

-- The Player script will call this
function AttackEffects:Emit(startPos, direction, speed, lifetime)
    if self.TimeSinceLastAttack < self.Cooldown then
        return -- Still in cooldown, ignore this attack
    end
    self.TimeSinceLastAttack = 0.0 -- Reset cooldown timer
    
    local projEntity = AttackMetaData.Entities[AttackMetaData.NextEntityIndex]
    AttackMetaData.NextEntityIndex = (AttackMetaData.NextEntityIndex + 1) % AttackMetaData.EntityCount
    
    projEntity:SetEnabled(true)
    projEntity:SetPosition(startPos)
    local AspectRatio = (projEntity.SpriteRenderer.TextureWidth / 8) / projEntity.SpriteRenderer.TextureHeight
    projEntity:SetScale(Vec3.new(-0.5 * direction.x * AspectRatio, 0.5, 1.0)) -- Flip based on direction

    -- Store it in our tracking table
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
            projectile.Entity:SetEnabled(false)
            
            -- 2. Remove it from our Lua tracking table
            table.remove(self.ActiveProjectiles, i)
        else
            -- If using OPTION A (Manual Movement), update position here:
            local currentPos = projectile.Entity:GetPosition()
            local newPos = Vec3.new(currentPos.x + (projectile.Velocity.x * ts), currentPos.y + (projectile.Velocity.y * ts), currentPos.z)
            projectile.Entity:SetPosition(newPos)

            projectile.Entity.SpriteRenderer.XSpriteIndex = math.floor((projectile.TimeLeft * 10) % projectile.Entity.SpriteRenderer.SpriteWidth) -- Simple animation logic
        end
    end
end

return AttackEffects