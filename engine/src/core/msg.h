#ifndef MSG_H
#define MSG_H

#include "types.h"

NS_BEGIN

class Message {
	friend class MessageSystem;
public:
	Message() : type(""), data(nullptr) {}
	String type;
	void *data;
protected:
	float time;
};

class IObject {
public:
	virtual void processMessage(const Message& msg) = 0;
};

class MessageSystem {
public:
	MessageSystem() {}

	void subscribe(IObject* obj);
	void submit(const String& type, void* data = nullptr, float delay = 0);

	void processQueue(float dt);

	static MessageSystem& get() { return *s_ston; }
private:
	MessageSystem(const MessageSystem&) = delete;
	MessageSystem& operator =(const MessageSystem&) = delete;

	Queue<uptr<Message>> m_messages;
	Vector<IObject*> m_subscribers;

	static uptr<MessageSystem> s_ston;
};

NS_END

#endif // MSG_H
