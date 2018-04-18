#ifndef MSG_H
#define MSG_H

#include "types.h"

NS_BEGIN

class Message {
public:
	String type;
};

class IObject {
public:
	virtual void processMessage(const Message& msg) = 0;
};

class MessageSystem {
public:
	MessageSystem() {}

	void subscribe(IObject* obj);
	void submit(const String& type, Message* msg = nullptr);
	void submitAndSend(const String& type, Message* msg = nullptr);

	void send();

	static MessageSystem& ston() { return *s_ston; }
private:
	MessageSystem(const MessageSystem&) = delete;
	MessageSystem& operator =(const MessageSystem&) = delete;

	Queue<uptr<Message>> m_messages;
	Vector<IObject*> m_subscribers;

	static uptr<MessageSystem> s_ston;
};

NS_END

#endif // MSG_H