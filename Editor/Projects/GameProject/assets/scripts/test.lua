local Player = {}

-- Properties
Player.Speed = 5.0

function Greeting(name, age)
    print("Hello from the registered function! My name is " .. name .. " and I am " .. age .. " years old.")
end


function Player:OnCreate()
    print("Player Created!")
    SetGlobal("Health", 78.42)
    SetGlobal("PlayerName", "Hero123")
    SetGlobal("IsAlive", true)
    SetGlobal("Position", Vec3.new(10.0, 5.0, 0.0))

    RegisterFunction("Greeting", Greeting)
end

function Player:OnUpdate(ts)
    if Input.IsKeyPressed(Key.R) then
        self.Entity.SpriteRenderer.Color = Vec4.new(Random.Float(), Random.Float(), Random.Float(), 1.0)
    end
    if Input.IsKeyPressed(Key.D1) then
        RequestSceneChange("scenes/demo.ssfmt")
    end
    if Input.IsKeyPressed(Key.D2) then
        RequestSceneChange("scenes/Example.ssfmt")
    end
    if Input.IsKeyPressed(Key.F) then
        CallFunction("Greeting", "Alice", 25)
    end
    if Input.IsKeyPressed(Key.W) then
        Physics.ApplyLinearImpulse(GetEntityByTag("Ball"), Vec2.new(0.0, 0.5), Vec2.new(0.0, 0.0), true)
    end
end

function Player:OnCollisionBegin(other)
    print("Player collided with entity:", other:GetName())
end

function Player:OnDestroy()
    print("Player Destroyed!")
end

return Player