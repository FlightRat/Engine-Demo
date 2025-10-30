#pragma once
#include "RP3D_Wrappers.h"
#include "UserData.h"

namespace ENGINE_PHYSICS {
	class ContactListener :public EventListener
	{
	private:
		UserData* m_pUserDataA{ nullptr };
		UserData* m_pUserDataB{ nullptr };

		void SetUserContacts(UserData* a, UserData* b);

	public:
		void onContact(const CollisionCallback::CallbackData& callbackData) override;
		void onTrigger(const OverlapCallback::CallbackData& callbackData) override;

		UserData* GetUserDataA() { return m_pUserDataA; }
		UserData* GetUserDataB() { return m_pUserDataB; }
	};
}