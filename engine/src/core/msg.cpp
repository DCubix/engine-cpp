#include "msg.h"

NS_BEGIN

uptr<MessageSystem> MessageSystem::s_ston(new MessageSystem());

void MessageSystem::subscribe(IObject* obj) {
	m_subscribers.push_back(obj);
}

void MessageSystem::submit(const String& type, Message* msg) {
	if (msg == nullptr) {
		msg = new Message();
	}
	msg->type = type;
	m_messages.push(uptr<Message>(mov(msg)));
}

void MessageSystem::submitAndSend(const String& type, Message* msg) {
	submit(type, msg);
	send();
}

void MessageSystem::send() {
	while (!m_messages.empty()) {
		auto msg = mov(m_messages.front());
		m_messages.pop();

		for (auto sub : m_subscribers) {
			sub->processMessage(*msg);
		}
	}
}

NS_END
