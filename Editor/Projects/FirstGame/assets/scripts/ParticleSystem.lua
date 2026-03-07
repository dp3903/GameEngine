local ParticleSystem = {
    Root = nil,
    maxParticles = 50,
    UsePhysics = false,
    Lifetime = 2.0,
    StartColor = Vec4.new(1, 0.8, 0, 1), -- Orange color
    EndColor = Vec4.new(1, 0, 0, 0), -- Red color, fully transparent
    ActiveParticles = {}
}

local ParticleMetadata = {
    particles = {},
    NextParticleIndex = 1
}

function ParticleSystem:OnCreate()
    self.Root = Scene.CreateEntity("ParticleSystemRoot")
    for i = 1, self.maxParticles do
        local p1 = Scene.CreateEntity("particle", self.Root)
        p1:AddCircleRenderer()
        if self.UsePhysics then
            local body = p1:AddRigidbody()
            body.Type = BodyType.Dynamic
            local collider = p1:AddCircleCollider()
            collider.Restitution = 0.5
            collider.Friction = 0.0
            collider.Density = 0.01
            Physics.SetGravityScale(p1, 0.3) -- low gravity for particles
            p1:RebuildFixtures()
        end
        p1:SetScale(Vec3.new(0.1, 0.1, 0.1)) -- Small size for particles
        p1:SetEnabled(false) -- Start disabled, will be enabled when emitted
        table.insert(ParticleMetadata.particles, {
            id = i,
            entity = p1,
            lifetime = 0,
            velocity = Vec2.new(0, 0),
            position = Vec3.new(0, 0, 0)
        })
    end
    print("ParticleSystem created")
end

function ParticleSystem:OnUpdate(ts)
    for i, particle in pairs(self.ActiveParticles) do
        particle.lifetime = particle.lifetime - ts
        particle.entity.CircleRenderer.Color = self.StartColor * (particle.lifetime / self.Lifetime) + self.EndColor * (1 - particle.lifetime / self.Lifetime)
        if not self.UsePhysics then
            particle.position.x = particle.position.x + particle.velocity.x * ts
            particle.position.y = particle.position.y + particle.velocity.y * ts
            particle.entity:SetPosition(particle.position)
        end
        if particle.lifetime <= 0 then
            -- print("Disabling particle " .. particle.id)
            particle.entity:SetEnabled(false)
            self.ActiveParticles[particle.id] = nil
        end
    end
end

function ParticleSystem:Emit(position, velocity)
    local particle = ParticleMetadata.particles[ParticleMetadata.NextParticleIndex]
    ParticleMetadata.NextParticleIndex = ParticleMetadata.NextParticleIndex % self.maxParticles + 1
    if not self.ActiveParticles[particle.id] then
        particle.entity:SetEnabled(true)
        particle.position = position
        particle.entity:SetPosition(position)
        if self.UsePhysics then
            Physics.SetLinearVelocity(particle.entity, velocity)
        end
        particle.velocity = velocity
        particle.lifetime = self.Lifetime
        self.ActiveParticles[particle.id] = particle
    end
end

return ParticleSystem