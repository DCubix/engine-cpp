#ifndef IMGUI_EXT
#define IMGUI_EXT

#include <string>
#include <list>
#include <vector>

#include "../core/filesys.h"

namespace ImGui {

IMGUI_API bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);
IMGUI_API bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values);

IMGUI_API void ShutdownDock();
IMGUI_API void RootDock(const ImVec2& pos, const ImVec2& size);
IMGUI_API bool BeginDock(const char* label, bool* opened = nullptr, ImGuiWindowFlags extra_flags = 0);
IMGUI_API void EndDock();
IMGUI_API void SetDockActive();
IMGUI_API void LoadDock();
IMGUI_API void SaveDock();

IMGUI_API bool FileDialog(
		const char* vName,
		const std::string& filters,
		std::string vPath = "",
		std::string vDefaultFileName = ""
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
	std::string GetFilepathName();
	std::string GetCurrentPath();
	std::string GetCurrentFileName();

private:
	void ScanDir(std::string vPath);
	void SetCurrentDir(std::string vPath);
	void ComposeNewPath(std::vector<std::string>::iterator vIter);

	std::vector<VFSFileInfo> m_FileList;
	std::string m_SelectedFileName;
	std::string m_CurrentPath;
	std::vector<std::string> m_CurrentPath_Decomposition;
	int m_currentExtIndex;

	static uptr<ImGuiFileDialog> g_instance;
};

}

#endif // IMGUI_EXT
