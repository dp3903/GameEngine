local Player = {}

-- Properties
Player.Speed = 5.0

function Player:OnCreate()
    print("Player Created!")
    SetGlobal("Health", 78.42)
    SetGlobal("PlayerName", "Hero123")
    SetGlobal("IsAlive", true)
    SetGlobal("Position", Vec3.new(10.0, 5.0, 0.0))
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
   
end

function Player:OnDestroy()
    print("Player Destroyed!")
end

return Player