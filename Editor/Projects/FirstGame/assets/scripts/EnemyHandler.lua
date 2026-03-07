local ParticleSystem = require("scripts.ParticleSystem")

local EnemiesHandler = {
    ActiveEnemies = {},
    SpawnRate = 0.5,
    SpawnVariance = 0.5,
    Cooldown = 0.0,
    LeftSpawnPoint = Vec3.new(-8.0, -1.0, 0),
    RightSpawnPoint = Vec3.new(8.0, -1.0, 0),
    Speed = 1.0,
    OnEnemyDeathCallback = nil
}

local EnemiesMetaData = {
    Parent = nil,
    Entities = {},
    NextEntityIndex = 1,
    EntityCount = 20
}

local EnemyDirections = {}
function ChangeEnemyDirection(entityId, direction)
    EnemyDirections[entityId] = direction
end
function GetEnemyDirection(entityId)
    return EnemyDirections[entityId]
end

function EnemyCollisionBeginHandler(enemyEntity, otherEntity)
    if otherEntity:GetName() == "Fireball" then
        for i = 1, 20 do
            ParticleSystem:Emit(enemyEntity:GetPosition(), Vec2.new((Random:Float()*2-1) * 5, (Random:Float()*2-1) * 5))
        end
    end
end

function EnemiesHandler:OnCreate()
    ParticleSystem:OnCreate()
    EnemiesMetaData.Parent = GetEntityByTag("Enemy Manager")
    EnemiesMetaData.Parent:SetPosition(Vec3.new(0.0, 0.0, 0.0))
    local origEnemy = EnemiesMetaData.Parent.Relation.FirstChild
    origEnemy:SetEnabled(false)
    EnemyDirections[origEnemy:GetUUID()] = 1
    table.insert(EnemiesMetaData.Entities, origEnemy)
    for i = 2, EnemiesMetaData.EntityCount do
        local enemy = Scene.DuplicateEntity(origEnemy)
        enemy:SetName("Enemy clone-"..i-1)
        enemy:SetEnabled(false)
        EnemyDirections[enemy:GetUUID()] = 1
        table.insert(EnemiesMetaData.Entities, enemy)
    end

    RegisterFunction("ChangeEnemyDirection", ChangeEnemyDirection)
    RegisterFunction("GetEnemyDirection", GetEnemyDirection)
    RegisterFunction("EnemyCollisionBeginHandler", EnemyCollisionBeginHandler)

end

function EnemiesHandler:OnUpdate(ts)
    ParticleSystem:OnUpdate(ts)
    self.Cooldown = self.Cooldown - ts

    for i = 1, #self.ActiveEnemies do
        local enemy = self.ActiveEnemies[i]
        enemy.TimeSinceBorn = enemy.TimeSinceBorn + ts
    end
    
    if self.Cooldown <= 0 then
        
        local enemyEntity = EnemiesMetaData.Entities[EnemiesMetaData.NextEntityIndex]
        EnemiesMetaData.NextEntityIndex = EnemiesMetaData.NextEntityIndex % EnemiesMetaData.EntityCount + 1
        print("Spawning")
        enemyEntity:SetEnabled(true)
        Physics.SetEnabled(enemyEntity, true)
        -- enemyEntity.SpriteRenderer.Color = Vec4.new(Random:Float(), Random:Float(), Random:Float(), 1.0)
        local direction = -1
        if Random:Float() > 0.5 then
            direction = 1
        end
        if direction == -1 then
            enemyEntity:SetPosition(self.LeftSpawnPoint)
            enemyEntity.SpriteRenderer.FlipX = false
        else
            enemyEntity:SetPosition(self.RightSpawnPoint)
            enemyEntity.SpriteRenderer.FlipX = true
        end
        ChangeEnemyDirection(enemyEntity:GetUUID(), -direction)
        
        -- Physics.SetLinearVelocity(enemyEntity, Vec2.new(-direction * self.Speed, 0))
        -- Store it in our tracking table
        table.insert(self.ActiveEnemies, {
            Entity = enemyEntity,
            TimeSinceBorn = 0.0,
            Velocity = Vec2.new(-direction * self.Speed, 0)
        })

        local NextEnemySpawnTime = 1 / self.SpawnRate
        NextEnemySpawnTime = NextEnemySpawnTime + (Random:Float() * NextEnemySpawnTime * self.SpawnVariance)
        self.Cooldown = NextEnemySpawnTime
    end
end

-- function EnemiesHandler:OnCollisionBegin(otherEntity)
--     if otherEntity:GetName() == "Fireball" then
--         print("Enemy hit.")
--         self.Entity:SetEnabled(false)
--         otherEntity:SetEnabled(false)
--     end
-- end

function EnemiesHandler:OnDestroy()

end

return EnemiesHandler