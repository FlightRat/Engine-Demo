#include "ContactListener.h"
#include <Logger.h>

namespace ENGINE_PHYSICS {
	void ContactListener::onContact(const rp3d::CollisionCallback::CallbackData& callbackData)
	{
		for (uint p = 0; p < callbackData.getNbContactPairs(); p++)
		{
			CollisionCallback::ContactPair contactPair = callbackData.getContactPair(p);
			auto evenType = contactPair.getEventType();
			if (evenType == rp3d::CollisionCallback::ContactPair::EventType::ContactStart)
			{
				UserData* a_data = reinterpret_cast<UserData*>(contactPair.getBody1()->getUserData());
				UserData* b_data = reinterpret_cast<UserData*>(contactPair.getBody2()->getUserData());

				try
				{
					auto a_any = std::any_cast<ObjectData>(a_data->userData);
					auto b_any = std::any_cast<ObjectData>(b_data->userData);
					//if (a_any.entityID > b_any.entityID)
					//	std::swap(a_any, b_any);
					
					a_any.AddContact(b_any);
					a_data->userData.reset();
					a_data->userData = a_any;

					b_any.AddContact(a_any);
					b_data->userData.reset();
					b_data->userData = b_any;

					auto currentPair = std::make_pair(a_data, b_data);
					if (std::find(m_ContactPairs.begin(), m_ContactPairs.end(), currentPair) == m_ContactPairs.end())
						m_ContactPairs.push_back(currentPair);
				}
				catch (const std::bad_any_cast& ex)
				{
					ENGINE_ERROR("Failed to cast user contacts: {}", ex.what());
				}
			}
			else if(evenType == rp3d::CollisionCallback::ContactPair::EventType::ContactExit)
			{
				UserData* a_data = reinterpret_cast<UserData*>(contactPair.getBody1()->getUserData());
				UserData* b_data = reinterpret_cast<UserData*>(contactPair.getBody2()->getUserData());

				try
				{
					auto a_any = std::any_cast<ObjectData>(a_data->userData);
					auto b_any = std::any_cast<ObjectData>(b_data->userData);
			/*		if (a_any.entityID > b_any.entityID)
						std::swap(a_any, b_any);*/


					if (!a_any.RemoveContact(b_any))
					{
						//todo: error log
					}
					if (!b_any.RemoveContact(a_any))
					{
						//todo: error log
					}

					a_data->userData.reset();
					a_data->userData = a_any;

					b_data->userData.reset();
					b_data->userData = b_any;

					auto currentPair = std::make_pair(a_data, b_data);
					m_ContactPairs.erase(std::remove(m_ContactPairs.begin(), m_ContactPairs.end(), currentPair),m_ContactPairs.end());
				}
				catch (const std::bad_any_cast& ex)
				{
					ENGINE_ERROR("Failed to cast user contacts: {}", ex.what());
				}
			}
		}
	}

	void ContactListener::onTrigger(const OverlapCallback::CallbackData& callbackData)
	{
	}
}


