#include "../gfx/shader.h"
#include "../gfx/texture.h"
#include "../core/types.h"
#include "../math/mat.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

NS_BEGIN

namespace ImGuiSystem {
	IMGUI_API bool Init(SDL_Window* window);
	IMGUI_API void Shutdown();
	IMGUI_API void NewFrame(u32 ww = 0, u32 wh = 0);
	IMGUI_API void Render();
	IMGUI_API bool ProcessEvent(SDL_Event* event);

	// Use if you want to reset your rendering device without losing ImGui state.
	IMGUI_API void InvalidateDeviceObjects();
	IMGUI_API bool CreateDeviceObjects();
}

NS_END
