#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

#include "types.h"

struct State {
	bool pressed, released, down;
};

class Input {
public:
	static void update();

	static const State getKeyboardState(int key);
	static const State getMouseState(int button);

	static bool isKeyPressed(int key);
	static bool isKeyReleased(int key);
	static bool isKeyDown(int key);

	static bool isMouseButtonPressed(int btn);
	static bool isMouseButtonReleased(int btn);
	static bool isMouseButtonDown(int btn);

	static i32 getScrollOffset() { return m_scrollOffset; }
	static i32 getMouseX() { return m_mouseX; }
	static i32 getMouseY() { return m_mouseY; }

	static bool isCloseRequested() { return m_closeRequested; }

private:
	static SDL_Event m_sdlEvent;
	static UMap<int, State> m_keyboard;
	static UMap<int, State> m_mouse;
	static i32 m_mouseX, m_mouseY;
	static i32 m_scrollOffset;
	static bool m_closeRequested;
};

#endif // INPUT_H