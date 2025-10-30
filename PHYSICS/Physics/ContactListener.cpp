#include "ContactListener.h"

namespace ENGINE_PHYSICS {
	void ContactListener::SetUserContacts(UserData* a, UserData* b)
	{
		m_pUserDataA = a;
		m_pUserDataB = b;
	}

	void ContactListener::onContact(const CollisionCallback::CallbackData& callbackData)
	{
		//UserData* a_data = reinterpret_cast<UserData*>(callbackData.getContactPair().g)
	}

	void ContactListener::onTrigger(const OverlapCallback::CallbackData& callbackData)
	{
	}
}


