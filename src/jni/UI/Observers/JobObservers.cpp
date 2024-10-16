/*
 * JobObservers.cpp
 *
 *  Created on: 9 Jan 2024
 *      Author: Andy Everitt
 */
#include "Debug.h"

#include "Configuration.h"
#include "UI/Logic/ObjectCancel.h"
#include "UI/OmObserver.h"
#include "UI/UserInterface.h"
#include "manager/LanguageManager.h"
#include <algorithm>

#include "ObjectModel/Job.h"
#include "UI/Logic/PrintStatus.h"

/*
 * These functions are run when the OM field is received.
 * The function takes 2 arguments:
 * 	- val data from the OM where the type is automatically converted based on the chosen macro
 * 	- indices is an array of the indicies for the OM key
 * The _IF_CHANGED suffix only runs the function if the data is different from the previous
 * time function was called. This is unique to each combination of indices.
 */
static UI::Observer<UI::ui_field_update_cb> JobObserversField[] = {
	OBSERVER_CHAR("job:file:fileName",
				  [](OBSERVER_CHAR_ARGS)
				  {
					  OM::SetJobName(val);
					  UI::PrintStatus::UpdateFileName();
				  }),
	OBSERVER_CHAR("job:lastFileName",
				  [](OBSERVER_CHAR_ARGS)
				  {
					  OM::SetLastJobName(val);
					  UI::PrintStatus::UpdateFileName();
				  }),
	OBSERVER_UINT("job:file:printTime", [](OBSERVER_UINT_ARGS) { OM::SetPrintTime(val); }),
	OBSERVER_UINT("job:duration",
				  [](OBSERVER_UINT_ARGS)
				  {
					  OM::SetPrintDuration(val);
					  UI::PrintStatus::UpdateElapsedTime(val);
				  }),
	OBSERVER_UINT("job:timesLeft:slicer",
				  [](OBSERVER_UINT_ARGS)
				  {
					  OM::SetPrintRemaining(OM::RemainingTimeType::slicer, val);
					  UI::PrintStatus::UpdateEstimatedPrintTime();
				  }),
	OBSERVER_UINT("job:warmUpDuration",
				  [](OBSERVER_UINT_ARGS)
				  {
					  OM::SetWarmUpDuration(val);
					  UI::PrintStatus::UpdateEstimatedPrintTime();
				  }),
	OBSERVER_CHAR("job:build",
				  [](OBSERVER_CHAR_ARGS)
				  {
					  dbg("Job: build is null");
					  OM::RemoveJobObject(indices[0], true);
					  UI::ObjectCancel::RenderObjectMap();
				  }),
	OBSERVER_INT("job:build:currentObject", [](OBSERVER_INT_ARGS) { OM::SetCurrentJobObject(val); }),
	OBSERVER_CHAR("job:build:objects^", [](OBSERVER_CHAR_ARGS) { OM::RemoveJobObject(indices[0], false); }),
	OBSERVER_BOOL("job:build:objects^:cancelled",
				  [](OBSERVER_BOOL_ARGS)
				  {
					  OM::JobObject* jobObject = OM::GetOrCreateJobObject(indices[0]);
					  if (jobObject == nullptr)
					  {
						  warn("Job object %u not found", indices[0]);
					  }
					  jobObject->cancelled = val;
				  }),
	OBSERVER_CHAR("job:build:objects^:name",
				  [](OBSERVER_CHAR_ARGS)
				  {
					  OM::JobObject* jobObject = OM::GetOrCreateJobObject(indices[0]);
					  if (jobObject == nullptr)
					  {
						  warn("Job object %u not found", indices[0]);
					  }
					  jobObject->name = val;
				  }),
	OBSERVER_INT("job:build:objects^:x^",
				 [](OBSERVER_INT_ARGS)
				 {
					 OM::JobObject* jobObject = OM::GetOrCreateJobObject(indices[0]);
					 if (jobObject == nullptr)
					 {
						 warn("Job object %u not found", indices[0]);
					 }
					 if (indices[1] < 0 || indices[1] >= 2)
					 {
						 warn("Job object %u x index %u out of range", indices[0], indices[1]);
						 return;
					 }
					 jobObject->bounds.x[indices[1]] = val;
				 }),
	OBSERVER_INT("job:build:objects^:y^",
				 [](OBSERVER_INT_ARGS)
				 {
					 OM::JobObject* jobObject = OM::GetOrCreateJobObject(indices[0]);
					 if (jobObject == nullptr)
					 {
						 warn("Job object %u not found", indices[0]);
					 }
					 if (indices[1] < 0 || indices[1] >= 2)
					 {
						 warn("Job object %u y index %u out of range", indices[0], indices[1]);
						 return;
					 }
					 jobObject->bounds.y[indices[1]] = val;
				 }),
};

/*
 * These functions are run when the end of an array has been received from the OM
 * The function needs to take in an array containing the indices of the OM key
 */
static UI::Observer<UI::ui_array_end_update_cb> JobObserversArrayEnd[] = {
	OBSERVER_ARRAY_END("job:build^", [](OBSERVER_ARRAY_END_ARGS) { dbg("Job: build end %u", indices[0]); }),
	OBSERVER_ARRAY_END("job:build:objects^", [](OBSERVER_ARRAY_END_ARGS) { OM::RemoveJobObject(indices[0], true); }),
};
