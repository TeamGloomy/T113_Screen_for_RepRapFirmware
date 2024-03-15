/*
 * Heat.cpp
 *
 *  Created on: 19 Dec 2023
 *      Author: Andy Everitt
 */

#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_WARN
#include "Configuration.h"
#include "Fan.h"
#include "ListHelpers.h"
#include "ObjectModel/Utils.h"
#include "uart/CommDef.h"
#include <Duet3D/General/Vector.h>

#include "Debug.h"

typedef Vector<OM::Fan*, MAX_FANS> FanList;
static FanList fans;

namespace OM
{
	void Fan::Reset()
	{
		index = 0;
		actualValue = 0;
		requestedValue = 0;
		rpm = -1;
	}

	Fan *GetFan(const size_t index)
	{
		return GetOrCreate<FanList, Fan>(fans, index, false);
	}

	Fan* GetFanBySlot(const size_t index)
	{
		if (index >= fans.Size())
			return nullptr;

		return fans[index];
	}

	Fan *GetOrCreateFan(const size_t index)
	{
		return GetOrCreate<FanList, Fan>(fans, index, true);
	}

	const size_t GetFanCount()
	{
		return fans.Size();
	}

	bool IterateFansWhile(function_ref<bool(Fan *&, size_t)> func, const size_t startAt)
	{
		return fans.IterateWhile(func, startAt);
	}

	bool UpdateFanActualVal(const size_t fanIndex, const float val)
	{
		Fan *fan = GetOrCreateFan(fanIndex);

		// If we do not handle this fan back off
		if (fan == nullptr)
		{
			return false;
		}

		fan->actualValue = val;
		return true;
	}

	bool UpdateFanRequestedVal(const size_t fanIndex, const float val)
	{
		Fan *fan = GetOrCreateFan(fanIndex);

		// If we do not handle this fan back off
		if (fan == nullptr)
		{
			return false;
		}

		fan->requestedValue = val;
		return true;
	}

	bool UpdateFanRpm(const size_t fanIndex, const int32_t val)
	{
		Fan *fan = GetOrCreateFan(fanIndex);

		// If we do not handle this fan back off
		if (fan == nullptr)
		{
			return false;
		}

		fan->rpm = val;
		return true;
	}

	size_t RemoveFan(const size_t index, const bool allFollowing)
	{
		info("Removing fan %d (allFollowing=%s)", index, allFollowing ? "true" : "false");
		return Remove<FanList, Fan>(fans, index, allFollowing);
	}
}
