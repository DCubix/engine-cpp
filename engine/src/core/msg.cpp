#include "msg.h"

NS_BEGIN

uptr<MessageSystem> MessageSystem::s_ston(new MessageSystem());

void MessageSystem::subscribe(IObject* obj) {
	m_subscribers.push_back(obj);
}

void MessageSystem::submit(const String& type, Message* msg, float delay) {
	if (msg == nullptr) {
		msg = new Message();
	}
	msg->time = delay;
	msg->type = type;
	m_messages.push(uptr<Message>(mov(msg)));
}

void MessageSystem::processQueue(float dt) {
	if (!m_messages.empty()) {
		m_messages.front()->time -= dt;
		if (m_messages.front()->time <= 0) {
			auto msg = mov(m_messages.front());
			m_messages.pop();

			for (auto sub : m_subscribers) {
				sub->processMessage(*msg);
			}
		}
	}
}

NS_END
