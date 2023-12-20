/*
 * Heat.hpp
 *
 *  Created on: 19 Dec 2023
 *      Author: Andy Everitt
 */

#ifndef JNI_LOGIC_OBJECTMODEL_HEAT_HPP_
#define JNI_LOGIC_OBJECTMODEL_HEAT_HPP_

#include <sys/types.h>
#include "Spindle.hpp"
#include <Duet3D/General/Bitmap.h>
#include <Duet3D/General/FreelistManager.h>
#include <Duet3D/General/String.h>
#include <Duet3D/General/StringRef.h>
#include <Duet3D/General/function_ref.h>
#include <UI/UserInterfaceConstants.hpp>

#define TOOL_NAME_MAX_LEN 10

namespace OM
{
	namespace Heat
	{
		// Status that a tool may report to us. Must be in alphabetical order.
		enum class HeaterStatus
		{
			active = 0,
			fault,
			off,
			standby,
		};

		struct HeaterStatusMapEntry
		{
			const char* key;
			HeaterStatus val;
		};

		// This table must be kept in case-insensitive alphabetical order of the search string.
		const HeaterStatusMapEntry heaterStatusMap[] =
		{
			{ "active",		HeaterStatus::active	},
			{ "fault", 		HeaterStatus::fault		},
			{ "off",		HeaterStatus::off 		},
			{ "standby",	HeaterStatus::standby 	},
		};

		struct Heater
		{
			void* operator new(size_t) noexcept { return FreelistManager::Allocate<Heater>(); }
			void operator delete(void* p) noexcept { FreelistManager::Release<Heater>(p); }

			uint8_t index;	// This is the heater number
			int32_t activeTemp;
			int32_t standbyTemp;
			float current;
			float avgPwm;
			HeaterStatus status;

			void Reset();
			int32_t GetTemperature();
			int32_t GetHeaterTarget(const bool active);
			const char * GetHeaterStatusStr();
			void UpdateTarget(const int32_t temp, const bool active);
			void UpdateTemp(const float temp) { current = temp; }
		};

		Heater* GetHeater(const size_t heaterIndex);
		Heater* GetOrCreateHeater(const size_t heaterIndex);
		bool IterateHeatersWhile(function_ref<bool(Heater*&, size_t)> func, const size_t startAt = 0);
		bool UpdateHeaterTarget(const size_t heaterIndex, const int32_t temp, const bool active);
		bool UpdateHeaterTemp(const size_t heaterIndex, const float temp);
		bool UpdateHeaterStatus(const size_t heaterIndex, HeaterStatus status);
		bool UpdateHeaterStatus(const size_t heaterIndex, const char *status);
		size_t RemoveHeater(const size_t index, const bool allFollowing);

		extern size_t lastHeater;
	}
}

#endif /* JNI_LOGIC_OBJECTMODEL_HEAT_HPP_ */
