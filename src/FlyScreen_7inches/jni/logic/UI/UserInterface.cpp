/*
 * UserInterface.cpp
 *
 *  Created on: 18 Dec 2023
 *      Author: andy
 */

#define DEBUG (1)
#include "UserInterface.h"
#include "Debug.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tool.h"
#include <algorithm>
#include <map>

template <typename T>
bool InVector(std::vector<T>& vec, T item)
{
	auto it = std::find(vec.begin(), vec.end(), item);
	if (it != vec.end()) { return true; }
	return false;
}

template <typename T>
bool RemoveFromVector(std::vector<T>& vec, T item)
{
	auto it = std::find(vec.begin(), vec.end(), item);
	if (it != vec.end())
	{
		dbg("Window: removing window %d from vector", item->getID());
		vec.erase(it);
		return true;
	}
	return false;
}

template <typename T>
bool AddToVector(std::vector<T>& vec, T item)
{
	RemoveFromVector<T>(vec, item);
	dbg("Window: adding window %d to vector", item->getID());
	vec.push_back(item);
	return true;
}

static std::string sSelectedFile;

namespace UI
{
	Window* Window::GetInstance()
	{
		static Window window;
		return &window;
	}

	void Window::AddHome(ZKWindow* home)
	{
		homeWindows_.push_back(home);
	}

	void Window::ClearHome()
	{
		homeWindows_.clear();
	}

	void Window::OpenWindow(ZKWindow* window)
	{
		CloseOverlay();
		dbg("Opening window %d", window->getID());
		window->showWnd();
		RemoveFromVector<ZKWindow*>(closedWindows_, window);
		if (!InVector<ZKWindow*>(homeWindows_, window)) AddToVector<ZKWindow*>(openedWindows_, window);
	}

	void Window::OpenOverlay(ZKWindow* overlay)
	{
		if (overlayWindow_ != nullptr) { overlayWindow_->hideWnd(); }
		overlay->showWnd();
		overlayWindow_ = overlay;
	}

	bool Window::CloseOverlay()
	{
		if (overlayWindow_ == nullptr)
			return false;

		// Ensure that you can't close the popup window if it's blocking. To do this, you must first Clear the popup
		if (POPUP_WINDOW->IsBlocking())
		{
			return false;
		}
		if (POPUP_WINDOW->IsOpen())
		{
			POPUP_WINDOW->Clear();
		}
		overlayWindow_->hideWnd();
		overlayWindow_ = nullptr;
		return true;
	}

	size_t Window::ReOpenLastWindow(size_t numWindows)
	{
		size_t count = 0;
		for (size_t i = 0; i < numWindows; ++i)
		{
			if (closedWindows_.empty()) return count;
			auto window = closedWindows_.back();
			OpenWindow(window);
		}
		return count;
	}

	void Window::CloseLastWindow()
	{
		for (auto window : homeWindows_)
		{
			if (!window->isWndShow()) continue;
			CloseWindow(window);
		}
		if (openedWindows_.empty()) return;
		CloseWindow(openedWindows_.back());
	}

	void Window::CloseWindow(ZKWindow* window, const bool returnable)
	{
		dbg("Closing window %d", window->getID());
		window->hideWnd();
		RemoveFromVector<ZKWindow*>(openedWindows_, window);
		if (returnable) AddToVector<ZKWindow*>(closedWindows_, window);
	}

	void Window::Back()
	{
		if (CloseOverlay()) { return; }
		if (OM::FileSystem::IsInSubFolder())
		{
			dbg("window: Returning to previous folder");
			OM::FileSystem::RequestFiles(OM::FileSystem::GetParentDirPath());
			return;
		}
		if (!openedWindows_.empty())
		{
			ZKWindow* lastOpened = openedWindows_.back();
			dbg("Hiding window %d", lastOpened->getID());
			CloseWindow(lastOpened, false);
		}
		if (!closedWindows_.empty())
		{
			ZKWindow* lastClosed = closedWindows_.back();
			OpenWindow(lastClosed);
		}
	}

	void Window::Home()
	{
		for (auto window : openedWindows_)
		{
			window->hideWnd();
		}
		for (auto window : homeWindows_)
		{
			window->showWnd();
		}
		CloseOverlay();
		Clear();
	}

	void Window::Clear()
	{
		openedWindows_.clear();
		closedWindows_.clear();
	}

	static std::map<const char*, ToolsList*> sToolsListMap;

	ToolsList::ToolsList(const char* id) : id(id), toolCount_(0), bedCount_(0), chamberCount_(0)
	{
		dbg("Registering tool list to id %s", id);
	}

	ToolsList* ToolsList::Create(const char* id)
	{
		sToolsListMap[id] = new ToolsList(id);
		return sToolsListMap[id];
	}

	ToolsList* ToolsList::Get(const char* id)
	{
		auto it = sToolsListMap.find(id);
		return (it != sToolsListMap.end()) ? it->second : nullptr;
	}

	void ToolsList::Init(ZKListView* toolListView, ZKWindow* numPadWindow, ZKTextView* numPadInput)
	{
		pToolListView_ = toolListView;
		pNumPadWindow_ = numPadWindow;
		pNumPadInput_ = numPadInput;
	}

	void ToolsList::CalculateTotalHeaterCount()
	{
		// dbg("Bed count %d", OM::GetBedCount());
		bedCount_ = OM::GetBedCount();

		// dbg("Chamber count %d", OM::GetChamberCount());
		chamberCount_ = OM::GetChamberCount();

		size_t count = 0;
		OM::IterateToolsWhile([&count](OM::Tool*& tool, size_t) {
			const bool hasHeater = tool->heaters[0] != nullptr;
			const bool hasSpindle = tool->spindle != nullptr;
			// Spindle takes precedence
			if (hasSpindle) { ++count; }
			else if (hasHeater) { count += tool->GetHeaterCount(); }
			else
			{
				// Hides everything by default
				++count;
			}
			return true;
		});

		// dbg("Tool count %d", count);
		toolCount_ = count;
	}

	size_t ToolsList::GetTotalHeaterCount(const bool calculate, const bool addTools, const bool addBeds,
										  const bool addChambers)
	{
		if (calculate) CalculateTotalHeaterCount();

		size_t count = 0;
		if (addTools) { count += toolCount_; }
		if (addBeds) { count += bedCount_; }
		if (addChambers) { count += chamberCount_; }
		return count;
	}

	void ToolsList::ObtainListItemData(ZKListView::ZKListItem* pListItem, int index,
									   ZKListView::ZKListSubItem* pToolName,
									   ZKListView::ZKListSubItem* pCurrentTemperature,
									   ZKListView::ZKListSubItem* pActiveTemperature,
									   ZKListView::ZKListSubItem* pStandbyTemperature,
									   ZKListView::ZKListSubItem* pStatus)
	{
		// Check Tools to see if list index is within tool heaters
		OM::Tool* tool = nullptr;
		int8_t toolHeaterIndex = UI::GetToolHeaterIndex(index, tool);
		if (tool != nullptr)
		{
			OM::ToolHeater* toolHeater;
			toolHeater = tool->GetHeater(toolHeaterIndex);
			if (toolHeater == nullptr)
			{
				dbg("List index %d: Tool %d heaterIndex %d is null", index, tool->index, toolHeaterIndex);
				pToolName->setText(tool->name.c_str());
				return;
			}
			// dbg("List index %d: Updating Tool %d heater %d=%d temperatures %.2f:%d:%d", index, tool->index,
			// 	toolHeaterIndex, toolHeater->heater->index, toolHeater->heater->current, toolHeater->activeTemp,
			// 	toolHeater->standbyTemp);
			if (tool->GetHeaterCount() > 1) { pToolName->setTextf("%s (%d)", tool->name.c_str(), toolHeaterIndex); }
			else { pToolName->setText(tool->name.c_str()); }
			pActiveTemperature->setText((int)toolHeater->activeTemp);
			pStandbyTemperature->setText((int)toolHeater->standbyTemp);
			pCurrentTemperature->setText(toolHeater->heater->current);
			pStatus->setTextTr(toolHeater->heater->GetHeaterStatusStr());
			(tool->status == OM::ToolStatus::active) ? pListItem->setSelected(true) : pListItem->setSelected(false);

			return;
		}

		int8_t bedOrChamberIndex = index - toolHeaterIndex;
		OM::BedOrChamber* bedOrChamber = OM::GetBedBySlot(bedOrChamberIndex);
		OM::Heat::Heater* heater;
		if (bedOrChamber != nullptr)
		{
			heater = OM::Heat::GetHeater(bedOrChamber->heater);
			if (heater == nullptr)
			{
				dbg("List index %d: Bed %d heater %d is null", index, bedOrChamber->index, bedOrChamber->heater);
				return;
			}
			// dbg("List index %d: Updating Bed %d heater %d=%d temperatures %.2f:%d:%d", index, bedOrChamber->index,
			// 	bedOrChamber->heater, heater->index, heater->current, heater->activeTemp, heater->standbyTemp);
			if (OM::GetBedCount() > 1)
			{
				pToolName->setTextf("%s %d", LANGUAGEMANAGER->getValue("bed").c_str(), bedOrChamber->index);
			}
			else
			{
				pToolName->setTextTr("bed");
			}
			pActiveTemperature->setText((int)heater->activeTemp);
			pStandbyTemperature->setText((int)heater->standbyTemp);
			pCurrentTemperature->setText(heater->current);
			pStatus->setText(heater->GetHeaterStatusStr());
			pListItem->setSelected(false);
			return;
		}

		bedOrChamberIndex -= OM::GetBedCount();
		bedOrChamber = OM::GetChamberBySlot(bedOrChamberIndex);
		if (bedOrChamber != nullptr)
		{
			heater = OM::Heat::GetHeater(bedOrChamber->heater);
			if (heater == nullptr)
			{
				dbg("List index %d: Bed %d heater %d is null", index, bedOrChamber->index, bedOrChamber->heater);
				return;
			}
			// dbg("List index %d: Updating Chamber %d heater %d=%d temperatures %.2f:%d:%d", index,
			// bedOrChamber->index, 	bedOrChamber->heater, heater->index, heater->current, heater->activeTemp,
			// heater->standbyTemp);
			if (OM::GetChamberCount() > 1)
			{
				pToolName->setTextf("%s %d", LANGUAGEMANAGER->getValue("chamber").c_str(), bedOrChamber->index);
			}
			else
			{
				pToolName->setTextTr("chamber");
			}
			pActiveTemperature->setText((int)heater->activeTemp);
			pStandbyTemperature->setText((int)heater->standbyTemp);
			pCurrentTemperature->setText(heater->current);
			pStatus->setText(heater->GetHeaterStatusStr());
			pListItem->setSelected(false);
			return;
		}
		dbg("Unknown index");
	}

	void ToolsList::OnListItemClick(int index, int id, const int nameId, int statusId, int activeId, int standbyId)
	{
		dbg("index=%d, id=%d", index, id);
		if (id == NULL) return;
		UI::NumPadData numPadData;

		CalculateTotalHeaterCount();
		if ((size_t)index < GetTotalHeaterCount(false, true, false, false))
		{
			OM::Tool* tool = nullptr;
			int8_t toolHeaterIndex = UI::GetToolHeaterIndex(index, tool);
			if (tool == nullptr) return;
			dbg("Tool index=%d", tool->index);
			numPadData.heaterType = UI::HeaterType::tool;
			numPadData.toolIndex = tool->index;
			numPadData.toolHeaterIndex = toolHeaterIndex;
			if (id == nameId)
			{
				tool->ToggleState();
				return;
			}
			if (id == statusId)
			{
				tool->ToggleHeaterState(toolHeaterIndex);
				return;
			}
		}
		else if ((size_t)index < GetTotalHeaterCount(false, true, true, false))
		{
			OM::Bed* bed = OM::GetBedBySlot(index - GetTotalHeaterCount(false, true, false, false));
			if (bed == nullptr) return;
			dbg("Bed index=%d", bed->index);
			numPadData.heaterType = UI::HeaterType::bed;
			numPadData.bedOrChamberIndex = bed->index;
			if (id == nameId || id == statusId)
			{
				bed->ToggleBedState();
				return;
			}
		}

		else if ((size_t)index < GetTotalHeaterCount(false, true, true, true))
		{
			OM::Chamber* chamber = OM::GetChamberBySlot(index - GetTotalHeaterCount(false, true, true, false));
			if (chamber == nullptr) return;
			dbg("Chamber index=%d", chamber->index);
			numPadData.heaterType = UI::HeaterType::chamber;
			numPadData.bedOrChamberIndex = chamber->index;
			if (id == nameId || id == statusId)
			{
				chamber->ToggleChamberState();
				return;
			}
		}

		if (id == activeId)
		{
			numPadData.active = true;
			OpenNumPad(numPadData);
			return;
		}
		if (id == standbyId)
		{
			numPadData.active = false;
			OpenNumPad(numPadData);
			return;
		}
	}

	void ToolsList::RefreshToolList(const bool lengthChanged)
	{
		if (lengthChanged) { CalculateTotalHeaterCount(); }
		pToolListView_->refreshListView();
	}

	void ToolsList::RefreshAllToolLists(const bool lengthChanged)
	{
		for (auto list : sToolsListMap)
		{
			list.second->RefreshToolList(lengthChanged);
		}
	}

	void ToolsList::OpenNumPad(const NumPadData data)
	{
		numPadData_.numPadStr = "";
		pNumPadInput_->setText("");
		numPadData_.heaterType = data.heaterType;
		numPadData_.active = data.active;

		switch (data.heaterType)
		{
		case HeaterType::tool:
			numPadData_.toolIndex = data.toolIndex;
			numPadData_.toolHeaterIndex = data.toolHeaterIndex;
			break;
		case HeaterType::bed:
		case HeaterType::chamber:
			numPadData_.bedOrChamberIndex = data.bedOrChamberIndex;
			break;
		}
		WINDOW->OpenOverlay(pNumPadWindow_);
	}

	void ToolsList::CloseNumPad()
	{
		WINDOW->CloseWindow(pNumPadWindow_, false);
	}

	void ToolsList::NumPadAddOneChar(char ch)
	{
		numPadData_.numPadStr += ch;
		pNumPadInput_->setText(numPadData_.numPadStr);
	}

	void ToolsList::NumPadDelOneChar()
	{
		if (!numPadData_.numPadStr.empty())
		{
			numPadData_.numPadStr.erase(numPadData_.numPadStr.length() - 1, 1);
			pNumPadInput_->setText(numPadData_.numPadStr);
		}
	}

	bool ToolsList::SendTempTarget()
	{
		if (numPadData_.numPadStr.empty()) return false;

		int32_t target = atoi(numPadData_.numPadStr.c_str());
		OM::Tool* tool = nullptr;
		OM::BedOrChamber* bedOrChamber = nullptr;
		dbg("onListItemClick heaterType=%d", numPadData_.heaterType);
		switch (numPadData_.heaterType)
		{
		case HeaterType::tool:
			tool = OM::GetTool(numPadData_.toolIndex);
			if (tool == nullptr) return false;

			tool->SetHeaterTemps(numPadData_.toolHeaterIndex, target, numPadData_.active);
			break;
		case HeaterType::bed:
			bedOrChamber = OM::GetBed(numPadData_.bedOrChamberIndex);
			if (bedOrChamber == nullptr) return false;

			bedOrChamber->SetBedTemp(target, numPadData_.active);
			break;
		case HeaterType::chamber:
			bedOrChamber = OM::GetChamber(numPadData_.bedOrChamberIndex);
			if (bedOrChamber == nullptr) return false;

			bedOrChamber->SetChamberTemp(target, numPadData_.active);
			break;
		}

		return true;
	}

	int8_t GetToolHeaterIndex(const size_t listIndex, OM::Tool*& tool)
	{
		uint8_t count = 0;
		int8_t toolHeaterIndex = -1;
		OM::IterateToolsWhile([&](OM::Tool*& toolIter, size_t) {
			if (listIndex < count + toolIter->GetHeaterCount())
			{
				tool = toolIter;
				toolHeaterIndex = listIndex - count;
				return false;
			}
			count += toolIter->GetHeaterCount();
			return true;
		});
		if (toolHeaterIndex > -1)
		{
			//			dbg("List index %d is Tool %d toolHeaterIndex %d=%d", listIndex, tool->index, toolHeaterIndex,
			// tool->GetHeater(toolHeaterIndex)->index);
			return toolHeaterIndex;
		}
		//		dbg("List index %d is not in tool heaters (total tool heaters %d)", listIndex, count);
		tool = nullptr;
		return count; // list index is greater than all heaters for tools
	}

	void Console::Init(ZKListView* console, ZKEditText* input)
	{
		pConsole_ = console;
		pInput_ = input;
	}

	void Console::AddCommand(const std::string& command)
	{
		dbg("AddingCommand %s", command.c_str());
		String<MaxResponseLineLength> line;
		AddLineBreak();
		line.catf("> %s:", command.c_str());
		AddMessage(line.GetRef());
		AddLineBreak();
		Refresh();
	}

	void Console::AddResponse(const StringRef& ref)
	{
		AddMessage(ref);
	}

	void Console::AddLineBreak()
	{
		AddMessage("");
	}

	void Console::AddMessage(const char* str)
	{
		String<MaxResponseLineLength> line;
		line.copy(str);
		buffer_.Push(line);
		dbg("resp: adding line to Console buffer_[%d] = %s", buffer_.GetHead(), line.c_str());
	}

	void Console::AddMessage(const StringRef& ref)
	{
		AddMessage(ref.c_str());
	}

	void Console::Refresh()
	{
		pConsole_->refreshListView();
		if (!buffer_.Full()) pConsole_->setSelection(buffer_.GetHead() - pConsole_->getRows());
	}

	void Console::Clear()
	{
		String<MaxResponseLineLength> line;
		line.copy("");
		for (size_t i = 0; i < MaxResponseLines; i++)
		{
			buffer_.Push(line);
		}
		buffer_.Reset();
		Refresh();
	}

	void SetSelectedFile(const std::string& filePath)
	{
		sSelectedFile = filePath;
	}

	std::string& GetSelectedFile()
	{
		return sSelectedFile;
	}

	void RunSelectedFile()
	{
		return OM::FileSystem::RunFile(sSelectedFile);
	}
} // namespace UI
