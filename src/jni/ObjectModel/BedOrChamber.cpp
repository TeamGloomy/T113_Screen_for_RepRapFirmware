/*
 * BedOrChamber.cpp
 *
 *  Created on: 17 Feb 2021
 *      Author: manuel
 */

#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_WARN

#include "BedOrChamber.h"
#include "Configuration.h"
#include "Hardware/Duet.h"
#include "ListHelpers.h"
#include "ObjectModel/Heat.h"
#include <Duet3D/General/String.h>
#include <Duet3D/General/Vector.h>

#include "Debug.h"

typedef Vector<OM::Bed*, MAX_SLOTS> BedList;
typedef Vector<OM::Chamber*, MAX_SLOTS> ChamberList;

static BedList s_beds;
static ChamberList s_chambers;

namespace OM
{
	int8_t g_lastBed = -1;
	int8_t g_lastChamber = -1;

	void BedOrChamber::Reset()
	{
		index = 0;
		heater = -1;
		slot = MAX_SLOTS;
	}

	int32_t BedOrChamber::GetActiveTemp()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return 0;

		return pheater->activeTemp;
	}

	int32_t BedOrChamber::GetStandbyTemp()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return 0;

		return pheater->standbyTemp;
	}

	float BedOrChamber::GetMin()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return 0;

		return pheater->min;
	}

	float BedOrChamber::GetMax()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return 0;

		return pheater->max;
	}

	bool BedOrChamber::SetBedTemp(const int32_t temp, const bool active)
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return false;

		String<MAX_COMMAND_LENGTH> command;
		command.catf("M140 P%d %s%d", index, active ? "S" : "R", temp);

		Comm::DUET.SendGcode(command.c_str());
		return true;
	}

	bool BedOrChamber::SetChamberTemp(const int32_t temp, const bool active)
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return false;

		String<MAX_COMMAND_LENGTH> command;
		command.catf("M141 P%d %s%d", index, active ? "S" : "R", temp);

		Comm::DUET.SendGcode(command.c_str());
		return true;
	}

	bool BedOrChamber::ToggleBedState()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return false;

		switch (pheater->status)
		{
		case Heat::HeaterStatus::active:
			Comm::DUET.SendGcodef("M144 P%d", index);
			break;
		case Heat::HeaterStatus::standby:
			Comm::DUET.SendGcodef("M140 P%d S-273.15", index);
			break;
		case Heat::HeaterStatus::off:
			Comm::DUET.SendGcodef("M140 P%d S%d", index, pheater->activeTemp);
			break;
		case Heat::HeaterStatus::fault:
			Comm::DUET.SendGcodef("M562 P%d", pheater->index);
			break;
		case Heat::HeaterStatus::offline:
		case Heat::HeaterStatus::tuning:
			break;
		}
		return true;
	}

	bool BedOrChamber::ToggleChamberState()
	{
		Heat::Heater* pheater = Heat::GetHeater(heater);
		if (pheater == nullptr)
			return false;

		switch (pheater->status)
		{
		case Heat::HeaterStatus::active:
		case Heat::HeaterStatus::standby:
			Comm::DUET.SendGcodef("M141 P%d S-273.15", index);
			break;
		case Heat::HeaterStatus::off:
			Comm::DUET.SendGcodef("M141 P%d S%d", index, pheater->activeTemp);
			break;
		case Heat::HeaterStatus::fault:
			Comm::DUET.SendGcodef("M562 P%d", pheater->index);
			break;
		case Heat::HeaterStatus::offline:
		case Heat::HeaterStatus::tuning:
			break;
		}
		return true;
	}

	Bed* GetBedBySlot(const size_t index)
	{
		if (index >= s_beds.Size())
			return nullptr;

		return s_beds[index];
	}

	Bed* GetBed(const size_t index)
	{
		return GetOrCreate<BedList, Bed>(s_beds, index, false);
	}

	Bed* GetOrCreateBed(const size_t index)
	{
		return GetOrCreate<BedList, Bed>(s_beds, index, true);
	}

	Bed* GetFirstBed()
	{
		return Find<BedList, Bed>(s_beds, [](Bed* bed) { return bed->heater > -1; });
	}

	size_t GetBedCount()
	{
		return s_beds.Size();
	}

	bool IterateBedsWhile(function_ref<bool(Bed*&, size_t)> func, const size_t startAt)
	{
		return s_beds.IterateWhile(func, startAt);
	}

	size_t RemoveBed(const size_t index, const bool allFollowing)
	{
		info("Removing bed %d allFollowing=%s", index, allFollowing ? "true" : "false");
		return Remove<BedList, Bed>(s_beds, index, allFollowing);
	}

	Chamber* GetChamberBySlot(const size_t index)
	{
		if (index >= s_chambers.Size())
			return nullptr;

		return s_chambers[index];
	}

	Chamber* GetChamber(const size_t index)
	{
		return GetOrCreate<ChamberList, Chamber>(s_chambers, index, false);
	}

	Chamber* GetOrCreateChamber(const size_t index)
	{
		return GetOrCreate<ChamberList, Chamber>(s_chambers, index, true);
	}

	Chamber* GetFirstChamber()
	{
		return Find<ChamberList, Chamber>(s_chambers, [](Chamber* chamber) { return chamber->heater > -1; });
	}

	size_t GetChamberCount()
	{
		return s_chambers.Size();
	}

	bool IterateChambersWhile(function_ref<bool(Chamber*&, size_t)> func,
			const size_t startAt)
	{
		return s_chambers.IterateWhile(func, startAt);
	}

	size_t RemoveChamber(const size_t index, const bool allFollowing)
	{
		return Remove<ChamberList, Chamber>(s_chambers, index, allFollowing);
	}

	bool SetBedHeater(const uint8_t bedIndex, const int8_t heaterNumber)
	{
		auto bed = OM::GetOrCreateBed(bedIndex);
		if (bed == nullptr)
		{
			error("Failed to get or create bed %d", bedIndex);
			return false;
		}
		bed->heater = heaterNumber;
		info("Created bed %d, heater number %d", bedIndex, heaterNumber);
		return true;

	}

	bool SetChamberHeater(const uint8_t chamberIndex, const int8_t heaterNumber)
	{
		auto chamber = OM::GetOrCreateChamber(chamberIndex);
		if (chamber == nullptr)
		{
			return false;
		}
		chamber->heater = heaterNumber;
		return true;
	}
} // namespace OM
