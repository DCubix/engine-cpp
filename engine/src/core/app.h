#ifndef APP_H
#define APP_H

#include "logging/log.h"
#include "types.h"
#include "msg.h"

#include "SDL.h"

NS_BEGIN

struct ApplicationConfig {
	u32 width;
	u32 height;
	bool fullScreen;
	String title;
	i32 frameCap;

	ApplicationConfig()
		: width(640), height(480), fullScreen(false), title("Engine"), frameCap(60)
	{}
};

class IApplicationAdapter {
public:
	virtual void init() = 0;
	virtual void update(float timeDelta) = 0;
	virtual void render() = 0;
	virtual ~IApplicationAdapter() {}

	ApplicationConfig config;
};

class Application final : public IObject {
public:
	Application() = default;
	Application(IApplicationAdapter* adapter, ApplicationConfig config = ApplicationConfig());

	void run();

	void processMessage(const Message& msg);

	ApplicationConfig config() const { return m_config; }

	Application(const Application&) = delete;
	Application& operator =(const Application&) = delete;
private:
	void eng_mainloop();

	uptr<IApplicationAdapter> m_applicationAdapter;
	ApplicationConfig m_config;

	bool m_running;

	// Internals
	SDL_Window *m_window;
	SDL_GLContext m_context;
};

NS_END

#endif // APP_H
