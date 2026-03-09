local AudioTest = {}

function AudioTest:OnCreate()
    self.Entity.AudioSources:AddSource("explosion")
    self.Entity.AudioSources["explosion"].AudioFile = "sounds/explosion.wav"
    print("AudioTest created")
end

function AudioTest:OnUpdate(ts)
    if Input.IsKeyPressed(Key.Space) then
        self.Entity.AudioSources["laser"]:Play()
    end
    if Input.IsKeyPressed(Key.E) then
        self.Entity.AudioSources["explosion"]:Play()
    end
end

return AudioTest