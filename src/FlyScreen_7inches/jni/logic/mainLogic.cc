#pragma once
#define DEBUG (1)

#include <vector>
#include <string>
#include "Communication.hpp"
#include "Hardware/SerialIo.hpp"
#include "ObjectModel/Fan.hpp"
#include "ObjectModel/Files.hpp"
#include "ObjectModel/Utils.hpp"
#include "UI/UserInterface.hpp"
#include "UI/UserInterfaceConstants.hpp"
#include "UI/Colors.hpp"
#include "UI/Gcodes.hpp"
#include "UI/OmObserver.hpp"
#include "UI/Observers/FanObservers.hpp"
#include "UI/Observers/FileObservers.hpp"
#include "UI/Observers/ToolObservers.hpp"
#include "UI/Observers/HeatObservers.hpp"
#include "UI/Observers/ResponseObservers.hpp"
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
	{TIMER_UPDATE_DATA,  1000}, // Timer id=0, min interval of 300ms at 115200 baud
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

	UI::WINDOW->AddHome(mMainWindowPtr);
	UI::WINDOW->AddHome(mWindowSelectWindowPtr);
	UI::ToolsList::Create("home")->Init(mToolListViewPtr, mTemperatureInputWindowPtr, mNumPadInputPtr);
	UI::ToolsList::Create("print")->Init(mPrintTemperatureListPtr, mTemperatureInputWindowPtr, mNumPadInputPtr);
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
	OM::FileSystem::RequestFiles("0:/macros");
	if (mMainWindowPtr->isWndShow())
		UI::WINDOW->CloseWindow(mMainWindowPtr);
	UI::WINDOW->CloseLastWindow();
	UI::WINDOW->OpenWindow(mFilesWindowPtr);
	return false;
}

static bool onButtonClick_ConsoleBtn(ZKButton *pButton) {
	if (mMainWindowPtr->isWndShow())
		UI::WINDOW->CloseWindow(mMainWindowPtr);
	UI::WINDOW->CloseLastWindow();
	UI::WINDOW->OpenWindow(mConsoleWindowPtr);
    return false;
}

static bool onButtonClick_EStopBtn(ZKButton *pButton)
{
	LOGD(" ButtonClick EStopBtn !!!\n");
	return false;
}

static int getListItemCount_ToolListView(const ZKListView *pListView)
{
	size_t count = UI::ToolsList::Get("home")->GetTotalHeaterCount(false);
	return count;
}

static void obtainListItemData_ToolListView(ZKListView *pListView, ZKListView::ZKListItem *pListItem, int index)
{
	ZKListView::ZKListSubItem *ptoolName = pListItem->findSubItemByID(ID_MAIN_ToolNameSubItem);
	ZKListView::ZKListSubItem *pcurrentTemperature = pListItem->findSubItemByID(ID_MAIN_ToolCurrentTemperatureSubItem);
	ZKListView::ZKListSubItem *pactiveTemperature = pListItem->findSubItemByID(ID_MAIN_ToolActiveTemperatureSubItem);
	ZKListView::ZKListSubItem *pstandbyTemperature = pListItem->findSubItemByID(ID_MAIN_ToolStandbyTemperatureSubItem);
	ZKListView::ZKListSubItem *pstatus = pListItem->findSubItemByID(ID_MAIN_ToolStatusSubItem);

	UI::ToolsList::Get("home")->ObtainListItemData(pListItem, index, ptoolName, pcurrentTemperature, pactiveTemperature, pstandbyTemperature, pstatus);
}

static void onListItemClick_ToolListView(ZKListView *pListView, int index, int id)
{
	UI::ToolsList::Get("home")->OnListItemClick(
			index, id,
			ID_MAIN_ToolNameSubItem, ID_MAIN_ToolStatusSubItem,
			ID_MAIN_ToolActiveTemperatureSubItem, ID_MAIN_ToolStandbyTemperatureSubItem);
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
	case (int)UI::SlideWindowIndex::status:
		UI::WINDOW->OpenWindow(mPrintWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::heightmap:
		UI::WINDOW->OpenWindow(mHeightMapWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::fans:
		UI::WINDOW->OpenWindow(mFanWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::print:
			OM::FileSystem::RequestFiles("0:/gcodes");
		UI::WINDOW->OpenWindow(mFilesWindowPtr);
		break;
	case (int)UI::SlideWindowIndex::network:
//		UI::WINDOW->OpenWindow(mNetworkWindowPtr);
		EASYUICONTEXT->openActivity("WifiSettingActivity");
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

static UI::ToolsList* getVisibleToolsList()
{
	if (mPrintWindowPtr->isVisible())
		return UI::ToolsList::Get("print");
	return UI::ToolsList::Get("home");
}
static bool onButtonClick_NumPad1(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('1');
	return false;
}

static bool onButtonClick_NumPad2(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('2');
	return false;
}

static bool onButtonClick_NumPad3(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('3');
	return false;
}

static bool onButtonClick_NumPad4(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('4');
	return false;
}

static bool onButtonClick_NumPad5(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('5');
	return false;
}

static bool onButtonClick_NumPad6(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('6');
	return false;
}

static bool onButtonClick_NumPad7(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('7');
	return false;
}

static bool onButtonClick_NumPad8(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('8');
	return false;
}

static bool onButtonClick_NumPad9(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('9');
	return false;
}

static bool onButtonClick_NumPadDel(ZKButton *pButton) {
	getVisibleToolsList()->NumPadDelOneChar();
	return false;
}

static bool onButtonClick_NumPad0(ZKButton *pButton) {
	getVisibleToolsList()->NumPadAddOneChar('0');
	return false;
}

static bool onButtonClick_NumPadConfirm(ZKButton *pButton) {
	getVisibleToolsList()->SendTempTarget();
	getVisibleToolsList()->CloseNumPad();
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
	OM::FileSystem::RequestFiles(OM::FileSystem::GetCurrentDirPath());
    return false;
}
static int getListItemCount_FileListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_FileListView !\n");
    return OM::FileSystem::GetItemCount();
}

static void obtainListItemData_FileListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ FileListView  !!!\n");
	ZKListView::ZKListSubItem *pFileType = pListItem->findSubItemByID(ID_MAIN_FileTypeSubItem);
	ZKListView::ZKListSubItem *pFileSize = pListItem->findSubItemByID(ID_MAIN_FileSizeSubItem);
	ZKListView::ZKListSubItem *pFileDate = pListItem->findSubItemByID(ID_MAIN_FileDateSubItem);
//	ZKListView::ZKListSubItem *pFileThumbnail = pListItem->findSubItemByID(ID_MAIN_FileThumbnailSubItem);

	OM::FileSystem::FileSystemItem* item = OM::FileSystem::GetItem(index);
	if (item == nullptr)
		return;
//	dbg("Files: settings list item %d name to %s", index, item->GetName().c_str());
	pListItem->setText(item->GetName());
	switch (item->GetType())
	{
	case OM::FileSystem::FileSystemItemType::file:
		pListItem->setSelected(false);
		pFileType->setText("File");
		break;
	case OM::FileSystem::FileSystemItemType::folder:
		pListItem->setSelected(true);
		pFileType->setText("Folder");
		break;
	}
	pFileSize->setTextf("Size: %d", item->GetSize());
	pFileDate->setTextf("Date: %s", item->GetDate().c_str());
}

static void onListItemClick_FileListView(ZKListView *pListView, int index, int id) {
	OM::FileSystem::FileSystemItem* item = OM::FileSystem::GetItem(index);
	switch (item->GetType())
	{
	case OM::FileSystem::FileSystemItemType::file:
		OM::FileSystem::RunFile(item->GetPath());
		break;
	case OM::FileSystem::FileSystemItemType::folder:
		OM::FileSystem::RequestFiles(item->GetPath());
		break;
	}
}
static bool onButtonClick_PrintBabystepDecBtn(ZKButton *pButton) {
    LOGD(" ButtonClick PrintBabystepDecBtn !!!\n");
    return false;
}

static bool onButtonClick_PrintBabystepIncBtn(ZKButton *pButton) {
    LOGD(" ButtonClick PrintBabystepIncBtn !!!\n");
    return false;
}

static int getListItemCount_PrintFanList(const ZKListView *pListView) {
    //LOGD("getListItemCount_PrintFanList !\n");
    return OM::GetFanCount();
}

static void obtainListItemData_PrintFanList(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    OM::Fan* fan = OM::GetFanBySlot(index);
    if (fan == nullptr)
    	return;
    char buf[50];
    snprintf(buf, 50, "Fan %d: %d %%", fan->index, 100*fan->requestedValue);
	pListItem->setText(buf);
}

static void onListItemClick_PrintFanList(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ PrintFanList  !!!\n");
}

static bool onButtonClick_PrintPauseBtn(ZKButton *pButton) {
    LOGD(" ButtonClick PrintPauseBtn !!!\n");
    return false;
}

static bool onButtonClick_PrintCancelBtn(ZKButton *pButton) {
    LOGD(" ButtonClick PrintCancelBtn !!!\n");
    return false;
}

static bool onButtonClick_PrintResumeBtn(ZKButton *pButton) {
    LOGD(" ButtonClick PrintResumeBtn !!!\n");
    return false;
}

static int getListItemCount_PrintPositionList(const ZKListView *pListView) {
    //LOGD("getListItemCount_PrintPositionList !\n");
    return 5;
}

static void obtainListItemData_PrintPositionList(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ PrintPositionList  !!!\n");
}

static void onListItemClick_PrintPositionList(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ PrintPositionList  !!!\n");
}

static int getListItemCount_PrintExtruderPositionList(const ZKListView *pListView) {
    //LOGD("getListItemCount_PrintExtruderPositionList !\n");
    return 3;
}

static void obtainListItemData_PrintExtruderPositionList(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ PrintExtruderPositionList  !!!\n");
}

static void onListItemClick_PrintExtruderPositionList(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ PrintExtruderPositionList  !!!\n");
}

static void onProgressChanged_PrintSpeedMultiplierBar(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged PrintSpeedMultiplierBar %d !!!\n", progress);
}
static int getListItemCount_PrintTemperatureList(const ZKListView *pListView) {
	size_t count = UI::ToolsList::Get("home")->GetTotalHeaterCount(false);
	return count;
}

static void obtainListItemData_PrintTemperatureList(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
	ZKListView::ZKListSubItem *ptoolName = pListItem->findSubItemByID(ID_MAIN_PrintTemperatureListNameSubItem);
	ZKListView::ZKListSubItem *pcurrentTemperature = pListItem->findSubItemByID(ID_MAIN_PrintTemperatureListCurrentSubItem);
	ZKListView::ZKListSubItem *pactiveTemperature = pListItem->findSubItemByID(ID_MAIN_PrintTemperatureListActiveSubItem);
	ZKListView::ZKListSubItem *pstandbyTemperature = pListItem->findSubItemByID(ID_MAIN_PrintTemperatureListStandbySubItem);
	ZKListView::ZKListSubItem *pstatus = pListItem->findSubItemByID(ID_MAIN_PrintTemperatureListStatusSubItem);

	UI::ToolsList::Get("print")->ObtainListItemData(pListItem, index, ptoolName, pcurrentTemperature, pactiveTemperature, pstandbyTemperature, pstatus);
}

static void onListItemClick_PrintTemperatureList(ZKListView *pListView, int index, int id) {
	UI::ToolsList::Get("print")->OnListItemClick(
			index, id,
			NULL, NULL,
			ID_MAIN_PrintTemperatureListActiveSubItem, ID_MAIN_PrintTemperatureListStandbySubItem);
}

static void onProgressChanged_Slider(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged Slider %d !!!\n", progress);
}

static bool onButtonClick_SliderCloseBtn(ZKButton *pButton) {
    LOGD(" ButtonClick SliderCloseBtn !!!\n");
    return false;
}
