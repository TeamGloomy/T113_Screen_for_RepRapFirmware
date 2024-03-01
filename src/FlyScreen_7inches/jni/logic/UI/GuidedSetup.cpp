/*
 * GuidedSetup.cpp
 *
 *  Created on: 28 Feb 2024
 *      Author: andy
 */

#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_DBG
#include "Debug.h"

#include "GuidedSetup.h"
#include "activity/mainActivity.h"
#include "storage/StoragePreferences.h"

namespace UI::GuidedSetup
{
	static ZKWindow* s_window = nullptr;
	static ZKTextView* s_pageNum = nullptr;
	static ZKButton* s_nextBtn = nullptr;
	static Guide* s_currentGuide = nullptr;
	static std::map<const char*, Guide*> guides;

	Page::Page(const char* guideId,
			   const char* imagePath,
			   function<void(void)> nextCb,
			   function<void(void)> previousCb,
			   ZKWindow* window)
		: imagePath(imagePath), nextCb(nextCb), previousCb(previousCb), window(window)
	{
		if (guides.find(guideId) == guides.end())
		{
			// TODO we could create the guide here, need to decide if we want to do that
			error("Guide with id %s does not exist", guideId);
			return;
		}
		info("Adding page to guide %s", guideId);
		guides[guideId]->AddPage(*this);
	}

	Guide::Guide(const char* id, bool closeAnyTime) : m_index(0), m_currentPage(nullptr)
	{
		if (guides.find(id) != guides.end())
		{
			error("Guide with id %s already exists", id);
			return;
		}
		info("Creating guide %s", id);
		guides[id] = this;
	}

	Guide::~Guide()
	{
		Close();
	}

	bool Guide::Show(size_t index)
	{

		if (index >= GetPageCount())
		{
			error("invalid page index %d, only %d pages available", index, GetPageCount());
			return false;
		}

		if (s_window == nullptr)
		{
			error("Guided setup window not initialised");
			return false;
		}
		s_window->showWnd();

		SetWindowVisible(false);

		m_index = index;
		m_currentPage = &m_pages.at(m_index);
		s_pageNum->setTextTrf("page_num", m_index + 1, GetPageCount());
		if (m_index + 1 >= GetPageCount())
		{
			s_nextBtn->setTextTr("finish");
		}
		else
		{
			s_nextBtn->setTextTr("next");
		}

		SetBackground();
		SetWindowVisible(true);
		return true;
	}

	void Guide::Close()
	{
		s_window->hideWnd();
		SetWindowVisible(false);
		m_currentPage = nullptr;
	}

	void Guide::NextPage()
	{
		RunNextCallback();
		if (m_index + 1 >= GetPageCount())
		{
			Close();
			return;
		}
		Show(m_index + 1);
	}

	void Guide::PreviousPage()
	{
		if (m_index <= 0)
		{
			return;
		}
		RunPreviousCallback();
		Show(m_index - 1);
	}

	bool Guide::SetWindowVisible(bool show)
	{
		if (m_currentPage != nullptr && m_currentPage->window != nullptr)
		{
			m_currentPage->window->setVisible(show);
			return true;
		}
		return false;
	}

	bool Guide::SetBackground()
	{
		if (m_currentPage != nullptr && m_currentPage->imagePath != nullptr)
		{
			info("Setting background pic \"%s\"", m_currentPage->imagePath);
			s_window->setBackgroundPic(m_currentPage->imagePath);
			dbg("Set background pic");
			return true;
		}
		return false;
	}

	bool Guide::RunNextCallback()
	{
		if (m_currentPage != nullptr)
		{
			m_currentPage->nextCb();
			return true;
		}
		return false;
	}

	bool Guide::RunPreviousCallback()
	{
		if (m_currentPage != nullptr)
		{
			m_currentPage->previousCb();
			return true;
		}
		return false;
	}

	void Init(ZKWindow* window)
	{
		s_window = window;
		s_pageNum = (ZKTextView*)s_window->findControlByID(ID_MAIN_GuidePageNum);
		s_nextBtn = (ZKButton*)s_window->findControlByID(ID_MAIN_NextPageBtn);
		if (StoragePreferences::getBool("show_setup_on_startup", true))
		{
			Show("setup");
		}
	}

	void AddPage(const char* guideId, Page& page)
	{
		if (guides.find(guideId) == guides.end())
		{
			error("Guide with id %s does not exist", guideId);
			return;
		}
		guides[guideId]->AddPage(page);
	}

	Guide* GetGuide(const char* id)
	{
		if (guides.find(id) == guides.end())
		{
			error("Guide with id %s does not exist", id);
			return nullptr;
		}
		dbg("Got guide %s", id);
		return guides[id];
	}

	Guide* GetCurrentGuide()
	{
		return s_currentGuide;
	}

	void Show(const char* guideId, size_t index)
	{
		dbg("Show guide %s", guideId);
		Guide* guide = GetGuide(guideId);
		Show(guide, index);
	}

	void Show(Guide* guide, size_t index)
	{
		if (guide == nullptr)
		{
			warn("Guide is null");
			return;
		}

		Close();
		guide->Show(index);
		s_currentGuide = guide;
	}

	void Close()
	{
		if (s_currentGuide == nullptr)
		{
			return;
		}
		s_currentGuide->Close();
	}

} // namespace UI::GuidedSetup
