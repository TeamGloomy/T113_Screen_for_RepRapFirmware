#pragma once
#define DEBUG (1)

#include <vector>
#include <string>
#include "Communication.hpp"
#include "Hardware/SerialIo.hpp"
#include "ObjectModel/Utils.hpp"
#include "UI/UserInterface.hpp"
#include "UI/UserInterfaceConstants.hpp"
#include "UI/Colors.hpp"
#include "UI/Gcodes.hpp"
#include "UI/OmObserver.hpp"
#include "UI/MainMenu.h"
#include "Debug.hpp"


/*
 * This file is generated by the GUI tool
 * File function: logic corresponding code for processing users
 * Function Description:
 *========================onButtonClick_XXXX
 When the button on the page is pressed, the system will call the corresponding function.
 XXX represents the [ID value] name in the GUI tool, such as Button1.
 When the return value is false, the system will no longer process this button,
 and when it returns true, the system will continue to process this button. Such as SYS_BACK.
 *========================onSlideWindowItemClick_XXXX(int index) 
 When there is a sliding window on the page and the user clicks the icon of the sliding window, the system will call this function.
 XXX represents the [ID value] name in the GUI tool, for example: slideWindow1.
 index represents the offset value of the pressed icon
 *========================onSeekBarChange_XXXX(int progress) 
 When there is a slider on the page and the user changes the progress, the system will call this function.
 XXX represents the [ID value] name in the GUI tool, such as SeekBar1.
 progress represents the current progress value
 *========================ogetListItemCount_XXXX()
 When there is a sliding list on the page, the system will call this interface to obtain the total number of lists when updating.
 XXX represents the [ID value] name in the GUI tool, such as List1.
 The return value is the total number of items in the current list
 *========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 When there is a sliding list on the page, the system will call this interface
 to obtain the content information under the current item of the list when updating,
 XXX represents the [ID value] name in the GUI tool. For example, List1.
 pListItem is a single item object in the map and index is the offset of the list's total purpose.
 See function description for details
 *========================Commonly used interface===============
 * LOGD(...)  interface to print debugging information
 * mTextXXXPtr->setText("****") Display text on the control TextXXX
 * mButton1Ptr->setSelected(true); Set the control mButton1 to the selected mode, the picture will be switched to the selected picture, and the button text will be switched to the selected color
 * mSeekBarPtr->setProgress(12) Adjust the progress to 12 on the control mSeekBar
 * mListView1Ptr->refreshListView() Make mListView1 re-refresh, call when the list data changes
 * mDashbroadView1Ptr->setTargetAngle(120) Adjust the pointer display angle on the control mDashbroadView1 to 120 degrees
 *
 * In Eclipse IDE, use the "alt + /" shortcut key to open the smart prompt
 */

#define TIMER_UPDATE_DATA 0  // Id of the timer used to update values from data received from serial

/**
 * Register timer
 * Fill the array to register the timer
 * Note: id cannot be repeated
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{TIMER_UPDATE_DATA,  1000}, // Timer id=0, interval of 500ms
//	{1,  100},
};

/**
 * Triggered when the interface is constructed
 */
static void onUI_init()
{
	// Tips : Add the display code for UI initialization here, such as: mText1Ptr->setText("123");
	srand(0);

	/* Initialise UI observer updaters that run on each field value that is received from the OM */
	auto *observer = UI::omFieldObserverHead;
	while (observer != nullptr)
	{
		observer->Init(UI::observerMap);
		dbg("Initialised field observer against key %s (%d)",
				observer->GetKey(),
				UI::observerMap.GetObservers(observer->GetKey()).size());
		observer = observer->next;
	}

	/* Initialise UI observer updaters that run after an array has been received */
	auto *observerArrayEnd = UI::omArrayEndObserverHead;
	while (observerArrayEnd != nullptr)
	{
		observerArrayEnd->Init(UI::observerMapArrayEnd);
		dbg("Initialised array end observer against key %s (%d)",
				observerArrayEnd->GetKey(),
				UI::observerMapArrayEnd.GetObservers(observerArrayEnd->GetKey()).size());
		observerArrayEnd = observerArrayEnd->next;
	}

	UI::TOOLSLIST->Init(mToolListViewPtr, mTemperatureInputWindowPtr, mNumPadInputPtr);
	UI::CONSOLE->Init(mConsoleListViewPtr, mEditText1Ptr);
}

/**
 * Triggered when switching to this interface
 */
static void onUI_intent(const Intent *intentPtr)
{
	if (intentPtr != NULL)
	{
		//TODO
	}
}

/*
 * Triggered when the interface is displayed
 */
static void onUI_show()
{

}

/*
 * Triggered when the interface is hidden
 */
static void onUI_hide()
{

}

/*
 * Triggered when the interface completely exits
 */
static void onUI_quit()
{

}

/**
 * Serial data callback interface
 */
static void onProtocolDataUpdate(const SProtocolData &rxData)
{
//	dbg("data %s", rxData.data);
	SerialIo::CheckInput(rxData.data, rxData.len);
}

/**
 * Timer trigger function
 * It is not recommended to write time-consuming operations in this function, otherwise it will affect UI refresh
 * Parameters:
 * 	- id: The id of the currently triggered timer is the same as the id at registration
 * Return value:
 * 	- True: Continue to run the current timer
 * 	- False: Stop running the current timer
 */
static bool onUI_Timer(int id)
{
	switch (id)
	{

	case TIMER_UPDATE_DATA:
	{
		Comm::sendNext();
	}
		break;

	case 1:
	{
		float val = (float) (rand() % 100);
		dbg("rand %f", val);
		mTempGraphPtr->addData(0, val);
	}
		break;
	default:
		break;
	}
	//LOGD("controlTabAxisPos %s", UI::controlTabAxisPos[0]->GetValue());

//	mAxisSlot1_labelPtr->setText(UI::controlTabAxisPos[0]->GetLabel());

	return true;
}

/**
 * Triggered when there is a new touch event
 * Parameters:
 * 	- ev: New touch event
 * Return value:
 * 	- True: Indicates that the touch event is intercepted here, and the system will no longer pass this touch event to the control
 * 	- False: Touch events will continue to be passed to the control
 */
static bool onmainActivityTouchEvent(const MotionEvent &ev)
{
	switch (ev.mActionStatus)
	{
	case MotionEvent::E_ACTION_DOWN: // touch pressed
		//LOGD("Time = %ld Pos x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
		break;
	case MotionEvent::E_ACTION_MOVE: // touch slide
		break;
	case MotionEvent::E_ACTION_UP: // touch released
		break;
	default:
		break;
	}
	return false;
}
static bool onButtonClick_HomeBtn(ZKButton *pButton)
{
	LOGD(" ButtonClick HomeBtn !!!\n");
	UI::WINDOW->Home();
	return false;
}
static bool onButtonClick_BackBtn(ZKButton *pButton)
{
	dbg("Back btn pressed, returning to previous window");
	UI::WINDOW->Back();
	return false;
}

static bool onButtonClick_MacroBtn(ZKButton *pButton)
{
	OM::RequestFiles("0:/macros");
	if (mMainWindowPtr->isWndShow())
		UI::WINDOW->CloseWindow(mMainWindowPtr);
	UI::WINDOW->CloseLastWindow();
	UI::WINDOW->OpenWindow(mFilesWindowPtr);
	return false;
}

static bool onButtonClick_EStopBtn(ZKButton *pButton)
{
	LOGD(" ButtonClick EStopBtn !!!\n");
	return false;
}

static int getListItemCount_ToolListView(const ZKListView *pListView)
{
	size_t count = UI::TOOLSLIST->GetTotalHeaterCount(false);
//	dbg("Setting ToolList length to %d", count);
	return count;
}

static void obtainListItemData_ToolListView(ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index)
{
	ZKListView::ZKListSubItem *ptoolName = pListItem->findSubItemByID(ID_MAIN_ToolNameSubItem);
	ZKListView::ZKListSubItem *pcurrentTemperature = pListItem->findSubItemByID(ID_MAIN_ToolCurrentTemperatureSubItem);
	ZKListView::ZKListSubItem *pactiveTemperature = pListItem->findSubItemByID(ID_MAIN_ToolActiveTemperatureSubItem);
	ZKListView::ZKListSubItem *pstandbyTemperature = pListItem->findSubItemByID(ID_MAIN_ToolStandbyTemperatureSubItem);
	ZKListView::ZKListSubItem *pstatus = pListItem->findSubItemByID(ID_MAIN_ToolStatusSubItem);

	// Check Tools to see if list index is within tool heaters
	OM::Tool *tool = nullptr;
	int8_t toolHeaterIndex = UI::GetToolHeaterIndex(index, tool);
	if (tool != nullptr)
	{
		OM::ToolHeater *toolHeater;
		toolHeater = tool->GetHeater(toolHeaterIndex);
		if (toolHeater == nullptr)
		{
			dbg("List index %d: Tool %d heaterIndex %d is null", index, tool->index, toolHeaterIndex);
			ptoolName->setText(tool->name.c_str());
			return;
		}
		dbg("List index %d: Updating Tool %d heater %d=%d temperatures %.2f:%d:%d",
				index, tool->index, toolHeaterIndex, toolHeater->heater->index,
				toolHeater->heater->current, toolHeater->activeTemp, toolHeater->standbyTemp);
		if (tool->GetHeaterCount() > 1)
		{
			ptoolName->setTextf("%s (%d)", tool->name.c_str(), toolHeaterIndex);
		}
		else
		{
			ptoolName->setText(tool->name.c_str());
		}
		pactiveTemperature->setText(toolHeater->activeTemp);
		pstandbyTemperature->setText(toolHeater->standbyTemp);
		pcurrentTemperature->setText(toolHeater->heater->current);
		pstatus->setText(toolHeater->heater->GetHeaterStatusStr());
		(tool->status == OM::ToolStatus::active) ? pListItem->setSelected(true) : pListItem->setSelected(false);

		return;
	}

	int8_t bedOrChamberIndex = index - toolHeaterIndex;
	OM::BedOrChamber *bedOrChamber = OM::GetBedBySlot(bedOrChamberIndex);
	OM::Heat::Heater *heater;
	if (bedOrChamber != nullptr)
	{
		heater = OM::Heat::GetHeater(bedOrChamber->heater);
		if (heater == nullptr)
		{
			dbg("List index %d: Bed %d heater %d is null", index, bedOrChamber->index, bedOrChamber->heater);
			return;
		}
		dbg("List index %d: Updating Bed %d heater %d=%d temperatures %.2f:%d:%d",
				index, bedOrChamber->index, bedOrChamber->heater, heater->index,
				heater->current, heater->activeTemp, heater->standbyTemp);
		if (OM::GetBedCount() > 1)
		{
			ptoolName->setTextf("Bed %d", bedOrChamber->index );
		}
		else
		{
			ptoolName->setText("Bed");
		}
		pactiveTemperature->setText(heater->activeTemp);
		pstandbyTemperature->setText(heater->standbyTemp);
		pcurrentTemperature->setText(heater->current);
		pstatus->setText(heater->GetHeaterStatusStr());
		pListItem->setSelected(false);
		return;
	}

	bedOrChamberIndex -= OM::GetBedCount();
	dbg("Chamber index %d", bedOrChamberIndex);
	bedOrChamber = OM::GetChamberBySlot(bedOrChamberIndex);
	if (bedOrChamber != nullptr)
	{
		heater = OM::Heat::GetHeater(bedOrChamber->heater);
		if (heater == nullptr)
		{
			dbg("List index %d: Bed %d heater %d is null", index, bedOrChamber->index, bedOrChamber->heater);
			return;
		}
		dbg("List index %d: Updating Chamber %d heater %d=%d temperatures %.2f:%d:%d",
				index, bedOrChamber->index, bedOrChamber->heater, heater->index,
				heater->current, heater->activeTemp, heater->standbyTemp);
		if (OM::GetChamberCount() > 1)
		{
			ptoolName->setTextf("Chamber %d", bedOrChamber->index );
		}
		else
		{
			ptoolName->setText("Chamber");
		}
		pactiveTemperature->setText(heater->activeTemp);
		pstandbyTemperature->setText(heater->standbyTemp);
		pcurrentTemperature->setText(heater->current);
		pstatus->setText(heater->GetHeaterStatusStr());
		pListItem->setSelected(false);
		return;
	}
	dbg("Unknown index");
}

static void onListItemClick_ToolListView(ZKListView *pListView, int index, int id)
{
//	LOGD(" onListItemClick_ ToolListView  !!!\n");
	dbg("index=%d, id=%d", index, id);
	OM::Tool *tool = nullptr;
	int8_t toolHeaterIndex = UI::GetToolHeaterIndex(index, tool);
	UI::NumPadData numPadData;
	if (tool != nullptr)
	{
		dbg("Tool index=%d", tool->index);
		numPadData.heaterType = UI::HeaterType::tool;
		numPadData.toolIndex = tool->index;
		numPadData.toolHeaterIndex = toolHeaterIndex;
		switch (id)
		{
		case ID_MAIN_ToolActiveTemperatureSubItem:
			numPadData.active = true;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStandbyTemperatureSubItem:
			numPadData.active = false;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStatusSubItem:
			tool->ToggleHeaterState(toolHeaterIndex);
			break;
		case ID_MAIN_ToolNameSubItem:
			tool->ToggleState();
			break;
		}
		return;
	}

	int8_t bedOrChamberIndex = index - toolHeaterIndex;
	OM::Bed *bedOrChamber = OM::GetBedBySlot(bedOrChamberIndex);
	if (bedOrChamber != nullptr)
	{
		dbg("Bed index=%d", bedOrChamber->index);
		numPadData.heaterType = UI::HeaterType::bed;
		numPadData.bedOrChamberIndex = bedOrChamber->index;
		switch (id)
		{
		case ID_MAIN_ToolActiveTemperatureSubItem:
			numPadData.active = true;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStandbyTemperatureSubItem:
			numPadData.active = false;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStatusSubItem:
		case ID_MAIN_ToolNameSubItem:
			bedOrChamber->ToggleBedState();
			break;
		}
		return;
	}

	bedOrChamberIndex -= OM::GetBedCount();
	bedOrChamber = OM::GetChamberBySlot(bedOrChamberIndex);
	if (bedOrChamber != nullptr)
	{
		dbg("Chamber index=%d", bedOrChamber->index);
		numPadData.heaterType = UI::HeaterType::chamber;
		numPadData.bedOrChamberIndex = bedOrChamber->index;
		switch (id)
		{
		case ID_MAIN_ToolActiveTemperatureSubItem:
			numPadData.active = true;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStandbyTemperatureSubItem:
			numPadData.active = false;
			UI::WINDOW->CloseWindow(mWindowSelectWindowPtr);
			UI::TOOLSLIST->OpenNumPad(numPadData);
			break;
		case ID_MAIN_ToolStatusSubItem:
		case ID_MAIN_ToolNameSubItem:
			bedOrChamber->ToggleChamberState();
			break;
		}
		return;
	}
	dbg("Unknown index");
}

static void onSlideItemClick_SlideWindow1(ZKSlideWindow *pSlideWindow, int index)
{
	LOGD(" onSlideItemClick_ SlideWindow1 %d !!!\n", index);
	UI::WINDOW->CloseWindow(mMainWindowPtr);
	switch (index)
	{
	case (int)UI::SlideWindowIndex::move:
		UI::WINDOW->OpenWindow(mMoveWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::extrude:
		UI::WINDOW->OpenWindow(mExtrudeWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::console:
		UI::WINDOW->OpenWindow(mConsoleWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::heightmap:
		UI::WINDOW->OpenWindow(mHeightMapWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::fans:
		UI::WINDOW->OpenWindow(mFanWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::print:
		OM::RequestFiles("0:/gcodes");
		UI::WINDOW->OpenWindow(mFilesWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::network:
		UI::WINDOW->OpenWindow(mNetworkWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::settings:
		UI::WINDOW->OpenWindow(mSettingsWindowPtr);
		break;
	}
}
static int getListItemCount_TemperatureGraphLegend(const ZKListView *pListView)
{
	//LOGD("getListItemCount_TemperatureGraphLegend !\n");
	return 5;
}

static void obtainListItemData_TemperatureGraphLegend(ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index)
{
	//LOGD(" obtainListItemData_ TemperatureGraphLegend  !!!\n");
}

static void onListItemClick_TemperatureGraphLegend(ZKListView *pListView, int index, int id)
{
	//LOGD(" onListItemClick_ TemperatureGraphLegend  !!!\n");
}

static bool onButtonClick_NumPad1(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('1');
    return false;
}

static bool onButtonClick_NumPad2(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('2');
    return false;
}

static bool onButtonClick_NumPad3(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('3');
    return false;
}

static bool onButtonClick_NumPad4(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('4');
    return false;
}

static bool onButtonClick_NumPad5(ZKButton *pButton) {
    UI::TOOLSLIST->NumPadAddOneChar('5');
    return false;
}

static bool onButtonClick_NumPad6(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('6');
    return false;
}

static bool onButtonClick_NumPad7(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('7');
    return false;
}

static bool onButtonClick_NumPad8(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('8');
    return false;
}

static bool onButtonClick_NumPad9(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('9');
    return false;
}

static bool onButtonClick_NumPadDel(ZKButton *pButton) {
    UI::TOOLSLIST->NumPadDelOneChar();
    return false;
}

static bool onButtonClick_NumPad0(ZKButton *pButton) {
	UI::TOOLSLIST->NumPadAddOneChar('0');
    return false;
}

static bool onButtonClick_NumPadConfirm(ZKButton *pButton) {
    UI::TOOLSLIST->SendTempTarget();
    UI::TOOLSLIST->CloseNumPad();
    return false;
}
static bool onButtonClick_HomeAllBtn(ZKButton *pButton) {
    LOGD(" ButtonClick HomeAllBtn !!!\n");
    return false;
}

static int getListItemCount_AxisControlListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_AxisControlListView !\n");
    return 5;
}

static void obtainListItemData_AxisControlListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ AxisControlListView  !!!\n");
}

static void onListItemClick_AxisControlListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ AxisControlListView  !!!\n");
}

static bool onButtonClick_FeedrateBtn1(ZKButton *pButton) {
    LOGD(" ButtonClick FeedrateBtn1 !!!\n");
    return false;
}

static bool onButtonClick_TrueLevelBtn(ZKButton *pButton) {
    LOGD(" ButtonClick TrueLevelBtn !!!\n");
    return false;
}

static bool onButtonClick_MeshLevelBtn(ZKButton *pButton) {
    LOGD(" ButtonClick MeshLevelBtn !!!\n");
    return false;
}

static bool onButtonClick_HeightmapBtn(ZKButton *pButton) {
    LOGD(" ButtonClick HeightmapBtn !!!\n");
    return false;
}

static bool onButtonClick_FeedrateBtn2(ZKButton *pButton) {
    LOGD(" ButtonClick FeedrateBtn2 !!!\n");
    return false;
}

static bool onButtonClick_FeedrateBtn3(ZKButton *pButton) {
    LOGD(" ButtonClick FeedrateBtn3 !!!\n");
    return false;
}

static bool onButtonClick_FeedrateBtn4(ZKButton *pButton) {
    LOGD(" ButtonClick FeedrateBtn4 !!!\n");
    return false;
}

static bool onButtonClick_FeedrateBtn5(ZKButton *pButton) {
    LOGD(" ButtonClick FeedrateBtn5 !!!\n");
    return false;
}
static int getListItemCount_ConsoleListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_ConsoleListView !\n");
    return MaxResponseLines;
}

static void obtainListItemData_ConsoleListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ ConsoleListView  !!!\n");
	pListItem->setText(UI::CONSOLE->GetItem(index).c_str());
}

static void onListItemClick_ConsoleListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ ConsoleListView  !!!\n");
}

static int getListItemCount_GcodeListView(const ZKListView *pListView) {
	return sizeof(Gcode) / sizeof(gcode);
}

static void obtainListItemData_GcodeListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
	pListItem->setText(Gcode[index].displayText);
}

static void onListItemClick_GcodeListView(ZKListView *pListView, int index, int id) {
}

static void onEditTextChanged_EditText1(const std::string &text) {
    //LOGD(" onEditTextChanged_ EditText1 %s !!!\n", text.c_str());
	SerialIo::Sendf("%s\n", text.c_str());
    UI::CONSOLE->AddCommand(text);
}

static bool onButtonClick_SendBtn(ZKButton *pButton) {
    SerialIo::Sendf(mEditText1Ptr->getText().c_str());
    UI::CONSOLE->AddCommand(mEditText1Ptr->getText());
    return true;
}
static bool onButtonClick_ConsoleClearBtn(ZKButton *pButton) {
    UI::CONSOLE->Clear();
    return false;
}

static bool onButtonClick_FileRefreshBtn(ZKButton *pButton) {
	OM::RequestFiles(OM::GetCurrentDirPath());
    return false;
}
static int getListItemCount_FileListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_FileListView !\n");
    return OM::GetItemCount();
}

static void obtainListItemData_FileListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ FileListView  !!!\n");
	ZKListView::ZKListSubItem *pFileType = pListItem->findSubItemByID(ID_MAIN_FileTypeSubItem);
	ZKListView::ZKListSubItem *pFileSize = pListItem->findSubItemByID(ID_MAIN_FileSizeSubItem);
	ZKListView::ZKListSubItem *pFileDate = pListItem->findSubItemByID(ID_MAIN_FileDateSubItem);
	ZKListView::ZKListSubItem *pFileThumbnail = pListItem->findSubItemByID(ID_MAIN_FileThumbnailSubItem);

	OM::FileSystemItem* item = OM::GetItem(index);
	if (item == nullptr)
		return;
//	dbg("Files: settings list item %d name to %s", index, item->GetName().c_str());
	pListItem->setText(item->GetName());
	switch (item->GetType())
	{
	case OM::FileSystemItemType::file:
		pFileType->setText("File");
		break;
	case OM::FileSystemItemType::folder:
		pFileType->setText("Folder");
		break;
	}
	pFileSize->setTextf("Size: %d", item->GetSize());
	pFileDate->setTextf("Date: %s", item->GetDate().c_str());
}

static void onListItemClick_FileListView(ZKListView *pListView, int index, int id) {
	OM::FileSystemItem* item = OM::GetItem(index);
	switch (item->GetType())
	{
	case OM::FileSystemItemType::file:
		OM::StartFile(item->GetPath());
		break;
	case OM::FileSystemItemType::folder:
		OM::RequestFiles(item->GetPath());
		break;
	}
}
