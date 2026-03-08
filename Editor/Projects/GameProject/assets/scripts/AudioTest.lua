local AudioTest = {}

function AudioTest:OnCreate()
    print("AudioTest created")
end

function AudioTest:OnUpdate(ts)
    if Input.IsKeyPressed(Key.Space) then
        self.Entity.AudioSource:StartSound()
    end
end

return AudioTest