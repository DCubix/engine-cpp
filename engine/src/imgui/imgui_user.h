#ifndef IMGUI_EXT
#define IMGUI_EXT

#include "../core/filesys.h"
#include "../core/types.h"

namespace ImGui {

IMGUI_API bool Combo(const char* label, int* currIndex, Vector<String>& values);
IMGUI_API bool ListBox(const char* label, int* currIndex, Vector<String>& values);

IMGUI_API void ShutdownDock();
IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
IMGUI_API bool BeginDock(const char* label, bool* opened = nullptr, ImGuiWindowFlags extra_flags = 0, const ImVec2& default_size = ImVec2(-1, -1));
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void SaveDock();
IMGUI_API void LoadDock();

IMGUI_API bool FileDialog(
		const char* vName,
		const String& filters,
		String vPath = "",
		String vDefaultFileName = ""
);

IMGUI_API bool FileDialogOk();
IMGUI_API std::string GetFileDialogFileName();

#define MAX_FILE_DIALOG_NAME_BUFFER 1024

class ImGuiFileDialog {
public:
	bool IsOk;

	static ImGuiFileDialog& instance() {
		return *g_instance.get();
	}

	ImGuiFileDialog() : m_currentExtIndex(0) {}
	~ImGuiFileDialog() = default;

	bool FileDialog(const char* vName, const std::string& vFilters = "", std::string vPath = "", std::string vDefaultFileName = "");
	String GetFilepathName();
	String GetCurrentPath();
	String GetCurrentFileName();

private:
	void ScanDir(String vPath);
	void SetCurrentDir(String vPath);
	void ComposeNewPath(Vector<String>::iterator vIter);

	Vector<VFSFileInfo> m_FileList;
	String m_SelectedFileName;
	String m_CurrentPath;
	Vector<String> m_CurrentPath_Decomposition;
	int m_currentExtIndex;

	static uptr<ImGuiFileDialog> g_instance;
};

}

namespace ImGuizmo {
	// call inside your own window and before Manipulate() in order to draw gizmo to that window.
	IMGUI_API void SetDrawlist();

	// call BeginFrame right after ImGui_XXXX_NewFrame();
	IMGUI_API void BeginFrame();

	// return true if mouse cursor is over any gizmo control (axis, plan or screen component)
	IMGUI_API bool IsOver();

	// return true if mouse IsOver or if the gizmo is in moving state
	IMGUI_API bool IsUsing();

	// enable/disable the gizmo. Stay in the state until next call to Enable.
	// gizmo is rendered with gray half transparent color when disabled
	IMGUI_API void Enable(bool enable);

	// These functions have some numerical stability issues for now. Use with caution.
	IMGUI_API void DecomposeMatrixToComponents(const float *matrix, float *translation, float *rotation, float *scale);
	IMGUI_API void RecomposeMatrixFromComponents(const float *translation, const float *rotation, const float *scale, float *matrix);

	IMGUI_API void SetRect(float x, float y, float width, float height);
	// default is false
	IMGUI_API void SetOrthographic(bool isOrthographic);

	// Render a cube with face color corresponding to face normal. Usefull for debug/tests
	IMGUI_API void DrawCube(const float *view, const float *projection, const float *matrix);
	IMGUI_API void DrawGrid(const float *view, const float *projection, const float *matrix, const float gridSize);

	// call it when you want a gizmo
	// Needs view and projection matrices.
	// matrix parameter is the source matrix (where will be gizmo be drawn) and might be transformed by the function. Return deltaMatrix is optional
	// translation is applied in world space
	enum OPERATION {
		TRANSLATE,
		ROTATE,
		SCALE,
		BOUNDS,
	};

	enum MODE {
		LOCAL,
		WORLD
	};

	IMGUI_API void Manipulate(const float *view, const float *projection, OPERATION operation, MODE mode, float *matrix, float *deltaMatrix = 0, float *snap = 0, float *localBounds = NULL, float *boundsSnap = NULL);
};

#endif // IMGUI_EXT
