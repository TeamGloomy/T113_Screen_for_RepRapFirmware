/*
 * Spindle.h
 *
 *  Created on: 17 Feb 2021
 *      Author: manuel
 */

#ifndef SRC_OBJECTMODEL_SPINDLE_HPP_
#define SRC_OBJECTMODEL_SPINDLE_HPP_

//#include <cstdint>
#include <sys/types.h>
#include <Duet3D/General/FreelistManager.h>

namespace OM
{
	enum SpindleState : uint8_t {
		forward,
		reverse,
		stopped,
	};

	struct SpindleStateMapEntry
	{
		const char* key;
		const SpindleState val;
	};

	// This table has to be kept in alphabetical order of the keys
	const SpindleStateMapEntry spindleStateMap[] =
	{
		{ "forward",	SpindleState::forward },
		{ "reverse",	SpindleState::reverse },
		{ "stopped",	SpindleState::stopped },
	};

	struct Spindle
	{
		void* operator new(size_t) noexcept { return FreelistManager::Allocate<Spindle>(); }
		void operator delete(void* p) noexcept { FreelistManager::Release<Spindle>(p); }

		// Index within configured spindles
		uint8_t index;
		uint32_t active;
		bool canReverse;
		uint32_t current;
		uint32_t max;
		uint32_t min;
		SpindleState state;

		void Reset();
		const char* GetStateStr();
	};

	Spindle* GetSpindle(const size_t index);
	Spindle* GetOrCreateSpindle(const size_t index);
	size_t RemoveSpindle(const size_t index, const bool allFollowing);

	bool SetSpindleActive(size_t index, uint32_t active);
	bool SetSpindleCanReverse(size_t index, bool canReverse);
	bool SetSpindleCurrent(size_t index, uint32_t current);
	bool SetSpindleMax(size_t index, uint32_t max);
	bool SetSpindleMin(size_t index, uint32_t min);
	bool SetSpindleState(size_t index, const char* state);
}

#endif /* SRC_OBJECTMODEL_SPINDLE_HPP_ */
