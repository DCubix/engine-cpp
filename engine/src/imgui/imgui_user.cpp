#include "imgui.h"
#include "imgui_user.h"
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui_internal.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#define ASSERT(x) IM_ASSERT(x)

#if defined(WIN32)
#include <windows.h>
#include <direct.h>
#include <tchar.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#define GetCurrentDir getcwd
#endif

#define IM_ARRAYSIZE(_ARR) ((int)(sizeof(_ARR)/sizeof(*_ARR)))

#if defined(ICON_FA_CARET_DOWN)
#define CARET_DOWN ICON_FA_CARET_DOWN
#else
#define CARET_DOWN "v"
#endif

using namespace std;
using namespace ImGui;

namespace ImGui
{

struct DockContext
{
	enum Slot_
	{
		Slot_Left,
		Slot_Right,
		Slot_Top,
		Slot_Bottom,
		Slot_Tab,

		Slot_Float,
		Slot_None
	};


	enum EndAction_
	{
		EndAction_None,
		EndAction_Panel,
		EndAction_End,
		EndAction_EndChild
	};


	enum Status_
	{
		Status_Docked,
		Status_Float,
		Status_Dragged
	};


	struct Dock
	{
		Dock()
			: id(0)
			, next_tab(nullptr)
			, prev_tab(nullptr)
			, parent(nullptr)
			, pos(0, 0)
			, size(-1, -1)
			, active(true)
			, status(Status_Float)
			, label(nullptr)
			, opened(false)
			, first(true)
			, last_frame(0)
		{
			location[0] = 0;
			children[0] = children[1] = nullptr;
		}


		~Dock() { MemFree(label); }


		ImVec2 getMinSize() const
		{
			if (!children[0]) return ImVec2(16, 16 + GetTextLineHeightWithSpacing());

			ImVec2 s0 = children[0]->getMinSize();
			ImVec2 s1 = children[1]->getMinSize();
			return isHorizontal() ? ImVec2(s0.x + s1.x, ImMax(s0.y, s1.y))
								  : ImVec2(ImMax(s0.x, s1.x), s0.y + s1.y);
		}


		bool isHorizontal() const { return children[0]->pos.x < children[1]->pos.x; }


		void setParent(Dock* dock)
		{
			parent = dock;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->parent = dock;
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->parent = dock;
		}


		Dock& getSibling()
		{
			IM_ASSERT(parent);
			if (parent->children[0] == &getFirstTab()) return *parent->children[1];
			return *parent->children[0];
		}


		Dock& getFirstTab()
		{
			Dock* tmp = this;
			while (tmp->prev_tab) tmp = tmp->prev_tab;
			return *tmp;
		}


		void setActive()
		{
			active = true;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab) tmp->active = false;
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab) tmp->active = false;
		}


		bool hasChildren() const { return children[0] != nullptr; }


		void setChildrenPosSize(const ImVec2& _pos, const ImVec2& _size)
		{
			ImVec2 s = children[0]->size;
			if (isHorizontal())
			{
				s.y = _size.y;
				s.x = (float)int(
					_size.x * children[0]->size.x / (children[0]->size.x + children[1]->size.x));
				if (s.x < children[0]->getMinSize().x)
				{
					s.x = children[0]->getMinSize().x;
				}
				else if (_size.x - s.x < children[1]->getMinSize().x)
				{
					s.x = _size.x - children[1]->getMinSize().x;
				}
				children[0]->setPosSize(_pos, s);

				s.x = _size.x - children[0]->size.x;
				ImVec2 p = _pos;
				p.x += children[0]->size.x;
				children[1]->setPosSize(p, s);
			}
			else
			{
				s.x = _size.x;
				s.y = (float)int(
					_size.y * children[0]->size.y / (children[0]->size.y + children[1]->size.y));
				if (s.y < children[0]->getMinSize().y)
				{
					s.y = children[0]->getMinSize().y;
				}
				else if (_size.y - s.y < children[1]->getMinSize().y)
				{
					s.y = _size.y - children[1]->getMinSize().y;
				}
				children[0]->setPosSize(_pos, s);

				s.y = _size.y - children[0]->size.y;
				ImVec2 p = _pos;
				p.y += children[0]->size.y;
				children[1]->setPosSize(p, s);
			}
		}


		void setPosSize(const ImVec2& _pos, const ImVec2& _size)
		{
			size = _size;
			pos = _pos;
			for (Dock* tmp = prev_tab; tmp; tmp = tmp->prev_tab)
			{
				tmp->size = _size;
				tmp->pos = _pos;
			}
			for (Dock* tmp = next_tab; tmp; tmp = tmp->next_tab)
			{
				tmp->size = _size;
				tmp->pos = _pos;
			}

			if (!hasChildren()) return;
			setChildrenPosSize(_pos, _size);
		}


		char* label;
		ImU32 id;
		Dock* next_tab;
		Dock* prev_tab;
		Dock* children[2];
		Dock* parent;
		bool active;
		ImVec2 pos;
		ImVec2 size;
		Status_ status;
		char location[16];
		bool opened;
		bool first;
		int last_frame;
	};


	ImVector<Dock*> m_docks;
	ImVec2 m_drag_offset;
	Dock* m_current = nullptr;
	int m_last_frame = 0;
	EndAction_ m_end_action;
	bool m_is_begin_open = false;


	~DockContext() {}


	Dock& getDock(const char* label, bool opened, const ImVec2& default_size)
	{
		ImU32 id = ImHash(label, 0);
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (m_docks[i]->id == id) return *m_docks[i];
		}

		Dock* new_dock = (Dock*)MemAlloc(sizeof(Dock));
		IM_PLACEMENT_NEW(new_dock) Dock();
		m_docks.push_back(new_dock);
		new_dock->label = ImStrdup(label);
		IM_ASSERT(new_dock->label);
		new_dock->id = id;
		new_dock->setActive();
		new_dock->status = Status_Float;
		new_dock->pos = ImVec2(0, 0);
		new_dock->size.x = default_size.x < 0 ? GetIO().DisplaySize.x : default_size.x;
		new_dock->size.y = default_size.y < 0 ? GetIO().DisplaySize.y : default_size.y;
		new_dock->opened = opened;
		new_dock->first = true;
		new_dock->location[0] = 0;
		return *new_dock;
	}


	void putInBackground()
	{
		ImGuiWindow* win = GetCurrentWindow();
		ImGuiContext& g = *GImGui;
		if (g.Windows[0] == win) return;

		for (int i = 0; i < g.Windows.Size; i++)
		{
			if (g.Windows[i] == win)
			{
				for (int j = i - 1; j >= 0; --j)
				{
					g.Windows[j + 1] = g.Windows[j];
				}
				g.Windows[0] = win;
				break;
			}
		}
	}


	void splits()
	{
		if (GetFrameCount() == m_last_frame) return;
		m_last_frame = GetFrameCount();

		putInBackground();

		ImU32 color = GetColorU32(ImGuiCol_Button);
		ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
		ImDrawList* draw_list = GetWindowDrawList();
		ImGuiIO& io = GetIO();
		for (int i = 0; i < m_docks.size(); ++i)
		{
			Dock& dock = *m_docks[i];
			if (!dock.hasChildren()) continue;

			PushID(i);
			if (!IsMouseDown(0)) dock.status = Status_Docked;

			ImVec2 size = dock.children[0]->size;
			ImVec2 dsize(0, 0);
			SetCursorScreenPos(dock.children[1]->pos);
			ImVec2 min_size0 = dock.children[0]->getMinSize();
			ImVec2 min_size1 = dock.children[1]->getMinSize();
			if (dock.isHorizontal())
			{
				InvisibleButton("split", ImVec2(3, dock.size.y));
				if (dock.status == Status_Dragged) dsize.x = io.MouseDelta.x;
				dsize.x = -ImMin(-dsize.x, dock.children[0]->size.x - min_size0.x);
				dsize.x = ImMin(dsize.x, dock.children[1]->size.x - min_size1.x);
			}
			else
			{
				InvisibleButton("split", ImVec2(dock.size.x, 3));
				if (dock.status == Status_Dragged) dsize.y = io.MouseDelta.y;
				dsize.y = -ImMin(-dsize.y, dock.children[0]->size.y - min_size0.y);
				dsize.y = ImMin(dsize.y, dock.children[1]->size.y - min_size1.y);
			}
			ImVec2 new_size0 = dock.children[0]->size + dsize;
			ImVec2 new_size1 = dock.children[1]->size - dsize;
			ImVec2 new_pos1 = dock.children[1]->pos + dsize;
			dock.children[0]->setPosSize(dock.children[0]->pos, new_size0);
			dock.children[1]->setPosSize(new_pos1, new_size1);

			if (IsItemHovered() && IsMouseClicked(0))
			{
				dock.status = Status_Dragged;
			}

			draw_list->AddRectFilled(
				GetItemRectMin(), GetItemRectMax(), IsItemHovered() ? color_hovered : color);
			PopID();
		}
	}


	void beginPanel()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
								 ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus;
		Dock* root = getRootDock();
		if (root)
		{
			SetNextWindowPos(root->pos);
			SetNextWindowSize(root->size);
		}
		else
		{
			SetNextWindowPos(ImVec2(0, 0));
			SetNextWindowSize(GetIO().DisplaySize);
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		Begin("###DockPanel", nullptr, flags);
		splits();
	}


	void endPanel()
	{
		End();
		ImGui::PopStyleVar();
	}


	Dock* getDockAt(const ImVec2& pos) const
	{
		for (int i = 0; i < m_docks.size(); ++i)
		{
			Dock& dock = *m_docks[i];
			if (dock.hasChildren()) continue;
			if (dock.status != Status_Docked) continue;
			if (IsMouseHoveringRect(dock.pos, dock.pos + dock.size, false))
			{
				return &dock;
			}
		}

		return nullptr;
	}


	static ImRect getDockedRect(const ImRect& rect, Slot_ dock_slot)
	{
		ImVec2 size = rect.GetSize();
		switch (dock_slot)
		{
			default: return rect;
			case Slot_Top: return ImRect(rect.Min, rect.Min + ImVec2(size.x, size.y * 0.5f));
			case Slot_Right: return ImRect(rect.Min + ImVec2(size.x * 0.5f, 0), rect.Max);
			case Slot_Bottom: return ImRect(rect.Min + ImVec2(0, size.y * 0.5f), rect.Max);
			case Slot_Left: return ImRect(rect.Min, rect.Min + ImVec2(size.x * 0.5f, rect.GetSize().y));
		}
	}


	static ImRect getSlotRect(ImRect parent_rect, Slot_ dock_slot)
	{
		ImVec2 size = parent_rect.Max - parent_rect.Min;
		ImVec2 center = parent_rect.Min + size * 0.5f;
		switch (dock_slot)
		{
			default: return ImRect(center - ImVec2(20, 20), center + ImVec2(20, 20));
			case Slot_Top: return ImRect(center + ImVec2(-20, -50), center + ImVec2(20, -30));
			case Slot_Right: return ImRect(center + ImVec2(30, -20), center + ImVec2(50, 20));
			case Slot_Bottom: return ImRect(center + ImVec2(-20, +30), center + ImVec2(20, 50));
			case Slot_Left: return ImRect(center + ImVec2(-50, -20), center + ImVec2(-30, 20));
		}
	}


	static ImRect getSlotRectOnBorder(ImRect parent_rect, Slot_ dock_slot)
	{
		ImVec2 size = parent_rect.Max - parent_rect.Min;
		ImVec2 center = parent_rect.Min + size * 0.5f;
		switch (dock_slot)
		{
			case Slot_Top:
				return ImRect(ImVec2(center.x - 20, parent_rect.Min.y + 10),
					ImVec2(center.x + 20, parent_rect.Min.y + 30));
			case Slot_Left:
				return ImRect(ImVec2(parent_rect.Min.x + 10, center.y - 20),
					ImVec2(parent_rect.Min.x + 30, center.y + 20));
			case Slot_Bottom:
				return ImRect(ImVec2(center.x - 20, parent_rect.Max.y - 30),
					ImVec2(center.x + 20, parent_rect.Max.y - 10));
			case Slot_Right:
				return ImRect(ImVec2(parent_rect.Max.x - 30, center.y - 20),
					ImVec2(parent_rect.Max.x - 10, center.y + 20));
			default: ASSERT(false);
		}
		IM_ASSERT(false);
		return ImRect();
	}


	Dock* getRootDock()
	{
		for (int i = 0; i < m_docks.size(); ++i)
		{
			if (!m_docks[i]->parent &&
				(m_docks[i]->status == Status_Docked || m_docks[i]->children[0]))
			{
				return m_docks[i];
			}
		}
		return nullptr;
	}


	bool dockSlots(Dock& dock, Dock* dest_dock, const ImRect& rect, bool on_border)
	{
		ImDrawList* canvas = GetWindowDrawList();
		ImU32 color = GetColorU32(ImGuiCol_Button);
		ImU32 color_hovered = GetColorU32(ImGuiCol_ButtonHovered);
		ImVec2 mouse_pos = GetIO().MousePos;
		for (int i = 0; i < (on_border ? 4 : 5); ++i)
		{
			ImRect r =
				on_border ? getSlotRectOnBorder(rect, (Slot_)i) : getSlotRect(rect, (Slot_)i);
			bool hovered = r.Contains(mouse_pos);
			canvas->AddRectFilled(r.Min, r.Max, hovered ? color_hovered : color);
			if (!hovered) continue;

			if (!IsMouseDown(0))
			{
				doDock(dock, dest_dock ? dest_dock : getRootDock(), (Slot_)i);
				return true;
			}
			ImRect docked_rect = getDockedRect(rect, (Slot_)i);
			canvas->AddRectFilled(docked_rect.Min, docked_rect.Max, GetColorU32(ImGuiCol_Button));
		}
		return false;
	}


	void handleDrag(Dock& dock)
	{
		Dock* dest_dock = getDockAt(GetIO().MousePos);

		Begin("##Overlay",
			NULL,
			ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_AlwaysAutoResize);
		ImDrawList* canvas = GetWindowDrawList();

		canvas->PushClipRectFullScreen();

		ImU32 docked_color = GetColorU32(ImGuiCol_FrameBg);
		docked_color = (docked_color & 0x00ffFFFF) | 0x80000000;
		dock.pos = GetIO().MousePos - m_drag_offset;
		if (dest_dock)
		{
			if (dockSlots(dock,
					dest_dock,
					ImRect(dest_dock->pos, dest_dock->pos + dest_dock->size),
					false))
			{
				canvas->PopClipRect();
				End();
				return;
			}
		}
		if (dockSlots(dock, nullptr, ImRect(ImVec2(0, 0), GetIO().DisplaySize), true))
		{
			canvas->PopClipRect();
			End();
			return;
		}
		canvas->AddRectFilled(dock.pos, dock.pos + dock.size, docked_color);
		canvas->PopClipRect();

		if (!IsMouseDown(0))
		{
			dock.status = Status_Float;
			dock.location[0] = 0;
			dock.setActive();
		}

		End();
	}


	void fillLocation(Dock& dock)
	{
		if (dock.status == Status_Float) return;
		char* c = dock.location;
		Dock* tmp = &dock;
		while (tmp->parent)
		{
			*c = getLocationCode(tmp);
			tmp = tmp->parent;
			++c;
		}
		*c = 0;
	}


	void doUndock(Dock& dock)
	{
		if (dock.prev_tab)
			dock.prev_tab->setActive();
		else if (dock.next_tab)
			dock.next_tab->setActive();
		else
			dock.active = false;
		Dock* container = dock.parent;

		if (container)
		{
			Dock& sibling = dock.getSibling();
			if (container->children[0] == &dock)
			{
				container->children[0] = dock.next_tab;
			}
			else if (container->children[1] == &dock)
			{
				container->children[1] = dock.next_tab;
			}

			bool remove_container = !container->children[0] || !container->children[1];
			if (remove_container)
			{
				if (container->parent)
				{
					Dock*& child = container->parent->children[0] == container
									   ? container->parent->children[0]
									   : container->parent->children[1];
					child = &sibling;
					child->setPosSize(container->pos, container->size);
					child->setParent(container->parent);
				}
				else
				{
					if (container->children[0])
					{
						container->children[0]->setParent(nullptr);
						container->children[0]->setPosSize(container->pos, container->size);
					}
					if (container->children[1])
					{
						container->children[1]->setParent(nullptr);
						container->children[1]->setPosSize(container->pos, container->size);
					}
				}
				for (int i = 0; i < m_docks.size(); ++i)
				{
					if (m_docks[i] == container)
					{
						m_docks.erase(m_docks.begin() + i);
						break;
					}
				}
				container->~Dock();
				MemFree(container);
			}
		}
		if (dock.prev_tab) dock.prev_tab->next_tab = dock.next_tab;
		if (dock.next_tab) dock.next_tab->prev_tab = dock.prev_tab;
		dock.parent = nullptr;
		dock.prev_tab = dock.next_tab = nullptr;
	}


	void drawTabbarListButton(Dock& dock)
	{
		if (!dock.next_tab) return;

		ImDrawList* draw_list = GetWindowDrawList();
		if (InvisibleButton("list", ImVec2(16, 16)))
		{
			OpenPopup("tab_list_popup");
		}
		if (BeginPopup("tab_list_popup"))
		{
			Dock* tmp = &dock;
			while (tmp)
			{
				bool dummy = false;
				if (Selectable(tmp->label, &dummy))
				{
					tmp->setActive();
				}
				tmp = tmp->next_tab;
			}
			EndPopup();
		}

		bool hovered = IsItemHovered();
		ImVec2 min = GetItemRectMin();
		ImVec2 max = GetItemRectMax();
		ImVec2 center = (min + max) * 0.5f;
		ImU32 text_color = GetColorU32(ImGuiCol_Text);
		ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
		draw_list->AddRectFilled(ImVec2(center.x - 4, min.y + 3),
			ImVec2(center.x + 4, min.y + 5),
			hovered ? color_active : text_color);
		draw_list->AddTriangleFilled(ImVec2(center.x - 4, min.y + 7),
			ImVec2(center.x + 4, min.y + 7),
			ImVec2(center.x, min.y + 12),
			hovered ? color_active : text_color);
	}


	bool tabbar(Dock& dock, bool close_button)
	{
		float tabbar_height = 2 * GetTextLineHeightWithSpacing();
		ImVec2 size(dock.size.x, tabbar_height);
		bool tab_closed = false;

		SetCursorScreenPos(dock.pos);
		char tmp[20];
		ImFormatString(tmp, IM_ARRAYSIZE(tmp), "tabs%d", (int)dock.id);

		if (BeginChild(tmp, size, true))
		{
			Dock* dock_tab = &dock;

			ImDrawList* draw_list = GetWindowDrawList();
			ImU32 color = GetColorU32(ImGuiCol_FrameBg);
			ImU32 color_active = GetColorU32(ImGuiCol_FrameBgActive);
			ImU32 color_hovered = GetColorU32(ImGuiCol_FrameBgHovered);
			ImU32 button_hovered = GetColorU32(ImGuiCol_ButtonHovered);
			ImU32 text_color = GetColorU32(ImGuiCol_Text);
			float line_height = GetTextLineHeightWithSpacing();
			float tab_base;

			drawTabbarListButton(dock);

			while (dock_tab)
			{
				SameLine(0, 15);

				const char* text_end = FindRenderedTextEnd(dock_tab->label);
				ImVec2 size(CalcTextSize(dock_tab->label, text_end).x, line_height);
				if (InvisibleButton(dock_tab->label, size))
				{
					dock_tab->setActive();
				}

				if (IsItemActive() && IsMouseDragging())
				{
					m_drag_offset = GetMousePos() - dock_tab->pos;
					doUndock(*dock_tab);
					dock_tab->status = Status_Dragged;
				}

				if (dock_tab->active && close_button) size.x += 16 + GetStyle().ItemSpacing.x;

				bool hovered = IsItemHovered();
				ImVec2 pos = GetItemRectMin();
				tab_base = pos.y;
				draw_list->PathClear();
				draw_list->PathLineTo(pos + ImVec2(-15, size.y));
				draw_list->PathBezierCurveTo(
					pos + ImVec2(-10, size.y), pos + ImVec2(-5, 0), pos + ImVec2(0, 0), 10);
				draw_list->PathLineTo(pos + ImVec2(size.x, 0));
				draw_list->PathBezierCurveTo(pos + ImVec2(size.x + 5, 0),
					pos + ImVec2(size.x + 10, size.y),
					pos + ImVec2(size.x + 15, size.y),
					10);
				draw_list->PathFillConvex(
					hovered ? color_hovered : (dock_tab->active ? color_active : color));
				draw_list->AddText(pos, text_color, dock_tab->label, text_end);

				if (dock_tab->active && close_button)
				{
					SameLine();
					tab_closed = InvisibleButton("close", ImVec2(16, 16));
					ImVec2 center = (GetItemRectMin() + GetItemRectMax()) * 0.5f;
					if (IsItemHovered())
					{
						draw_list->AddRectFilled(center + ImVec2(-6.0f, -6.0f), center + ImVec2(7.0f, 7.0f), button_hovered);
					}
					draw_list->AddLine(
						center + ImVec2(-3.5f, -3.5f), center + ImVec2(3.5f, 3.5f), text_color);
					draw_list->AddLine(
						center + ImVec2(3.5f, -3.5f), center + ImVec2(-3.5f, 3.5f), text_color);
				}

				dock_tab = dock_tab->next_tab;
			}
			ImVec2 cp(dock.pos.x, tab_base + line_height);
			draw_list->AddLine(cp, cp + ImVec2(dock.size.x, 0), color);
		}
		EndChild();
		return tab_closed;
	}


	static void setDockPosSize(Dock& dest, Dock& dock, Slot_ dock_slot, Dock& container)
	{
		IM_ASSERT(!dock.prev_tab && !dock.next_tab && !dock.children[0] && !dock.children[1]);

		dest.pos = container.pos;
		dest.size = container.size;
		dock.pos = container.pos;
		dock.size = container.size;

		switch (dock_slot)
		{
			case Slot_Bottom:
				dest.size.y *= 0.5f;
				dock.size.y *= 0.5f;
				dock.pos.y += dest.size.y;
				break;
			case Slot_Right:
				dest.size.x *= 0.5f;
				dock.size.x *= 0.5f;
				dock.pos.x += dest.size.x;
				break;
			case Slot_Left:
				dest.size.x *= 0.5f;
				dock.size.x *= 0.5f;
				dest.pos.x += dock.size.x;
				break;
			case Slot_Top:
				dest.size.y *= 0.5f;
				dock.size.y *= 0.5f;
				dest.pos.y += dock.size.y;
				break;
			default: IM_ASSERT(false); break;
		}
		dest.setPosSize(dest.pos, dest.size);

		if (container.children[1]->pos.x < container.children[0]->pos.x ||
			container.children[1]->pos.y < container.children[0]->pos.y)
		{
			Dock* tmp = container.children[0];
			container.children[0] = container.children[1];
			container.children[1] = tmp;
		}
	}


	void doDock(Dock& dock, Dock* dest, Slot_ dock_slot)
	{
		IM_ASSERT(!dock.parent);
		if (!dest)
		{
			dock.status = Status_Docked;
			dock.setPosSize(ImVec2(0, 0), GetIO().DisplaySize);
		}
		else if (dock_slot == Slot_Tab)
		{
			Dock* tmp = dest;
			while (tmp->next_tab)
			{
				tmp = tmp->next_tab;
			}

			tmp->next_tab = &dock;
			dock.prev_tab = tmp;
			dock.size = tmp->size;
			dock.pos = tmp->pos;
			dock.parent = dest->parent;
			dock.status = Status_Docked;
		}
		else if (dock_slot == Slot_None)
		{
			dock.status = Status_Float;
		}
		else
		{
			Dock* container = (Dock*)MemAlloc(sizeof(Dock));
			IM_PLACEMENT_NEW(container) Dock();
			m_docks.push_back(container);
			container->children[0] = &dest->getFirstTab();
			container->children[1] = &dock;
			container->next_tab = nullptr;
			container->prev_tab = nullptr;
			container->parent = dest->parent;
			container->size = dest->size;
			container->pos = dest->pos;
			container->status = Status_Docked;
			container->label = ImStrdup("");

			if (!dest->parent)
			{
			}
			else if (&dest->getFirstTab() == dest->parent->children[0])
			{
				dest->parent->children[0] = container;
			}
			else
			{
				dest->parent->children[1] = container;
			}

			dest->setParent(container);
			dock.parent = container;
			dock.status = Status_Docked;

			setDockPosSize(*dest, dock, dock_slot, *container);
		}
		dock.setActive();
	}


	void rootDock(const ImVec2& pos, const ImVec2& size)
	{
		Dock* root = getRootDock();
		if (!root) return;

		ImVec2 min_size = root->getMinSize();
		ImVec2 requested_size = size;
		root->setPosSize(pos, ImMax(min_size, requested_size));

		static bool is_first_call = true;
		if (!is_first_call)
		{
			for (auto it = m_docks.begin(); it != m_docks.end();)
			{
				auto& dock = *it;
				if (!dock->hasChildren() && dock != root && (ImGui::GetFrameCount() - dock->last_frame) > 1)
				{
					doUndock(*dock);
					dock->~Dock();
					MemFree(dock);
					it = m_docks.erase(it);
				}
				else
					++it;
			}
		}
		is_first_call = false;
	}


	void setDockActive()
	{
		IM_ASSERT(m_current);
		if (m_current) m_current->setActive();
	}


	static Slot_ getSlotFromLocationCode(char code)
	{
		switch (code)
		{
			case '1': return Slot_Left;
			case '2': return Slot_Top;
			case '3': return Slot_Bottom;
			default: return Slot_Right;
		}
	}


	static char getLocationCode(Dock* dock)
	{
		if (!dock) return '0';

		if (dock->parent->isHorizontal())
		{
			if (dock->pos.x < dock->parent->children[0]->pos.x) return '1';
			if (dock->pos.x < dock->parent->children[1]->pos.x) return '1';
			return '0';
		}
		else
		{
			if (dock->pos.y < dock->parent->children[0]->pos.y) return '2';
			if (dock->pos.y < dock->parent->children[1]->pos.y) return '2';
			return '3';
		}
	}


	void tryDockToStoredLocation(Dock& dock)
	{
		if (dock.status == Status_Docked) return;
		if (dock.location[0] == 0) return;

		Dock* tmp = getRootDock();
		if (!tmp) return;

		Dock* prev = nullptr;
		char* c = dock.location + strlen(dock.location) - 1;
		while (c >= dock.location && tmp)
		{
			prev = tmp;
			tmp = *c == getLocationCode(tmp->children[0]) ? tmp->children[0] : tmp->children[1];
			if(tmp) --c;
		}
		if (tmp && tmp->children[0]) tmp = tmp->parent;
		doDock(dock, tmp ? tmp : prev, tmp && !tmp->children[0] ? Slot_Tab : getSlotFromLocationCode(*c));
	}


	void cleanDocks()
	{
		restart:
			for (int i = 0, c = m_docks.size(); i < c; ++i)
			{
				Dock& dock = *m_docks[i];
				if (dock.last_frame == 0 && dock.status != Status_Float && !dock.children[0])
				{
					fillLocation(*m_docks[i]);
					doUndock(*m_docks[i]);
					m_docks[i]->status = Status_Float;
					goto restart;
				}
			}
	}


	bool begin(const char* label, bool* opened, ImGuiWindowFlags extra_flags, const ImVec2& default_size)
	{
		IM_ASSERT(!m_is_begin_open);
		m_is_begin_open = true;
		Dock& dock = getDock(label, !opened || *opened, default_size);
		if (dock.last_frame != 0 && m_last_frame != ImGui::GetFrameCount())
		{
			cleanDocks();
		}
		dock.last_frame = ImGui::GetFrameCount();
		if (!dock.opened && (!opened || *opened)) tryDockToStoredLocation(dock);
		if (strcmp(dock.label, label) != 0)
		{
			MemFree(dock.label);
			dock.label = ImStrdup(label);
		}

		m_end_action = EndAction_None;

		if (dock.first && opened) *opened = dock.opened;
		dock.first = false;
		if (opened && !*opened)
		{
			if (dock.status != Status_Float)
			{
				fillLocation(dock);
				doUndock(dock);
				dock.status = Status_Float;
			}
			dock.opened = false;
			return false;
		}
		dock.opened = true;

		m_end_action = EndAction_Panel;
		beginPanel();

		m_current = &dock;
		if (dock.status == Status_Dragged) handleDrag(dock);

		bool is_float = dock.status == Status_Float;

		if (is_float) {
			SetNextWindowPos(dock.pos);
			SetNextWindowSize(dock.size, ImGuiCond_FirstUseEver);
			bool ret = Begin(label,
				opened,
				ImGuiWindowFlags_NoCollapse | extra_flags);
			m_end_action = EndAction_End;
			dock.pos = GetWindowPos();
			dock.size = GetWindowSize();

			ImGuiContext& g = *GImGui;

			if (g.ActiveId == GetCurrentWindow()->MoveId && g.IO.MouseDown[0])
			{
				m_drag_offset = GetMousePos() - dock.pos;
				doUndock(dock);
				dock.status = Status_Dragged;
			}
			return ret;
		}

		if (!dock.active && dock.status != Status_Dragged) return false;

		m_end_action = EndAction_EndChild;

		PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
		PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
		float tabbar_height = GetTextLineHeightWithSpacing();
		if (tabbar(dock.getFirstTab(), opened != nullptr))
		{
			fillLocation(dock);
			*opened = false;
		}
		ImVec2 pos = dock.pos;
		ImVec2 size = dock.size;
		pos.y += tabbar_height + GetStyle().WindowPadding.y;
		size.y -= tabbar_height + GetStyle().WindowPadding.y;

		SetCursorScreenPos(pos);
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
								 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
								 extra_flags;
		char tmp[256];
		strcpy(tmp, label);
		strcat(tmp, "_docked"); // to avoid https://github.com/ocornut/imgui/issues/713
		bool ret = BeginChild(tmp, size, true, flags);
		PopStyleColor();
		PopStyleColor();
		return ret;
	}

	void end() {
		if (m_end_action == EndAction_End) {
			End();
		} else if (m_end_action == EndAction_EndChild) {
			PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
			PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));
			EndChild();
			PopStyleColor();
			PopStyleColor();
		}
		m_current = nullptr;
		if (m_end_action > EndAction_None) endPanel();
		m_is_begin_open = false;
	}


	int getDockIndex(Dock* dock) {
		if (!dock) return -1;

		for (int i = 0; i < m_docks.size(); ++i) {
			if (dock == m_docks[i]) return i;
		}

		IM_ASSERT(false);
		return -1;
	}

	void save() {
		FILE *fp = fopen("imgui_dock.layout", "w");
		fprintf(fp, "docks %d\n\n", m_docks.size());
		for (int i = 0; i < m_docks.size(); ++i) {
			Dock& dock = *m_docks[i];

			fprintf(fp, "index    %d\n", i);
			fprintf(fp, "label    %s\n", dock.parent ? (dock.label[0] == '\0' ? "DOCK" : dock.label) : "ROOT");
			fprintf(fp, "x        %d\n", (int)dock.pos.x);
			fprintf(fp, "y        %d\n", (int)dock.pos.y);
			fprintf(fp, "size_x   %d\n", (int)dock.size.x);
			fprintf(fp, "size_y   %d\n", (int)dock.size.y);
			fprintf(fp, "status   %d\n", (int)dock.status);
			fprintf(fp, "active   %d\n", dock.active ? 1 : 0);
			fprintf(fp, "opened   %d\n", dock.opened ? 1 : 0);
			fillLocation(dock);
			fprintf(fp, "location %s\n", strlen(dock.location) ? dock.location : "-1");
			fprintf(fp, "child0   %d\n", getDockIndex(dock.children[0]));
			fprintf(fp, "child1   %d\n", getDockIndex(dock.children[1]));
			fprintf(fp, "prev_tab %d\n", getDockIndex(dock.prev_tab));
			fprintf(fp, "next_tab %d\n", getDockIndex(dock.next_tab));
			fprintf(fp, "parent   %d\n\n", getDockIndex(dock.parent));
		}
		fclose(fp);
	}

	Dock* getDockByIndex(int idx) { return idx < 0 ? nullptr : m_docks[idx]; }

	void load() {
		for (int i = 0; i < m_docks.size(); ++i) {
			m_docks[i]->~Dock();
			MemFree(m_docks[i]);
		}
		m_docks.clear();

		FILE *fp = fopen("imgui_dock.layout", "r");

		if (fp) {
			int ival;
			char str2[64];
			fscanf(fp, "docks %d", &ival);

			for (int i = 0; i < ival; i++) {
				Dock *new_dock = (Dock *) MemAlloc(sizeof(Dock));
				IM_PLACEMENT_NEW(new_dock) Dock();
				m_docks.push_back(new_dock);
			}

			for (int i = 0; i < ival; i++) {
				int id, id1, id2, id3, id4, id5;
				int st;
				int b1, b2;
				char lab[32];

				fscanf(fp, "%s %d", str2, &id);
				fscanf(fp, "%s %[^\n]s", str2, &lab[0]);
				fscanf(fp, "%s %f", str2, &m_docks[id]->pos.x);
				fscanf(fp, "%s %f", str2, &m_docks[id]->pos.y);
				fscanf(fp, "%s %f", str2, &m_docks[id]->size.x);
				fscanf(fp, "%s %f", str2, &m_docks[id]->size.y);
				fscanf(fp, "%s %d", str2, &st);
				fscanf(fp, "%s %d", str2, &b1);
				fscanf(fp, "%s %d", str2, &b2);
				fscanf(fp, "%s %s", str2, &m_docks[id]->location[0]);
				fscanf(fp, "%s %d", str2, &id1);
				fscanf(fp, "%s %d", str2, &id2);
				fscanf(fp, "%s %d", str2, &id3);
				fscanf(fp, "%s %d", str2, &id4);
				fscanf(fp, "%s %d", str2, &id5);

				m_docks[id]->label = strdup(lab);
				m_docks[id]->id = ImHash(m_docks[id]->label,0);

				m_docks[id]->children[0] = getDockByIndex(id1);
				m_docks[id]->children[1] = getDockByIndex(id2);
				m_docks[id]->prev_tab = getDockByIndex(id3);
				m_docks[id]->next_tab = getDockByIndex(id4);
				m_docks[id]->parent = getDockByIndex(id5);
				m_docks[id]->status = (Status_)st;
				m_docks[id]->active = b1;
				m_docks[id]->opened = b2;

				tryDockToStoredLocation(*m_docks[id]);
			}
			fclose(fp);
		}
	}
};

static DockContext g_dock;

void ShutdownDock() {
	for (int i = 0; i < g_dock.m_docks.size(); ++i) {
		g_dock.m_docks[i]->~Dock();
		MemFree(g_dock.m_docks[i]);
	}
	g_dock.m_docks.clear();
}

void RootDock(const ImVec2& pos, const ImVec2& size) {
	g_dock.rootDock(pos, size);
}

void SetDockActive() {
	g_dock.setDockActive();
}

bool BeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags, const ImVec2& default_size) {
	return g_dock.begin(label, opened, extra_flags, default_size);
}


void EndDock() {
	g_dock.end();
}

void SaveDock() {
	g_dock.save();
}

void LoadDock() {
	g_dock.load();
}

uptr<ImGuiFileDialog> ImGuiFileDialog::g_instance = uptr<ImGuiFileDialog>(new ImGuiFileDialog());

static bool stringComparator(VFSFileInfo a, VFSFileInfo b) {
	return !a.directory && !a.link;
}

void ImGuiFileDialog::ScanDir(String vPath) {
	m_FileList.clear();

	if (m_CurrentPath_Decomposition.empty()) {
		Vector<VFSFileInfo> finfos = VFS::get().listFiles(vPath);
		if (!finfos.empty()) {
			VFSFileInfo fi;
			fi.directory = true;
			fi.link = false;
			fi.ext = "";
			fi.fileName = "..";
			finfos.insert(finfos.begin(), fi);
		}
		m_FileList = finfos;
		SetCurrentDir(vPath);
	}
}

void ImGuiFileDialog::SetCurrentDir(String vPath) {
	if (VFS::get().exists(vPath)) {
		m_CurrentPath = vPath;
		m_CurrentPath_Decomposition = Util::split(m_CurrentPath, "/");
	}
}

void ImGuiFileDialog::ComposeNewPath(Vector<String>::iterator vIter) {
	m_CurrentPath = "";
	while (vIter != m_CurrentPath_Decomposition.begin()) {
		if (!m_CurrentPath.empty())
			m_CurrentPath = *vIter + "/" + m_CurrentPath;
		else
			m_CurrentPath = *vIter;
		vIter--;
	}
}

bool ImGuiFileDialog::FileDialog(const char* vName, const std::string& vFilters, std::string vPath, std::string vDefaultFileName) {
	bool res = false;

	IsOk = false;

	Vector<String> filters = Util::split(vFilters, ";");

	ImGui::SetNextWindowSize(ImVec2(512, 320));
	ImGui::Begin(vName, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	if (m_FileList.empty()) {
		if (vDefaultFileName.empty()) {
			m_SelectedFileName = vDefaultFileName;
		}

		ScanDir(vPath);
	}

	// show current path
	bool pathClick = false;
	ImGui::Text("Dir: ");
	ImGui::SameLine();

	for (auto itPathDecomp = m_CurrentPath_Decomposition.begin();
		 itPathDecomp != m_CurrentPath_Decomposition.end();
		 ++itPathDecomp)
	{
		if (itPathDecomp != m_CurrentPath_Decomposition.begin())
			ImGui::SameLine();

		String n = (*itPathDecomp);
		if (n.empty()) n = ".";

		if (ImGui::Button(n.c_str())) {
			ComposeNewPath(itPathDecomp);
			pathClick = true;
			break;
		}
	}

	ImVec2 size = ImGui::GetContentRegionAvail() - ImVec2(0.0f, 50.0f);

	ImGui::BeginChild("##FileDialog_FileList", size, true);

	for (std::vector<VFSFileInfo>::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it) {
		VFSFileInfo infos = *it;
		bool isFile = !infos.directory && !infos.link;

		String extNoDot = filters.empty() ? "" : filters.at(m_currentExtIndex);
		if (isFile && !extNoDot.empty() && infos.ext != extNoDot) {
			continue;
		}

		std::string str;
		if (infos.directory) str = "[D]";
		else if (infos.link) str = "[L]";
		else str = "[F]";

		ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.3f, 1.0f), str.c_str());

		ImGui::SameLine();

		bool sel = (infos.fileName == m_SelectedFileName);
		if (ImGui::Selectable(infos.fileName.c_str(), &sel)) {
			if (infos.directory) {
				if (infos.fileName == "..") {
					if (m_CurrentPath_Decomposition.size() > 1) {
						std::vector<std::string>::iterator itPathDecomp = m_CurrentPath_Decomposition.end() - 1;
						ComposeNewPath(itPathDecomp);
					}
				} else {
					m_CurrentPath += infos.fileName + "/";
				}
				pathClick = true;
			} else {
				m_SelectedFileName = infos.fileName;
			}
			break;
		}
	}

	// Directory change
	if (pathClick == true) {
		m_CurrentPath_Decomposition.clear();
		ScanDir(m_CurrentPath);
	}

	ImGui::EndChild();

	ImGui::Text("File Name: ");

	ImGui::SameLine();

	float width = ImGui::GetContentRegionAvailWidth();
	if (!vFilters.empty()) width -= 120.0f;

	ImGui::PushItemWidth(width);
	ImGui::InputText(
				"##FileName",
				(char*)m_SelectedFileName.c_str(),
				MAX_FILE_DIALOG_NAME_BUFFER,
				ImGuiInputTextFlags_ReadOnly
	);
	ImGui::PopItemWidth();

	if (!vFilters.empty()) {
		ImGui::SameLine();

		ImGui::PushItemWidth(100.0f);
		ImGui::Combo("##Filters", &m_currentExtIndex, filters);
		ImGui::PopItemWidth();
	}

	if (ImGui::Button("Ok")) {
		IsOk = true;
		res = true;
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel")) {
		IsOk = false;
		res = true;
	}

	ImGui::End();

	if (res == true) {
		m_FileList.clear();
	}

	return res;
}

String ImGuiFileDialog::GetFilepathName()
{
	String path = GetCurrentPath() + "/" + GetCurrentFileName();
	m_CurrentPath = "";
	m_SelectedFileName = "";
	m_FileList.clear();
	m_CurrentPath_Decomposition.clear();
	return path;
}

String ImGuiFileDialog::GetCurrentPath()
{
	return m_CurrentPath;
}

String ImGuiFileDialog::GetCurrentFileName()
{
	return m_SelectedFileName;
}

bool FileDialog(const char* vName, const String& filters, String vPath, String vDefaultFileName) {
	return ImGuiFileDialog::instance().FileDialog(
				vName,
				filters.c_str(),
				vPath,
				vDefaultFileName
	);
}

bool FileDialogOk() {
	return ImGuiFileDialog::instance().IsOk;
}

String GetFileDialogFileName() {
	return ImGuiFileDialog::instance().GetFilepathName();
}

static bool vector_getter(void* vec, int idx, const char** out_text) {
	auto& vector = *static_cast<Vector<String>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
}

bool Combo(const char* label, int* currIndex, Vector<String>& values) {
	if (values.empty()) { return false; }
	return ImGui::Combo(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

bool ListBox(const char* label, int* currIndex, Vector<String>& values) {
	if (values.empty()) { return false; }
	return ImGui::ListBox(label, currIndex, vector_getter, static_cast<void*>(&values), values.size());
}

} // namespace ImGui

namespace ImGuizmo {
   static const float ZPI = 3.14159265358979323846f;
   static const float RAD2DEG = (180.f / ZPI);
   static const float DEG2RAD = (ZPI / 180.f);
   static const float gGizmoSizeClipSpace = 0.1f;
   const float screenRotateSize = 0.06f;

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // utility and math

   void FPU_MatrixF_x_MatrixF(const float *a, const float *b, float *r)
   {
	  r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	  r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	  r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	  r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	  r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	  r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	  r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	  r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	  r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	  r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	  r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	  r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	  r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	  r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	  r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	  r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
   }

   //template <typename T> T LERP(T x, T y, float z) { return (x + (y - x)*z); }
   template <typename T> T Clamp(T x, T y, T z) { return ((x<y) ? y : ((x>z) ? z : x)); }
   template <typename T> T max(T x, T y) { return (x > y) ? x : y; }
   template <typename T> T min(T x, T y) { return (x < y) ? x : y; }
   template <typename T> bool IsWithin(T x, T y, T z) { return (x>=y) && (x<=z); }

   struct matrix_t;
   struct vec_t
   {
   public:
	  float x, y, z, w;

	  void Lerp(const vec_t& v, float t)
	  {
		 x += (v.x - x) * t;
		 y += (v.y - y) * t;
		 z += (v.z - z) * t;
		 w += (v.w - w) * t;
	  }

	  void Set(float v) { x = y = z = w = v; }
	  void Set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

	  vec_t& operator -= (const vec_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	  vec_t& operator += (const vec_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	  vec_t& operator *= (const vec_t& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
	  vec_t& operator *= (float v) { x *= v;    y *= v;    z *= v;    w *= v;    return *this; }

	  vec_t operator * (float f) const;
	  vec_t operator - () const;
	  vec_t operator - (const vec_t& v) const;
	  vec_t operator + (const vec_t& v) const;
	  vec_t operator * (const vec_t& v) const;

	  const vec_t& operator + () const { return (*this); }
	  float Length() const { return sqrtf(x*x + y*y + z*z); };
	  float LengthSq() const { return (x*x + y*y + z*z); };
	  vec_t Normalize() { (*this) *= (1.f / Length()); return (*this); }
	  vec_t Normalize(const vec_t& v) { this->Set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }
	  vec_t Abs() const;
	  void Cross(const vec_t& v)
	  {
		 vec_t res;
		 res.x = y * v.z - z * v.y;
		 res.y = z * v.x - x * v.z;
		 res.z = x * v.y - y * v.x;

		 x = res.x;
		 y = res.y;
		 z = res.z;
		 w = 0.f;
	  }
	  void Cross(const vec_t& v1, const vec_t& v2)
	  {
		 x = v1.y * v2.z - v1.z * v2.y;
		 y = v1.z * v2.x - v1.x * v2.z;
		 z = v1.x * v2.y - v1.y * v2.x;
		 w = 0.f;
	  }
	  float Dot(const vec_t &v) const
	  {
		 return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
	  }
	  float Dot3(const vec_t &v) const
	  {
		 return (x * v.x) + (y * v.y) + (z * v.z);
	  }

	  void Transform(const matrix_t& matrix);
	  void Transform(const vec_t & s, const matrix_t& matrix);

	  void TransformVector(const matrix_t& matrix);
	  void TransformPoint(const matrix_t& matrix);
	  void TransformVector(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformVector(matrix); }
	  void TransformPoint(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformPoint(matrix); }

	  float& operator [] (size_t index) { return ((float*)&x)[index]; }
	  const float& operator [] (size_t index) const { return ((float*)&x)[index]; }
   };

   vec_t makeVect(float _x, float _y, float _z = 0.f, float _w = 0.f) { vec_t res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }
   vec_t makeVect(ImVec2 v) { vec_t res; res.x = v.x; res.y = v.y; res.z = 0.f; res.w = 0.f; return res; }
   vec_t vec_t::operator * (float f) const { return makeVect(x * f, y * f, z * f, w *f); }
   vec_t vec_t::operator - () const { return makeVect(-x, -y, -z, -w); }
   vec_t vec_t::operator - (const vec_t& v) const { return makeVect(x - v.x, y - v.y, z - v.z, w - v.w); }
   vec_t vec_t::operator + (const vec_t& v) const { return makeVect(x + v.x, y + v.y, z + v.z, w + v.w); }
   vec_t vec_t::operator * (const vec_t& v) const { return makeVect(x * v.x, y * v.y, z * v.z, w * v.w); }
   vec_t vec_t::Abs() const { return makeVect(fabsf(x), fabsf(y), fabsf(z)); }

   vec_t Normalized(const vec_t& v) { vec_t res; res = v; res.Normalize(); return res; }
   vec_t Cross(const vec_t& v1, const vec_t& v2)
   {
	  vec_t res;
	  res.x = v1.y * v2.z - v1.z * v2.y;
	  res.y = v1.z * v2.x - v1.x * v2.z;
	  res.z = v1.x * v2.y - v1.y * v2.x;
	  res.w = 0.f;
	  return res;
   }

   float Dot(const vec_t &v1, const vec_t &v2)
   {
	  return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
   }

   vec_t BuildPlan(const vec_t & p_point1, const vec_t & p_normal)
   {
	  vec_t normal, res;
	  normal.Normalize(p_normal);
	  res.w = normal.Dot(p_point1);
	  res.x = normal.x;
	  res.y = normal.y;
	  res.z = normal.z;
	  return res;
   }

   struct matrix_t
   {
   public:

	  union
	  {
		 float m[4][4];
		 float m16[16];
		 struct
		 {
			vec_t right, up, dir, position;
		 } v;
		 vec_t component[4];
	  };

	  matrix_t(const matrix_t& other) { memcpy(&m16[0], &other.m16[0], sizeof(float) * 16); }
	  matrix_t() {}

	  operator float * () { return m16; }
	  operator const float* () const { return m16; }
	  void Translation(float _x, float _y, float _z) { this->Translation(makeVect(_x, _y, _z)); }

	  void Translation(const vec_t& vt)
	  {
		 v.right.Set(1.f, 0.f, 0.f, 0.f);
		 v.up.Set(0.f, 1.f, 0.f, 0.f);
		 v.dir.Set(0.f, 0.f, 1.f, 0.f);
		 v.position.Set(vt.x, vt.y, vt.z, 1.f);
	  }

	  void Scale(float _x, float _y, float _z)
	  {
		 v.right.Set(_x, 0.f, 0.f, 0.f);
		 v.up.Set(0.f, _y, 0.f, 0.f);
		 v.dir.Set(0.f, 0.f, _z, 0.f);
		 v.position.Set(0.f, 0.f, 0.f, 1.f);
	  }
	  void Scale(const vec_t& s) { Scale(s.x, s.y, s.z); }

	  matrix_t& operator *= (const matrix_t& mat)
	  {
		 matrix_t tmpMat;
		 tmpMat = *this;
		 tmpMat.Multiply(mat);
		 *this = tmpMat;
		 return *this;
	  }
	  matrix_t operator * (const matrix_t& mat) const
	  {
		 matrix_t matT;
		 matT.Multiply(*this, mat);
		 return matT;
	  }

	  void Multiply(const matrix_t &matrix)
	  {
		 matrix_t tmp;
		 tmp = *this;

		 FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
	  }

	  void Multiply(const matrix_t &m1, const matrix_t &m2)
	  {
		 FPU_MatrixF_x_MatrixF((float*)&m1, (float*)&m2, (float*)this);
	  }

	  float GetDeterminant() const
	  {
		 return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
			m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
	  }

	  float Inverse(const matrix_t &srcMatrix, bool affine = false);
	  void SetToIdentity()
	  {
		 v.right.Set(1.f, 0.f, 0.f, 0.f);
		 v.up.Set(0.f, 1.f, 0.f, 0.f);
		 v.dir.Set(0.f, 0.f, 1.f, 0.f);
		 v.position.Set(0.f, 0.f, 0.f, 1.f);
	  }
	  void Transpose()
	  {
		 matrix_t tmpm;
		 for (int l = 0; l < 4; l++)
		 {
			for (int c = 0; c < 4; c++)
			{
			   tmpm.m[l][c] = m[c][l];
			}
		 }
		 (*this) = tmpm;
	  }

	  void RotationAxis(const vec_t & axis, float angle);

	  void OrthoNormalize()
	  {
		 v.right.Normalize();
		 v.up.Normalize();
		 v.dir.Normalize();
	  }
   };

   void vec_t::Transform(const matrix_t& matrix)
   {
	  vec_t out;

	  out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
	  out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
	  out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
	  out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];

	  x = out.x;
	  y = out.y;
	  z = out.z;
	  w = out.w;
   }

   void vec_t::Transform(const vec_t & s, const matrix_t& matrix)
   {
	  *this = s;
	  Transform(matrix);
   }

   void vec_t::TransformPoint(const matrix_t& matrix)
   {
	  vec_t out;

	  out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
	  out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
	  out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
	  out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

	  x = out.x;
	  y = out.y;
	  z = out.z;
	  w = out.w;
   }


   void vec_t::TransformVector(const matrix_t& matrix)
   {
	  vec_t out;

	  out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
	  out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
	  out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
	  out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];

	  x = out.x;
	  y = out.y;
	  z = out.z;
	  w = out.w;
   }

   float matrix_t::Inverse(const matrix_t &srcMatrix, bool affine)
   {
	  float det = 0;

	  if (affine)
	  {
		 det = GetDeterminant();
		 float s = 1 / det;
		 m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
		 m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
		 m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
		 m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
		 m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
		 m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
		 m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
		 m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
		 m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
		 m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
		 m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
		 m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
	  }
	  else
	  {
		 // transpose matrix
		 float src[16];
		 for (int i = 0; i < 4; ++i)
		 {
			src[i] = srcMatrix.m16[i * 4];
			src[i + 4] = srcMatrix.m16[i * 4 + 1];
			src[i + 8] = srcMatrix.m16[i * 4 + 2];
			src[i + 12] = srcMatrix.m16[i * 4 + 3];
		 }

		 // calculate pairs for first 8 elements (cofactors)
		 float tmp[12]; // temp array for pairs
		 tmp[0] = src[10] * src[15];
		 tmp[1] = src[11] * src[14];
		 tmp[2] = src[9] * src[15];
		 tmp[3] = src[11] * src[13];
		 tmp[4] = src[9] * src[14];
		 tmp[5] = src[10] * src[13];
		 tmp[6] = src[8] * src[15];
		 tmp[7] = src[11] * src[12];
		 tmp[8] = src[8] * src[14];
		 tmp[9] = src[10] * src[12];
		 tmp[10] = src[8] * src[13];
		 tmp[11] = src[9] * src[12];

		 // calculate first 8 elements (cofactors)
		 m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
		 m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
		 m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
		 m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
		 m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
		 m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
		 m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
		 m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

		 // calculate pairs for second 8 elements (cofactors)
		 tmp[0] = src[2] * src[7];
		 tmp[1] = src[3] * src[6];
		 tmp[2] = src[1] * src[7];
		 tmp[3] = src[3] * src[5];
		 tmp[4] = src[1] * src[6];
		 tmp[5] = src[2] * src[5];
		 tmp[6] = src[0] * src[7];
		 tmp[7] = src[3] * src[4];
		 tmp[8] = src[0] * src[6];
		 tmp[9] = src[2] * src[4];
		 tmp[10] = src[0] * src[5];
		 tmp[11] = src[1] * src[4];

		 // calculate second 8 elements (cofactors)
		 m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
		 m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
		 m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
		 m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
		 m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
		 m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
		 m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
		 m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

		 // calculate determinant
		 det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

		 // calculate matrix inverse
		 float invdet = 1 / det;
		 for (int j = 0; j < 16; ++j)
		 {
			m16[j] *= invdet;
		 }
	  }

	  return det;
   }

   void matrix_t::RotationAxis(const vec_t & axis, float angle)
   {
	  float length2 = axis.LengthSq();
	  if (length2 < FLT_EPSILON)
	  {
		 SetToIdentity();
		 return;
	  }

	  vec_t n = axis * (1.f / sqrtf(length2));
	  float s = sinf(angle);
	  float c = cosf(angle);
	  float k = 1.f - c;

	  float xx = n.x * n.x * k + c;
	  float yy = n.y * n.y * k + c;
	  float zz = n.z * n.z * k + c;
	  float xy = n.x * n.y * k;
	  float yz = n.y * n.z * k;
	  float zx = n.z * n.x * k;
	  float xs = n.x * s;
	  float ys = n.y * s;
	  float zs = n.z * s;

	  m[0][0] = xx;
	  m[0][1] = xy + zs;
	  m[0][2] = zx - ys;
	  m[0][3] = 0.f;
	  m[1][0] = xy - zs;
	  m[1][1] = yy;
	  m[1][2] = yz + xs;
	  m[1][3] = 0.f;
	  m[2][0] = zx + ys;
	  m[2][1] = yz - xs;
	  m[2][2] = zz;
	  m[2][3] = 0.f;
	  m[3][0] = 0.f;
	  m[3][1] = 0.f;
	  m[3][2] = 0.f;
	  m[3][3] = 1.f;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   enum MOVETYPE
   {
	  NONE,
	  MOVE_X,
	  MOVE_Y,
	  MOVE_Z,
	  MOVE_YZ,
	  MOVE_ZX,
	  MOVE_XY,
	  MOVE_SCREEN,
	  ROTATE_X,
	  ROTATE_Y,
	  ROTATE_Z,
	  ROTATE_SCREEN,
	  SCALE_X,
	  SCALE_Y,
	  SCALE_Z,
	  SCALE_XYZ
   };

   struct Context
   {
	  Context() : mbUsing(false), mbEnable(true), mbUsingBounds(false)
	  {
	  }

	  ImDrawList* mDrawList;

	  MODE mMode;
	  matrix_t mViewMat;
	  matrix_t mProjectionMat;
	  matrix_t mModel;
	  matrix_t mModelInverse;
	  matrix_t mModelSource;
	  matrix_t mModelSourceInverse;
	  matrix_t mMVP;
	  matrix_t mViewProjection;

	  vec_t mModelScaleOrigin;
	  vec_t mCameraEye;
	  vec_t mCameraRight;
	  vec_t mCameraDir;
	  vec_t mCameraUp;
	  vec_t mRayOrigin;
	  vec_t mRayVector;

	  float  mRadiusSquareCenter;
	  ImVec2 mScreenSquareCenter;
	  ImVec2 mScreenSquareMin;
	  ImVec2 mScreenSquareMax;

	  float mScreenFactor;
	  vec_t mRelativeOrigin;

	  bool mbUsing;
	  bool mbEnable;

	  // translation
	  vec_t mTranslationPlan;
	  vec_t mTranslationPlanOrigin;
	  vec_t mMatrixOrigin;

	  // rotation
	  vec_t mRotationVectorSource;
	  float mRotationAngle;
	  float mRotationAngleOrigin;
	  //vec_t mWorldToLocalAxis;

	  // scale
	  vec_t mScale;
	  vec_t mScaleValueOrigin;
	  float mSaveMousePosx;

	  // save axis factor when using gizmo
	  bool mBelowAxisLimit[3];
	  bool mBelowPlaneLimit[3];
	  float mAxisFactor[3];

	  // bounds stretching
	  vec_t mBoundsPivot;
	  vec_t mBoundsAnchor;
	  vec_t mBoundsPlan;
	  vec_t mBoundsLocalPivot;
	  int mBoundsBestAxis;
	  int mBoundsAxis[2];
	  bool mbUsingBounds;
	  matrix_t mBoundsMatrix;

	  //
	  int mCurrentOperation;

	  float mX = 0.f;
	  float mY = 0.f;
	  float mWidth = 0.f;
	  float mHeight = 0.f;
	  float mXMax = 0.f;
	  float mYMax = 0.f;
	 float mDisplayRatio = 1.f;

	 bool mIsOrthographic = false;
   };

   static Context gContext;

   static const float angleLimit = 0.96f;
   static const float planeLimit = 0.2f;

   static const vec_t directionUnary[3] = { makeVect(1.f, 0.f, 0.f), makeVect(0.f, 1.f, 0.f), makeVect(0.f, 0.f, 1.f) };
   static const ImU32 directionColor[3] = { 0xFF0000AA, 0xFF00AA00, 0xFFAA0000 };

   // Alpha: 100%: FF, 87%: DE, 70%: B3, 54%: 8A, 50%: 80, 38%: 61, 12%: 1F
   static const ImU32 planeColor[3] = { 0x610000AA, 0x6100AA00, 0x61AA0000 };
   static const ImU32 selectionColor = 0x8A1080FF;
   static const ImU32 inactiveColor = 0x99999999;
   static const ImU32 translationLineColor = 0xAAAAAAAA;
   static const char *translationInfoMask[] = { "X : %5.3f", "Y : %5.3f", "Z : %5.3f",
	  "Y : %5.3f Z : %5.3f", "X : %5.3f Z : %5.3f", "X : %5.3f Y : %5.3f",
	  "X : %5.3f Y : %5.3f Z : %5.3f" };
   static const char *scaleInfoMask[] = { "X : %5.2f", "Y : %5.2f", "Z : %5.2f", "XYZ : %5.2f" };
   static const char *rotationInfoMask[] = { "X : %5.2f deg %5.2f rad", "Y : %5.2f deg %5.2f rad", "Z : %5.2f deg %5.2f rad", "Screen : %5.2f deg %5.2f rad" };
   static const int translationInfoIndex[] = { 0,0,0, 1,0,0, 2,0,0, 1,2,0, 0,2,0, 0,1,0, 0,1,2 };
   static const float quadMin = 0.5f;
   static const float quadMax = 0.8f;
   static const float quadUV[8] = { quadMin, quadMin, quadMin, quadMax, quadMax, quadMax, quadMax, quadMin };
   static const int halfCircleSegmentCount = 64;
   static const float snapTension = 0.5f;

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //
   static int GetMoveType(vec_t *gizmoHitProportion);
   static int GetRotateType();
   static int GetScaleType();

   static ImVec2 worldToPos(const vec_t& worldPos, const matrix_t& mat)
   {
	  vec_t trans;
	  trans.TransformPoint(worldPos, mat);
	  trans *= 0.5f / trans.w;
	  trans += makeVect(0.5f, 0.5f);
	  trans.y = 1.f - trans.y;
	  trans.x *= gContext.mWidth;
	  trans.y *= gContext.mHeight;
	  trans.x += gContext.mX;
	  trans.y += gContext.mY;
	  return ImVec2(trans.x, trans.y);
   }

   static void ComputeCameraRay(vec_t &rayOrigin, vec_t &rayDir)
   {
	  ImGuiIO& io = ImGui::GetIO();

	  matrix_t mViewProjInverse;
	  mViewProjInverse.Inverse(gContext.mViewMat * gContext.mProjectionMat);

	  float mox = ((io.MousePos.x - gContext.mX) / gContext.mWidth) * 2.f - 1.f;
	  float moy = (1.f - ((io.MousePos.y - gContext.mY) / gContext.mHeight)) * 2.f - 1.f;

	  rayOrigin.Transform(makeVect(mox, moy, 0.f, 1.f), mViewProjInverse);
	  rayOrigin *= 1.f / rayOrigin.w;
	  vec_t rayEnd;
	  rayEnd.Transform(makeVect(mox, moy, 1.f, 1.f), mViewProjInverse);
	  rayEnd *= 1.f / rayEnd.w;
	  rayDir = Normalized(rayEnd - rayOrigin);
   }

   static float GetSegmentLengthClipSpace(const vec_t& start, const vec_t& end)
   {
	  vec_t startOfSegment = start;
	  startOfSegment.TransformPoint(gContext.mMVP);
	  if (fabsf(startOfSegment.w)> FLT_EPSILON) // check for axis aligned with camera direction
		 startOfSegment *= 1.f / startOfSegment.w;

	  vec_t endOfSegment = end;
	  endOfSegment.TransformPoint(gContext.mMVP);
	  if (fabsf(endOfSegment.w)> FLT_EPSILON) // check for axis aligned with camera direction
		 endOfSegment *= 1.f / endOfSegment.w;

	  vec_t clipSpaceAxis = endOfSegment - startOfSegment;
	  clipSpaceAxis.y /= gContext.mDisplayRatio;
	  float segmentLengthInClipSpace = sqrtf(clipSpaceAxis.x*clipSpaceAxis.x + clipSpaceAxis.y*clipSpaceAxis.y);
	  return segmentLengthInClipSpace;
   }

   static float GetParallelogram(const vec_t& ptO, const vec_t& ptA, const vec_t& ptB)
   {
	  vec_t pts[] = { ptO, ptA, ptB };
	  for (unsigned int i = 0; i < 3; i++)
	  {
		 pts[i].TransformPoint(gContext.mMVP);
		 if (fabsf(pts[i].w)> FLT_EPSILON) // check for axis aligned with camera direction
			pts[i] *= 1.f / pts[i].w;
	  }
	  vec_t segA = pts[1] - pts[0];
	  vec_t segB = pts[2] - pts[0];
	  segA.y /= gContext.mDisplayRatio;
	  segB.y /= gContext.mDisplayRatio;
	  vec_t segAOrtho = makeVect(-segA.y, segA.x);
	  segAOrtho.Normalize();
	  float dt = segAOrtho.Dot3(segB);
	  float surface = sqrtf(segA.x*segA.x + segA.y*segA.y) * fabsf(dt);
	  return surface;
   }

   inline vec_t PointOnSegment(const vec_t & point, const vec_t & vertPos1, const vec_t & vertPos2)
   {
	  vec_t c = point - vertPos1;
	  vec_t V;

	  V.Normalize(vertPos2 - vertPos1);
	  float d = (vertPos2 - vertPos1).Length();
	  float t = V.Dot3(c);

	  if (t < 0.f)
		 return vertPos1;

	  if (t > d)
		 return vertPos2;

	  return vertPos1 + V * t;
   }

   static float IntersectRayPlane(const vec_t & rOrigin, const vec_t& rVector, const vec_t& plan)
   {
	  float numer = plan.Dot3(rOrigin) - plan.w;
	  float denom = plan.Dot3(rVector);

	  if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
		 return -1.0f;

	  return -(numer / denom);
   }

   static bool IsInContextRect( ImVec2 p )
   {
	   return IsWithin( p.x, gContext.mX, gContext.mXMax ) && IsWithin(p.y, gContext.mY, gContext.mYMax );
   }

   void SetRect(float x, float y, float width, float height)
   {
	   gContext.mX = x;
	   gContext.mY = y;
	   gContext.mWidth = width;
	   gContext.mHeight = height;
	   gContext.mXMax = gContext.mX + gContext.mWidth;
	   gContext.mYMax = gContext.mY + gContext.mXMax;
	   gContext.mDisplayRatio = width / height;
   }

   IMGUI_API void SetOrthographic(bool isOrthographic)
   {
	  gContext.mIsOrthographic = isOrthographic;
   }

   void SetDrawlist()
   {
	  gContext.mDrawList = ImGui::GetWindowDrawList();
   }

   void BeginFrame()
   {
	  ImGuiIO& io = ImGui::GetIO();

	  const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
	  ImGui::SetNextWindowSize(io.DisplaySize);
	  ImGui::SetNextWindowPos(ImVec2(0, 0));

	  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
	  ImGui::PushStyleColor(ImGuiCol_Border, 0);
	  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	  ImGui::Begin("gizmo", NULL, flags);
	  gContext.mDrawList = ImGui::GetWindowDrawList();
	  ImGui::End();
	  ImGui::PopStyleVar();
	  ImGui::PopStyleColor(2);
   }

   bool IsUsing()
   {
	  return gContext.mbUsing||gContext.mbUsingBounds;
   }

   bool IsOver()
   {
	  return (GetMoveType(NULL) != NONE) || GetRotateType() != NONE || GetScaleType() != NONE || IsUsing();
   }

   void Enable(bool enable)
   {
	  gContext.mbEnable = enable;
	  if (!enable)
	  {
		  gContext.mbUsing = false;
		  gContext.mbUsingBounds = false;
	  }
   }

   static float GetUniform(const vec_t& position, const matrix_t& mat)
   {
	  vec_t trf = makeVect(position.x, position.y, position.z, 1.f);
	  trf.Transform(mat);
	  return trf.w;
   }

   static void ComputeContext(const float *view, const float *projection, float *matrix, MODE mode)
   {
	  gContext.mMode = mode;
	  gContext.mViewMat = *(matrix_t*)view;
	  gContext.mProjectionMat = *(matrix_t*)projection;

	  if (mode == LOCAL)
	  {
		 gContext.mModel = *(matrix_t*)matrix;
		 gContext.mModel.OrthoNormalize();
	  }
	  else
	  {
		 gContext.mModel.Translation(((matrix_t*)matrix)->v.position);
	  }
	  gContext.mModelSource = *(matrix_t*)matrix;
	  gContext.mModelScaleOrigin.Set(gContext.mModelSource.v.right.Length(), gContext.mModelSource.v.up.Length(), gContext.mModelSource.v.dir.Length());

	  gContext.mModelInverse.Inverse(gContext.mModel);
	  gContext.mModelSourceInverse.Inverse(gContext.mModelSource);
	  gContext.mViewProjection = gContext.mViewMat * gContext.mProjectionMat;
	  gContext.mMVP = gContext.mModel * gContext.mViewProjection;

	  matrix_t viewInverse;
	  viewInverse.Inverse(gContext.mViewMat);
	  gContext.mCameraDir = viewInverse.v.dir;
	  gContext.mCameraEye = viewInverse.v.position;
	  gContext.mCameraRight = viewInverse.v.right;
	  gContext.mCameraUp = viewInverse.v.up;

	 // compute scale from the size of camera right vector projected on screen at the matrix position
	 vec_t pointRight = viewInverse.v.right;
	 pointRight.TransformPoint(gContext.mViewProjection);
	 gContext.mScreenFactor = gGizmoSizeClipSpace / (pointRight.x / pointRight.w - gContext.mMVP.v.position.x / gContext.mMVP.v.position.w);

	 vec_t rightViewInverse = viewInverse.v.right;
	 rightViewInverse.TransformVector(gContext.mModelInverse);
	 float rightLength = GetSegmentLengthClipSpace(makeVect(0.f, 0.f), rightViewInverse);
	 gContext.mScreenFactor = gGizmoSizeClipSpace / rightLength;

	  ImVec2 centerSSpace = worldToPos(makeVect(0.f, 0.f), gContext.mMVP);
	  gContext.mScreenSquareCenter = centerSSpace;
	  gContext.mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
	  gContext.mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

	  ComputeCameraRay(gContext.mRayOrigin, gContext.mRayVector);
   }

   static void ComputeColors(ImU32 *colors, int type, OPERATION operation)
   {
	  if (gContext.mbEnable)
	  {
		 switch (operation)
		 {
		 case TRANSLATE:
			colors[0] = (type == MOVE_SCREEN) ? selectionColor : 0xFFFFFFFF;
			for (int i = 0; i < 3; i++)
			{
			   colors[i + 1] = (type == (int)(MOVE_X + i)) ? selectionColor : directionColor[i];
			   colors[i + 4] = (type == (int)(MOVE_YZ + i)) ? selectionColor : planeColor[i];
			   colors[i + 4] = (type == MOVE_SCREEN) ? selectionColor : colors[i + 4];
			}
			break;
		 case ROTATE:
			colors[0] = (type == ROTATE_SCREEN) ? selectionColor : 0xFFFFFFFF;
			for (int i = 0; i < 3; i++)
			   colors[i + 1] = (type == (int)(ROTATE_X + i)) ? selectionColor : directionColor[i];
			break;
		 case SCALE:
			colors[0] = (type == SCALE_XYZ) ? selectionColor : 0xFFFFFFFF;
			for (int i = 0; i < 3; i++)
			   colors[i + 1] = (type == (int)(SCALE_X + i)) ? selectionColor : directionColor[i];
			break;
		 case BOUNDS:
			break;
		 }
	  }
	  else
	  {
		 for (int i = 0; i < 7; i++)
			colors[i] = inactiveColor;
	  }
   }

   static void ComputeTripodAxisAndVisibility(int axisIndex, vec_t& dirAxis, vec_t& dirPlaneX, vec_t& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
   {
	  dirAxis = directionUnary[axisIndex];
	  dirPlaneX = directionUnary[(axisIndex + 1) % 3];
	  dirPlaneY = directionUnary[(axisIndex + 2) % 3];

	  if (gContext.mbUsing)
	  {
		 // when using, use stored factors so the gizmo doesn't flip when we translate
		 belowAxisLimit = gContext.mBelowAxisLimit[axisIndex];
		 belowPlaneLimit = gContext.mBelowPlaneLimit[axisIndex];

		 dirPlaneX *= gContext.mAxisFactor[axisIndex];
		 dirPlaneY *= gContext.mAxisFactor[(axisIndex + 1) % 3];
	  }
	  else
	  {
		 // new method
		 float lenDir = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis);
		 float lenDirMinus = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirAxis);

		 float lenDirPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneX);
		 float lenDirMinusPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneX);

		 float lenDirPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneY);
		 float lenDirMinusPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneY);

		 float mulAxis = (lenDir < lenDirMinus && fabsf(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.f : 1.f;
		 float mulAxisX = (lenDirPlaneX < lenDirMinusPlaneX && fabsf(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
		 float mulAxisY = (lenDirPlaneY < lenDirMinusPlaneY && fabsf(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
		 dirAxis *= mulAxis;
		 dirPlaneX *= mulAxisX;
		 dirPlaneY *= mulAxisY;

		 // for axis
		 float axisLengthInClipSpace = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis * gContext.mScreenFactor);

		 float paraSurf = GetParallelogram(makeVect(0.f, 0.f, 0.f), dirPlaneX * gContext.mScreenFactor, dirPlaneY * gContext.mScreenFactor);
		 belowPlaneLimit = (paraSurf > 0.0025f);
		 belowAxisLimit = (axisLengthInClipSpace > 0.02f);

		 // and store values
		 gContext.mAxisFactor[axisIndex] = mulAxis;
		 gContext.mAxisFactor[(axisIndex + 1) % 3] = mulAxisY;
		 gContext.mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
		 gContext.mBelowAxisLimit[axisIndex] = belowAxisLimit;
		 gContext.mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
	  }
   }

   static void ComputeSnap(float*value, float snap)
   {
	  if (snap <= FLT_EPSILON)
		 return;
	  float modulo = fmodf(*value, snap);
	  float moduloRatio = fabsf(modulo) / snap;
	  if (moduloRatio < snapTension)
		 *value -= modulo;
	  else if (moduloRatio >(1.f - snapTension))
		 *value = *value - modulo + snap * ((*value<0.f) ? -1.f : 1.f);
   }
   static void ComputeSnap(vec_t& value, float *snap)
   {
	  for (int i = 0; i < 3; i++)
	  {
		 ComputeSnap(&value[i], snap[i]);
	  }
   }

   static float ComputeAngleOnPlan()
   {
	  const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
	  vec_t localPos = Normalized(gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.v.position);

	  vec_t perpendicularVector;
	  perpendicularVector.Cross(gContext.mRotationVectorSource, gContext.mTranslationPlan);
	  perpendicularVector.Normalize();
	  float acosAngle = Clamp(Dot(localPos, gContext.mRotationVectorSource), -0.9999f, 0.9999f);
	  float angle = acosf(acosAngle);
	  angle *= (Dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
	  return angle;
   }

   static void DrawRotationGizmo(int type)
   {
	  ImDrawList* drawList = gContext.mDrawList;

	  // colors
	  ImU32 colors[7];
	  ComputeColors(colors, type, ROTATE);

	 vec_t cameraToModelNormalized;
	 if (gContext.mIsOrthographic)
	 {
		matrix_t viewInverse;
		viewInverse.Inverse(*(matrix_t*)&gContext.mViewMat);
		cameraToModelNormalized = viewInverse.v.dir;
	 }
	 else
	 {
		cameraToModelNormalized = Normalized(gContext.mModel.v.position - gContext.mCameraEye);
	 }

	  cameraToModelNormalized.TransformVector(gContext.mModelInverse);

	  gContext.mRadiusSquareCenter = screenRotateSize * gContext.mHeight;

	 for (int axis = 0; axis < 3; axis++)
	  {
		 ImVec2 circlePos[halfCircleSegmentCount];

		 float angleStart = atan2f(cameraToModelNormalized[(4-axis)%3], cameraToModelNormalized[(3 - axis) % 3]) + ZPI * 0.5f;

		 for (unsigned int i = 0; i < halfCircleSegmentCount; i++)
		 {
			float ng = angleStart + ZPI * ((float)i / (float)halfCircleSegmentCount);
			vec_t axisPos = makeVect(cosf(ng), sinf(ng), 0.f);
			vec_t pos = makeVect(axisPos[axis], axisPos[(axis+1)%3], axisPos[(axis+2)%3]) * gContext.mScreenFactor;
			circlePos[i] = worldToPos(pos, gContext.mMVP);
		 }

		 float radiusAxis = sqrtf( (ImLengthSqr(worldToPos(gContext.mModel.v.position, gContext.mViewProjection) - circlePos[0]) ));
		 if(radiusAxis > gContext.mRadiusSquareCenter)
		   gContext.mRadiusSquareCenter = radiusAxis;

		 drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 2);
	  }
	  drawList->AddCircle(worldToPos(gContext.mModel.v.position, gContext.mViewProjection), gContext.mRadiusSquareCenter, colors[0], 64, 3.f);

	  if (gContext.mbUsing)
	  {
		 ImVec2 circlePos[halfCircleSegmentCount +1];

		 circlePos[0] = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
		 for (unsigned int i = 1; i < halfCircleSegmentCount; i++)
		 {
			float ng = gContext.mRotationAngle * ((float)(i-1) / (float)(halfCircleSegmentCount -1));
			matrix_t rotateVectorMatrix;
			rotateVectorMatrix.RotationAxis(gContext.mTranslationPlan, ng);
			vec_t pos;
			pos.TransformPoint(gContext.mRotationVectorSource, rotateVectorMatrix);
			pos *= gContext.mScreenFactor;
			circlePos[i] = worldToPos(pos + gContext.mModel.v.position, gContext.mViewProjection);
		 }
		 drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount, 0x801080FF);
		 drawList->AddPolyline(circlePos, halfCircleSegmentCount, 0xFF1080FF, true, 2);

		 ImVec2 destinationPosOnScreen = circlePos[1];
		 char tmps[512];
		 ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - ROTATE_X], (gContext.mRotationAngle/ZPI)*180.f, gContext.mRotationAngle);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
	  }
   }

   static void DrawHatchedAxis(const vec_t& axis)
   {
	  for (int j = 1; j < 10; j++)
	  {
		 ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * gContext.mScreenFactor, gContext.mMVP);
		 ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * gContext.mScreenFactor, gContext.mMVP);
		 gContext.mDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.f);
	  }
   }

   static void DrawScaleGizmo(int type)
   {
	  ImDrawList* drawList = gContext.mDrawList;

	  // colors
	  ImU32 colors[7];
	  ComputeColors(colors, type, SCALE);

	  // draw
	  vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

	  if (gContext.mbUsing)
		 scaleDisplay = gContext.mScale;

	  for (unsigned int i = 0; i < 3; i++)
	  {
		vec_t dirPlaneX, dirPlaneY, dirAxis;
		 bool belowAxisLimit, belowPlaneLimit;
		 ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

		 // draw axis
		 if (belowAxisLimit)
		 {
			ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
			ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * gContext.mScreenFactor, gContext.mMVP);
			ImVec2 worldDirSSpace = worldToPos((dirAxis * scaleDisplay[i]) * gContext.mScreenFactor, gContext.mMVP);

			if (gContext.mbUsing)
			{
			   drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 3.f);
			   drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, 0xFF404040);
			}

			drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
			drawList->AddCircleFilled(worldDirSSpace, 6.f, colors[i + 1]);

			if (gContext.mAxisFactor[i] < 0.f)
			   DrawHatchedAxis(dirAxis * scaleDisplay[i]);
		 }
	  }

	  // draw screen cirle
	  drawList->AddCircleFilled(gContext.mScreenSquareCenter, 6.f, colors[0], 32);

	  if (gContext.mbUsing)
	  {
		 //ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
		 ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
		 /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
		 dif.Normalize();
		 dif *= 5.f;
		 drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
		 drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
		 drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
		 */
		 char tmps[512];
		 //vec_t deltaInfo = gContext.mModel.v.position - gContext.mMatrixOrigin;
		 int componentInfoIndex = (type - SCALE_X) * 3;
		 ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
	  }
   }


   static void DrawTranslationGizmo(int type)
   {
	  ImDrawList* drawList = gContext.mDrawList;
	  if (!drawList)
		  return;

	  // colors
	  ImU32 colors[7];
	  ComputeColors(colors, type, TRANSLATE);

	  const ImVec2 origin = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);

	  // draw
	  bool belowAxisLimit = false;
	  bool belowPlaneLimit = false;
	  for (unsigned int i = 0; i < 3; ++i)
	  {
		 vec_t dirPlaneX, dirPlaneY, dirAxis;
		 ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

		 // draw axis
		 if (belowAxisLimit)
		 {
			ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gContext.mScreenFactor, gContext.mMVP);
			ImVec2 worldDirSSpace = worldToPos(dirAxis * gContext.mScreenFactor, gContext.mMVP);

			drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);

			// Arrow head begin
			ImVec2 dir(origin - worldDirSSpace);

			float d = sqrtf(ImLengthSqr(dir));
			dir /= d; // Normalize
			dir *= 6.0f;

			ImVec2 ortogonalDir(dir.y, -dir.x); // Perpendicular vector
			ImVec2 a(worldDirSSpace + dir);
			drawList->AddTriangleFilled(worldDirSSpace - dir, a + ortogonalDir, a - ortogonalDir, colors[i + 1]);
			// Arrow head end

			if (gContext.mAxisFactor[i] < 0.f)
			   DrawHatchedAxis(dirAxis);
		 }

		 // draw plane
		 if (belowPlaneLimit)
		 {
			ImVec2 screenQuadPts[4];
			for (int j = 0; j < 4; ++j)
			{
			   vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * gContext.mScreenFactor;
			   screenQuadPts[j] = worldToPos(cornerWorldPos, gContext.mMVP);
			}
			drawList->AddPolyline(screenQuadPts, 4, directionColor[i], true, 1.0f);
			drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
		 }
	  }

	  drawList->AddCircleFilled(gContext.mScreenSquareCenter, 6.f, colors[0], 32);

	  if (gContext.mbUsing)
	  {
		 ImVec2 sourcePosOnScreen = worldToPos(gContext.mMatrixOrigin, gContext.mViewProjection);
		 ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
		 vec_t dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.f, 0.f };
		 dif.Normalize();
		 dif *= 5.f;
		 drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
		 drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
		 drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);

		 char tmps[512];
		 vec_t deltaInfo = gContext.mModel.v.position - gContext.mMatrixOrigin;
		 int componentInfoIndex = (type - MOVE_X) * 3;
		 ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
		 drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
	  }
   }

   static bool CanActivate()
   {
	  if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
		 return true;
	  return false;
   }

   static void HandleAndDrawLocalBounds(float *bounds, matrix_t *matrix, float *snapValues, OPERATION operation)
   {
	   ImGuiIO& io = ImGui::GetIO();
	   ImDrawList* drawList = gContext.mDrawList;

	   // compute best projection axis
	   vec_t axesWorldDirections[3];
	   vec_t bestAxisWorldDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
	   int axes[3];
	   unsigned int numAxes = 1;
	   axes[0] = gContext.mBoundsBestAxis;
	   int bestAxis = axes[0];
	   if (!gContext.mbUsingBounds)
	   {
		   numAxes = 0;
		   float bestDot = 0.f;
		   for (unsigned int i = 0; i < 3; i++)
		   {
			   vec_t dirPlaneNormalWorld;
			   dirPlaneNormalWorld.TransformVector(directionUnary[i], gContext.mModelSource);
			   dirPlaneNormalWorld.Normalize();

			   float dt = fabsf( Dot(Normalized(gContext.mCameraEye - gContext.mModelSource.v.position), dirPlaneNormalWorld) );
			   if ( dt >= bestDot )
			   {
				   bestDot = dt;
				   bestAxis = i;
				   bestAxisWorldDirection = dirPlaneNormalWorld;
			   }

			   if( dt >= 0.1f )
			   {
				   axes[numAxes] = i;
				   axesWorldDirections[numAxes] = dirPlaneNormalWorld;
				   ++numAxes;
			   }
		   }
	   }

	   if( numAxes == 0 )
	   {
			axes[0] = bestAxis;
			axesWorldDirections[0] = bestAxisWorldDirection;
			numAxes = 1;
	   }
	   else if( bestAxis != axes[0] )
	   {
		  unsigned int bestIndex = 0;
		  for (unsigned int i = 0; i < numAxes; i++)
		  {
			  if( axes[i] == bestAxis )
			  {
				  bestIndex = i;
				  break;
			  }
		  }
		  int tempAxis = axes[0];
		  axes[0] = axes[bestIndex];
		  axes[bestIndex] = tempAxis;
		  vec_t tempDirection = axesWorldDirections[0];
		  axesWorldDirections[0] = axesWorldDirections[bestIndex];
		  axesWorldDirections[bestIndex] = tempDirection;
	   }

	   for (unsigned int axisIndex = 0; axisIndex < numAxes; ++axisIndex)
	   {
		   bestAxis = axes[axisIndex];
		   bestAxisWorldDirection = axesWorldDirections[axisIndex];

		   // corners
		   vec_t aabb[4];

		   int secondAxis = (bestAxis + 1) % 3;
		   int thirdAxis = (bestAxis + 2) % 3;

		   for (int i = 0; i < 4; i++)
		   {
			   aabb[i][3] = aabb[i][bestAxis] = 0.f;
			   aabb[i][secondAxis] = bounds[secondAxis + 3 * (i >> 1)];
			   aabb[i][thirdAxis] = bounds[thirdAxis + 3 * ((i >> 1) ^ (i & 1))];
		   }

		   // draw bounds
		   unsigned int anchorAlpha = gContext.mbEnable ? 0xFF000000 : 0x80000000;

		   matrix_t boundsMVP = gContext.mModelSource * gContext.mViewProjection;
		   for (int i = 0; i < 4;i++)
		   {
			   ImVec2 worldBound1 = worldToPos(aabb[i], boundsMVP);
			   ImVec2 worldBound2 = worldToPos(aabb[(i+1)%4], boundsMVP);
			   if( !IsInContextRect( worldBound1 ) || !IsInContextRect( worldBound2 ) )
			   {
				   continue;
			   }
			   float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
			   int stepCount = (int)(boundDistance / 10.f);
			   stepCount = min( stepCount, 1000 );
			   float stepLength = 1.f / (float)stepCount;
			   for (int j = 0; j < stepCount; j++)
			   {
				   float t1 = (float)j * stepLength;
				   float t2 = (float)j * stepLength + stepLength * 0.5f;
				   ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
				   ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
				   //drawList->AddLine(worldBoundSS1, worldBoundSS2, 0x000000 + anchorAlpha, 3.f);
			   drawList->AddLine(worldBoundSS1, worldBoundSS2, 0xAAAAAA + anchorAlpha, 2.f);
			   }
			   vec_t midPoint = (aabb[i] + aabb[(i + 1) % 4] ) * 0.5f;
			   ImVec2 midBound = worldToPos(midPoint, boundsMVP);
			   static const float AnchorBigRadius = 8.f;
			   static const float AnchorSmallRadius = 6.f;
			   bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
			   bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);

			int type = NONE;
			vec_t gizmoHitProportion;

			switch (operation)
			{
			case TRANSLATE: type = GetMoveType(&gizmoHitProportion); break;
			case ROTATE: type = GetRotateType(); break;
			case SCALE: type = GetScaleType(); break;
			case BOUNDS: break;
			}
			if (type != NONE)
			{
			   overBigAnchor = false;
			   overSmallAnchor = false;
			}


			   unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);
			   unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);

			   drawList->AddCircleFilled(worldBound1, AnchorBigRadius, 0xFF000000);
			drawList->AddCircleFilled(worldBound1, AnchorBigRadius-1.2f, bigAnchorColor);

			   drawList->AddCircleFilled(midBound, AnchorSmallRadius, 0xFF000000);
			drawList->AddCircleFilled(midBound, AnchorSmallRadius-1.2f, smallAnchorColor);
			   int oppositeIndex = (i + 2) % 4;
			   // big anchor on corners
			   if (!gContext.mbUsingBounds && gContext.mbEnable && overBigAnchor && CanActivate())
			   {
				   gContext.mBoundsPivot.TransformPoint(aabb[(i + 2) % 4], gContext.mModelSource);
				   gContext.mBoundsAnchor.TransformPoint(aabb[i], gContext.mModelSource);
				   gContext.mBoundsPlan = BuildPlan(gContext.mBoundsAnchor, bestAxisWorldDirection);
				   gContext.mBoundsBestAxis = bestAxis;
				   gContext.mBoundsAxis[0] = secondAxis;
				   gContext.mBoundsAxis[1] = thirdAxis;

				   gContext.mBoundsLocalPivot.Set(0.f);
				   gContext.mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
				   gContext.mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

				   gContext.mbUsingBounds = true;
				   gContext.mBoundsMatrix = gContext.mModelSource;
			   }
			   // small anchor on middle of segment
			   if (!gContext.mbUsingBounds && gContext.mbEnable && overSmallAnchor && CanActivate())
			   {
				   vec_t midPointOpposite = (aabb[(i + 2) % 4] + aabb[(i + 3) % 4]) * 0.5f;
				   gContext.mBoundsPivot.TransformPoint(midPointOpposite, gContext.mModelSource);
				   gContext.mBoundsAnchor.TransformPoint(midPoint, gContext.mModelSource);
				   gContext.mBoundsPlan = BuildPlan(gContext.mBoundsAnchor, bestAxisWorldDirection);
				   gContext.mBoundsBestAxis = bestAxis;
				   int indices[] = { secondAxis , thirdAxis };
				   gContext.mBoundsAxis[0] = indices[i%2];
				   gContext.mBoundsAxis[1] = -1;

				   gContext.mBoundsLocalPivot.Set(0.f);
				   gContext.mBoundsLocalPivot[gContext.mBoundsAxis[0]] = aabb[oppositeIndex][indices[i % 2]];// bounds[gContext.mBoundsAxis[0]] * (((i + 1) & 2) ? 1.f : -1.f);

				   gContext.mbUsingBounds = true;
				   gContext.mBoundsMatrix = gContext.mModelSource;
			   }
		   }

		   if (gContext.mbUsingBounds)
		   {
			   matrix_t scale;
			   scale.SetToIdentity();

			   // compute projected mouse position on plan
			   const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mBoundsPlan);
			   vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;

			   // compute a reference and delta vectors base on mouse move
			   vec_t deltaVector = (newPos - gContext.mBoundsPivot).Abs();
			   vec_t referenceVector = (gContext.mBoundsAnchor - gContext.mBoundsPivot).Abs();

			   // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
			   for (int i = 0; i < 2; i++)
			   {
				   int axisIndex1 = gContext.mBoundsAxis[i];
				   if (axisIndex1 == -1)
					   continue;

				   float ratioAxis = 1.f;
				   vec_t axisDir = gContext.mBoundsMatrix.component[axisIndex1].Abs();

				   float dtAxis = axisDir.Dot(referenceVector);
				   float boundSize = bounds[axisIndex1 + 3] - bounds[axisIndex1];
				   if (dtAxis > FLT_EPSILON)
					   ratioAxis = axisDir.Dot(deltaVector) / dtAxis;

				   if (snapValues)
				   {
					   float length = boundSize * ratioAxis;
					   ComputeSnap(&length, snapValues[axisIndex1]);
					   if (boundSize > FLT_EPSILON)
						   ratioAxis = length / boundSize;
				   }
				   scale.component[axisIndex1] *= ratioAxis;
			   }

			   // transform matrix
			   matrix_t preScale, postScale;
			   preScale.Translation(-gContext.mBoundsLocalPivot);
			   postScale.Translation(gContext.mBoundsLocalPivot);
			   matrix_t res = preScale * scale * postScale * gContext.mBoundsMatrix;
			   *matrix = res;

			   // info text
			   char tmps[512];
			   ImVec2 destinationPosOnScreen = worldToPos(gContext.mModel.v.position, gContext.mViewProjection);
			   ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
				   , (bounds[3] - bounds[0]) * gContext.mBoundsMatrix.component[0].Length() * scale.component[0].Length()
				   , (bounds[4] - bounds[1]) * gContext.mBoundsMatrix.component[1].Length() * scale.component[1].Length()
				   , (bounds[5] - bounds[2]) * gContext.mBoundsMatrix.component[2].Length() * scale.component[2].Length()
			   );
			   drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
			   drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
			}

		   if (!io.MouseDown[0])
			   gContext.mbUsingBounds = false;

		   if( gContext.mbUsingBounds )
			   break;
	   }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   static int GetScaleType()
   {
	  ImGuiIO& io = ImGui::GetIO();
	  int type = NONE;

	  // screen
	  if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
		 io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y)
		 type = SCALE_XYZ;

	  // compute
	  for (unsigned int i = 0; i < 3 && type == NONE; i++)
	  {
		 vec_t dirPlaneX, dirPlaneY, dirAxis;
		 bool belowAxisLimit, belowPlaneLimit;
		 ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

	   const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.v.position, dirAxis));
	   vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

	   const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gContext.mViewProjection);
	   const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
	   const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

	   vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

	   if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
		  type = SCALE_X + i;
	  }
	  return type;
   }

   static int GetRotateType()
   {
	  ImGuiIO& io = ImGui::GetIO();
	  int type = NONE;

	  vec_t deltaScreen = { io.MousePos.x - gContext.mScreenSquareCenter.x, io.MousePos.y - gContext.mScreenSquareCenter.y, 0.f, 0.f };
	  float dist = deltaScreen.Length();
	  if (dist >= (gContext.mRadiusSquareCenter - 1.0f) && dist < (gContext.mRadiusSquareCenter + 1.0f))
		 type = ROTATE_SCREEN;

	  const vec_t planNormals[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir};

	  for (unsigned int i = 0; i < 3 && type == NONE; i++)
	  {
		 // pickup plan
		 vec_t pickupPlan = BuildPlan(gContext.mModel.v.position, planNormals[i]);

		 const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, pickupPlan);
		 vec_t localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.v.position;

		 if (Dot(Normalized(localPos), gContext.mRayVector) > FLT_EPSILON)
			continue;
	   vec_t idealPosOnCircle = Normalized(localPos);
	   idealPosOnCircle.TransformVector(gContext.mModelInverse);
	   ImVec2 idealPosOnCircleScreen = worldToPos(idealPosOnCircle * gContext.mScreenFactor, gContext.mMVP);

	   //gContext.mDrawList->AddCircle(idealPosOnCircleScreen, 5.f, 0xFFFFFFFF);
	   ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

		 float distance = makeVect(distanceOnScreen).Length();
		 if (distance < 8.f) // pixel size
			type = ROTATE_X + i;
	  }

	  return type;
   }

   static int GetMoveType(vec_t *gizmoHitProportion)
   {
	  ImGuiIO& io = ImGui::GetIO();
	  int type = NONE;

	  // screen
	  if (io.MousePos.x >= gContext.mScreenSquareMin.x && io.MousePos.x <= gContext.mScreenSquareMax.x &&
		 io.MousePos.y >= gContext.mScreenSquareMin.y && io.MousePos.y <= gContext.mScreenSquareMax.y)
		 type = MOVE_SCREEN;

	  // compute
	  for (unsigned int i = 0; i < 3 && type == NONE; i++)
	  {
		 vec_t dirPlaneX, dirPlaneY, dirAxis;
		 bool belowAxisLimit, belowPlaneLimit;
		 ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
	   dirAxis.TransformVector(gContext.mModel);
		 dirPlaneX.TransformVector(gContext.mModel);
		 dirPlaneY.TransformVector(gContext.mModel);

		 const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, BuildPlan(gContext.mModel.v.position, dirAxis));
		 vec_t posOnPlan = gContext.mRayOrigin + gContext.mRayVector * len;

	   const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gContext.mViewProjection);
	   const ImVec2 axisStartOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor * 0.1f, gContext.mViewProjection);
	   const ImVec2 axisEndOnScreen = worldToPos(gContext.mModel.v.position + dirAxis * gContext.mScreenFactor, gContext.mViewProjection);

	   vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

	   if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
			type = MOVE_X + i;

	   const float dx = dirPlaneX.Dot3((posOnPlan - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor));
	   const float dy = dirPlaneY.Dot3((posOnPlan - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor));
		 if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
			type = MOVE_YZ + i;

		 if (gizmoHitProportion)
			*gizmoHitProportion = makeVect(dx, dy, 0.f);
	  }
	  return type;
   }

   static void HandleTranslation(float *matrix, float *deltaMatrix, int& type, float *snap)
   {
	  ImGuiIO& io = ImGui::GetIO();
	  bool applyRotationLocaly = gContext.mMode == LOCAL || type == MOVE_SCREEN;

	  // move
	  if (gContext.mbUsing)
	  {
		 ImGui::CaptureMouseFromApp();
		 const float len = fabsf(IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan)); // near plan
		 vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;



		 // compute delta
		 vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
		 vec_t delta = newOrigin - gContext.mModel.v.position;

		 // 1 axis constraint
		 if (gContext.mCurrentOperation >= MOVE_X && gContext.mCurrentOperation <= MOVE_Z)
		 {
			int axisIndex = gContext.mCurrentOperation - MOVE_X;
			const vec_t& axisValue = *(vec_t*)&gContext.mModel.m[axisIndex];
			float lengthOnAxis = Dot(axisValue, delta);
			delta = axisValue * lengthOnAxis;
		 }

		 // snap
		 if (snap)
		 {
			vec_t cumulativeDelta = gContext.mModel.v.position + delta - gContext.mMatrixOrigin;
			if (applyRotationLocaly)
			{
			   matrix_t modelSourceNormalized = gContext.mModelSource;
			   modelSourceNormalized.OrthoNormalize();
			   matrix_t modelSourceNormalizedInverse;
			   modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
			   cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
			   ComputeSnap(cumulativeDelta, snap);
			   cumulativeDelta.TransformVector(modelSourceNormalized);
			}
			else
			{
			   ComputeSnap(cumulativeDelta, snap);
			}
			delta = gContext.mMatrixOrigin + cumulativeDelta - gContext.mModel.v.position;

		 }

		 // compute matrix & delta
		 matrix_t deltaMatrixTranslation;
		 deltaMatrixTranslation.Translation(delta);
		 if (deltaMatrix)
			memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);


		 matrix_t res = gContext.mModelSource * deltaMatrixTranslation;
		 *(matrix_t*)matrix = res;

		 if (!io.MouseDown[0])
			gContext.mbUsing = false;

		 type = gContext.mCurrentOperation;
	  }
	  else
	  {
		 // find new possible way to move
		 vec_t gizmoHitProportion;
		 type = GetMoveType(&gizmoHitProportion);
		 if(type != NONE)
		 {
			ImGui::CaptureMouseFromApp();
		 }
	   if (CanActivate() && type != NONE)
	   {
		  gContext.mbUsing = true;
		  gContext.mCurrentOperation = type;
		  vec_t movePlanNormal[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir,
			 gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir,
			 -gContext.mCameraDir };

		  vec_t cameraToModelNormalized = Normalized(gContext.mModel.v.position - gContext.mCameraEye);
		  for (unsigned int i = 0; i < 3; i++)
		  {
			 vec_t orthoVector = Cross(movePlanNormal[i], cameraToModelNormalized);
			 movePlanNormal[i].Cross(orthoVector);
			 movePlanNormal[i].Normalize();
		  }
			// pickup plan
			gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, movePlanNormal[type - MOVE_X]);
			const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
			gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
			gContext.mMatrixOrigin = gContext.mModel.v.position;

			gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor);
		 }
	  }
   }

   static void HandleScale(float *matrix, float *deltaMatrix, int& type, float *snap)
   {
	  ImGuiIO& io = ImGui::GetIO();

	  if (!gContext.mbUsing)
	  {
		 // find new possible way to scale
		 type = GetScaleType();
		 if(type != NONE)
		 {
			ImGui::CaptureMouseFromApp();
		 }
		 if (CanActivate() && type != NONE)
		 {
			gContext.mbUsing = true;
			gContext.mCurrentOperation = type;
			const vec_t movePlanNormal[] = { gContext.mModel.v.up, gContext.mModel.v.dir, gContext.mModel.v.right, gContext.mModel.v.dir, gContext.mModel.v.up, gContext.mModel.v.right, -gContext.mCameraDir };
			// pickup plan

			gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, movePlanNormal[type - SCALE_X]);
			const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
			gContext.mTranslationPlanOrigin = gContext.mRayOrigin + gContext.mRayVector * len;
			gContext.mMatrixOrigin = gContext.mModel.v.position;
			gContext.mScale.Set(1.f, 1.f, 1.f);
			gContext.mRelativeOrigin = (gContext.mTranslationPlanOrigin - gContext.mModel.v.position) * (1.f / gContext.mScreenFactor);
			gContext.mScaleValueOrigin = makeVect(gContext.mModelSource.v.right.Length(), gContext.mModelSource.v.up.Length(), gContext.mModelSource.v.dir.Length());
			gContext.mSaveMousePosx = io.MousePos.x;
		 }
	  }
	  // scale
	  if (gContext.mbUsing)
	  {
		 ImGui::CaptureMouseFromApp();
		 const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
		 vec_t newPos = gContext.mRayOrigin + gContext.mRayVector * len;
		 vec_t newOrigin = newPos - gContext.mRelativeOrigin * gContext.mScreenFactor;
		 vec_t delta = newOrigin - gContext.mModel.v.position;

		 // 1 axis constraint
		 if (gContext.mCurrentOperation >= SCALE_X && gContext.mCurrentOperation <= SCALE_Z)
		 {
			int axisIndex = gContext.mCurrentOperation - SCALE_X;
			const vec_t& axisValue = *(vec_t*)&gContext.mModel.m[axisIndex];
			float lengthOnAxis = Dot(axisValue, delta);
			delta = axisValue * lengthOnAxis;

			vec_t baseVector = gContext.mTranslationPlanOrigin - gContext.mModel.v.position;
			float ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

			gContext.mScale[axisIndex] = max(ratio, 0.001f);
		 }
		 else
		 {
			float scaleDelta = (io.MousePos.x - gContext.mSaveMousePosx)  * 0.01f;
			gContext.mScale.Set(max(1.f + scaleDelta, 0.001f));
		 }

		 // snap
		 if (snap)
		 {
			float scaleSnap[] = { snap[0], snap[0], snap[0] };
			ComputeSnap(gContext.mScale, scaleSnap);
		 }

		 // no 0 allowed
		 for (int i = 0; i < 3;i++)
			gContext.mScale[i] = max(gContext.mScale[i], 0.001f);

		 // compute matrix & delta
		 matrix_t deltaMatrixScale;
		 deltaMatrixScale.Scale(gContext.mScale * gContext.mScaleValueOrigin);

		 matrix_t res = deltaMatrixScale * gContext.mModel;
		 *(matrix_t*)matrix = res;

		 if (deltaMatrix)
		 {
			deltaMatrixScale.Scale(gContext.mScale);
			memcpy(deltaMatrix, deltaMatrixScale.m16, sizeof(float) * 16);
		 }

		 if (!io.MouseDown[0])
			gContext.mbUsing = false;

		 type = gContext.mCurrentOperation;
	  }
   }

   static void HandleRotation(float *matrix, float *deltaMatrix, int& type, float *snap)
   {
	  ImGuiIO& io = ImGui::GetIO();
	  bool applyRotationLocaly = gContext.mMode == LOCAL;

	  if (!gContext.mbUsing)
	  {
		 type = GetRotateType();

		 if(type != NONE)
		 {
			ImGui::CaptureMouseFromApp();
		 }

		 if (type == ROTATE_SCREEN)
		 {
			applyRotationLocaly = true;
		 }

		 if (CanActivate() && type != NONE)
		 {
			gContext.mbUsing = true;
			gContext.mCurrentOperation = type;
			const vec_t rotatePlanNormal[] = { gContext.mModel.v.right, gContext.mModel.v.up, gContext.mModel.v.dir, -gContext.mCameraDir };
			// pickup plan
			if (applyRotationLocaly)
			{
			   gContext.mTranslationPlan = BuildPlan(gContext.mModel.v.position, rotatePlanNormal[type - ROTATE_X]);
			}
			else
			{
			   gContext.mTranslationPlan = BuildPlan(gContext.mModelSource.v.position, directionUnary[type - ROTATE_X]);
			}

			const float len = IntersectRayPlane(gContext.mRayOrigin, gContext.mRayVector, gContext.mTranslationPlan);
			vec_t localPos = gContext.mRayOrigin + gContext.mRayVector * len - gContext.mModel.v.position;
			gContext.mRotationVectorSource = Normalized(localPos);
			gContext.mRotationAngleOrigin = ComputeAngleOnPlan();
		 }
	  }

	  // rotation
	  if (gContext.mbUsing)
	  {
		 ImGui::CaptureMouseFromApp();
		 gContext.mRotationAngle = ComputeAngleOnPlan();
		 if (snap)
		 {
			float snapInRadian = snap[0] * DEG2RAD;
			ComputeSnap(&gContext.mRotationAngle, snapInRadian);
		 }
		 vec_t rotationAxisLocalSpace;

		 rotationAxisLocalSpace.TransformVector(makeVect(gContext.mTranslationPlan.x, gContext.mTranslationPlan.y, gContext.mTranslationPlan.z, 0.f), gContext.mModelInverse);
		 rotationAxisLocalSpace.Normalize();

		 matrix_t deltaRotation;
		 deltaRotation.RotationAxis(rotationAxisLocalSpace, gContext.mRotationAngle - gContext.mRotationAngleOrigin);
		 gContext.mRotationAngleOrigin = gContext.mRotationAngle;

		 matrix_t scaleOrigin;
		 scaleOrigin.Scale(gContext.mModelScaleOrigin);

		 if (applyRotationLocaly)
		 {
			*(matrix_t*)matrix = scaleOrigin * deltaRotation * gContext.mModel;
		 }
		 else
		 {
			matrix_t res = gContext.mModelSource;
			res.v.position.Set(0.f);

			*(matrix_t*)matrix = res * deltaRotation;
			((matrix_t*)matrix)->v.position = gContext.mModelSource.v.position;
		 }

		 if (deltaMatrix)
		 {
			*(matrix_t*)deltaMatrix = gContext.mModelInverse * deltaRotation * gContext.mModel;
		 }

		 if (!io.MouseDown[0])
			gContext.mbUsing = false;

		 type = gContext.mCurrentOperation;
	  }
   }

   void DecomposeMatrixToComponents(const float *matrix, float *translation, float *rotation, float *scale)
   {
	  matrix_t mat = *(matrix_t*)matrix;

	  scale[0] = mat.v.right.Length();
	  scale[1] = mat.v.up.Length();
	  scale[2] = mat.v.dir.Length();

	  mat.OrthoNormalize();

	  rotation[0] = RAD2DEG * atan2f(mat.m[1][2], mat.m[2][2]);
	  rotation[1] = RAD2DEG * atan2f(-mat.m[0][2], sqrtf(mat.m[1][2] * mat.m[1][2] + mat.m[2][2]* mat.m[2][2]));
	  rotation[2] = RAD2DEG * atan2f(mat.m[0][1], mat.m[0][0]);

	  translation[0] = mat.v.position.x;
	  translation[1] = mat.v.position.y;
	  translation[2] = mat.v.position.z;
   }

   void RecomposeMatrixFromComponents(const float *translation, const float *rotation, const float *scale, float *matrix)
   {
	  matrix_t& mat = *(matrix_t*)matrix;

	  matrix_t rot[3];
	  for (int i = 0; i < 3;i++)
		 rot[i].RotationAxis(directionUnary[i], rotation[i] * DEG2RAD);

	  mat = rot[0] * rot[1] * rot[2];

	  float validScale[3];
	  for (int i = 0; i < 3; i++)
	  {
		 if (fabsf(scale[i]) < FLT_EPSILON)
			validScale[i] = 0.001f;
		 else
			validScale[i] = scale[i];
	  }
	  mat.v.right *= validScale[0];
	  mat.v.up *= validScale[1];
	  mat.v.dir *= validScale[2];
	  mat.v.position.Set(translation[0], translation[1], translation[2], 1.f);
   }

   void Manipulate(const float *view, const float *projection, OPERATION operation, MODE mode, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
   {
	  ComputeContext(view, projection, matrix, mode);

	  // set delta to identity
	  if (deltaMatrix)
		 ((matrix_t*)deltaMatrix)->SetToIdentity();

	  // behind camera
	  vec_t camSpacePosition;
	  camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gContext.mMVP);
	  if (!gContext.mIsOrthographic && camSpacePosition.z < 0.001f)
		 return;

	  // --
	  int type = NONE;
	  if (gContext.mbEnable)
	  {
		  if (!gContext.mbUsingBounds)
		  {
			  switch (operation)
			  {
			  case ROTATE:
				  HandleRotation(matrix, deltaMatrix, type, snap);
				  break;
			  case TRANSLATE:
				  HandleTranslation(matrix, deltaMatrix, type, snap);
				  break;
			  case SCALE:
				  HandleScale(matrix, deltaMatrix, type, snap);
				  break;
			  case BOUNDS:
				  break;
			  }
		  }
	  }

	  if (localBounds && !gContext.mbUsing)
		  HandleAndDrawLocalBounds(localBounds, (matrix_t*)matrix, boundsSnap, operation);

	  if (!gContext.mbUsingBounds)
	  {
		  switch (operation)
		  {
		  case ROTATE:
			  DrawRotationGizmo(type);
			  break;
		  case TRANSLATE:
			  DrawTranslationGizmo(type);
			  break;
		  case SCALE:
			  DrawScaleGizmo(type);
			  break;
		  case BOUNDS:
			  break;
		  }
	  }
   }

   void DrawCube(const float *view, const float *projection, const float *matrix)
   {
	  matrix_t viewInverse;
	  viewInverse.Inverse(*(matrix_t*)view);
	  const matrix_t& model = *(matrix_t*)matrix;
	  matrix_t res = *(matrix_t*)matrix * *(matrix_t*)view * *(matrix_t*)projection;

	  for (int iFace = 0; iFace < 6; iFace++)
	  {
		 const int normalIndex = (iFace % 3);
		 const int perpXIndex = (normalIndex + 1) % 3;
		 const int perpYIndex = (normalIndex + 2) % 3;
		 const float invert = (iFace > 2) ? -1.f : 1.f;

		 const vec_t faceCoords[4] = { directionUnary[normalIndex] + directionUnary[perpXIndex] + directionUnary[perpYIndex],
			directionUnary[normalIndex] + directionUnary[perpXIndex] - directionUnary[perpYIndex],
			directionUnary[normalIndex] - directionUnary[perpXIndex] - directionUnary[perpYIndex],
			directionUnary[normalIndex] - directionUnary[perpXIndex] + directionUnary[perpYIndex],
		 };

		 // clipping
		 bool skipFace = false;
		 for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
		 {
			vec_t camSpacePosition;
			camSpacePosition.TransformPoint(faceCoords[iCoord] * 0.5f * invert, gContext.mMVP);
			if (camSpacePosition.z < 0.001f)
			{
			   skipFace = true;
			   break;
			}
		 }
		 if (skipFace)
			continue;

		 // 3D->2D
		 ImVec2 faceCoordsScreen[4];
		 for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
			faceCoordsScreen[iCoord] = worldToPos(faceCoords[iCoord] * 0.5f * invert, res);

		 // back face culling
		 vec_t cullPos, cullNormal;
		 cullPos.TransformPoint(faceCoords[0] * 0.5f * invert, model);
		 cullNormal.TransformVector(directionUnary[normalIndex] * invert, model);
		 float dt = Dot(Normalized(cullPos - viewInverse.v.position), Normalized(cullNormal));
		 if (dt>0.f)
			continue;

		 // draw face with lighter color
		 gContext.mDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, directionColor[normalIndex] | 0x808080);
	  }
   }

   void DrawGrid(const float *view, const float *projection, const float *matrix, const float gridSize)
   {
	  matrix_t res = *(matrix_t*)matrix * *(matrix_t*)view * *(matrix_t*)projection;

	  for (float f = -gridSize; f <= gridSize; f += 1.f)
	  {
		 gContext.mDrawList->AddLine(worldToPos(makeVect(f, 0.f, -gridSize), res), worldToPos(makeVect(f, 0.f, gridSize), res), 0xFF808080);
		 gContext.mDrawList->AddLine(worldToPos(makeVect(-gridSize, 0.f, f), res), worldToPos(makeVect(gridSize, 0.f, f), res), 0xFF808080);
	  }
   }
};

