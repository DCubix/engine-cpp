#ifndef INPUT_H
#define INPUT_H

#include "SDL.h"

#include "types.h"
#include "../math/vec.h"

using ProcessCallback = Fn<void(SDL_Event&)>;

struct State {
	bool pressed, released, down;
};

class Input {
	friend class Application;
public:
	static void update(const ProcessCallback& customProcess = nullptr);

	static const State getKeyboardState(int key);
	static const State getMouseState(int button);

	static bool isKeyPressed(int key);
	static bool isKeyReleased(int key);
	static bool isKeyDown(int key);

	static bool isMouseButtonPressed(int btn);
	static bool isMouseButtonReleased(int btn);
	static bool isMouseButtonDown(int btn);

	static i32 getScrollOffset() { i32 off = m_scrollOffset; m_scrollOffset = 0; return off; }
	static i32 getMouseX() { return m_mouseX; }
	static i32 getMouseY() { return m_mouseY; }

	static Vec2 getMousePosition();
	static void setMousePosition(int x, int y);
	static void setMousePosition(const Vec2& pos);

	static bool isCloseRequested() { return m_closeRequested; }

	static void setCursorVisible(bool state);

private:
	static SDL_Event m_sdlEvent;
	static UMap<int, State> m_keyboard;
	static UMap<int, State> m_mouse;
	static i32 m_mouseX, m_mouseY;
	static i32 m_scrollOffset;
	static bool m_closeRequested;

protected:
	static SDL_Window* m_window;
};

#endif // INPUT_H
