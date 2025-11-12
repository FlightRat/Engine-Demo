#include "UserData.h"

namespace ENGINE_PHYSICS {
	bool operator==(const ObjectData& a, const ObjectData& b)
	{
		return a.entityID == b.entityID;
	}

	bool ObjectData::AddContact(const ObjectData& objectData)
	{
		if (tag.empty() && group.empty())
			return false;
		if (objectData.tag.empty() && objectData.group.empty())
			return false;

		// return the "index" of the found target which follow the function
		auto contactItr = std::find_if(
			contactEntities.begin(), contactEntities.end(),
			[&](ObjectData& contactInfo) {
				return contactInfo == objectData;
			}
		);

		if (contactItr != contactEntities.end()) // foumd, which means already exists
			return false;

		contactEntities.push_back(objectData);
		return true;
	}

	bool ObjectData::RemoveContact(const ObjectData& objectData)
	{
		auto contactItr = std::find_if(
			contactEntities.begin(), contactEntities.end(),
			[&](ObjectData& contactInfo) {
				return contactInfo == objectData;
			}
		);

		if (contactItr == contactEntities.end())
			return false;

		contactEntities.erase(contactItr);
		return true;
	}

	std::string ObjectData::to_string() const
	{
		std::stringstream ss;
		ss <<
			"=== Object Data === \n" << std::boolalpha <<
			"Tag: " << tag << "\n" <<
			"Group: " << group << "\n" <<
			"bCollider: " << bCollider << "\n" <<
			"bTrigger: " << bTrigger << "\n" <<
			"EntityID: " << entityID << "\n";
		return ss.str();
	}
}


