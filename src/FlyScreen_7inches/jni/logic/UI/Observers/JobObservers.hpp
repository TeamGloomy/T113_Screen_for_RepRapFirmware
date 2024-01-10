/*
 * JobObservers.hpp
 *
 *  Created on: 9 Jan 2024
 *      Author: Andy Everitt
 */

#ifndef JNI_LOGIC_UI_OBSERVERS_JOBOBSERVERS_HPP_
#define JNI_LOGIC_UI_OBSERVERS_JOBOBSERVERS_HPP_

#include "Debug.hpp"

#include "ObjectModel/Job.hpp"
#include "UI/OmObserver.hpp"
#include "UI/UserInterface.hpp"
#include "UI/UserInterfaceConstants.hpp"

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
                  [](OBSERVER_CHAR_ARGS) {
                      OM::SetJobName(val);
                      mPrintFileNamePtr->setText(val);
                  }),
    OBSERVER_UINT("job:file:printTime", [](OBSERVER_UINT_ARGS) { OM::SetPrintTime(val); }),
    OBSERVER_UINT("job:duration",
                  [](OBSERVER_UINT_ARGS) {
                      OM::SetPrintDuration(val);
                      mPrintElapsedTimePtr->setTextf("Elapsed: %d", val);
                  }),
    OBSERVER_UINT("job:timesLeft:slicer",
                  [](OBSERVER_UINT_ARGS) {
                      OM::SetPrintRemaining(val);
                      uint32_t percentage = (100 * OM::GetPrintDuration()) / OM::GetPrintTime();
                      mPrintEstimatedTimePtr->setTextf("Estimated: %d", val);
                      mPrintProgressBarPtr->setProgress(percentage);
                  }),
};

/*
 * These functions are run when the end of an array has been received from the OM
 * The function needs to take in an array containing the indices of the OM key
 */
static UI::Observer<UI::ui_array_end_update_cb> JobObserversArrayEnd[] = {
    OBSERVER_ARRAY_END("job", [](OBSERVER_ARRAY_END_ARGS) {}),
};

#endif /* JNI_LOGIC_UI_OBSERVERS_JOBOBSERVERS_HPP_ */
