#pragma once
#include "IDisplay.h"
#include <glad/glad.h>
namespace ENGINE_EDITOR {
class GameDisplay : public IDisplay {
    GLuint RenderGame(int width, int height);
protected:
    void DrawToolbar() override;
public:
    GameDisplay() = default;
    void Draw() override;
};
}
