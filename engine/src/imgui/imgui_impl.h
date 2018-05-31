#include "../gfx/shader.h"
#include "../core/types.h"
#include "../math/mat.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

NS_BEGIN

IMGUI_API bool ImGui_Impl_Init(SDL_Window* window);
IMGUI_API void ImGui_Impl_Shutdown();
IMGUI_API void ImGui_Impl_NewFrame(SDL_Window* window);
IMGUI_API void ImGui_Impl_RenderDrawData(ImDrawData* draw_data);
IMGUI_API bool ImGui_Impl_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_Impl_InvalidateDeviceObjects();
IMGUI_API bool ImGui_Impl_CreateDeviceObjects();

NS_END
