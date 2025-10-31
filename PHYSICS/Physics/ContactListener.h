#pragma once
#include "RP3D_Wrappers.h"
#include "UserData.h"

namespace ENGINE_PHYSICS {
	class ContactListener :public EventListener
	{
	private:
		std::vector<std::pair<UserData*, UserData*>> m_ContactPairs;

	public:
		void onContact(const CollisionCallback::CallbackData& callbackData) override;
		void onTrigger(const OverlapCallback::CallbackData& callbackData) override;

		std::vector<std::pair<UserData*, UserData*>> GetContactPairs() { return m_ContactPairs; }
	};
}