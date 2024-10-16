/*
 * ObjectModel.cpp
 *
 *  Created on: 7 Sep 2020
 *      Author: manuel
 */
#include "Debug.h"

#include "UI/UserInterface.h"

#include "Alert.h"
#include "Axis.h"
#include "BedOrChamber.h"
#include "Configuration.h"
#include "DebugCommands.h"
#include "Files.h"
#include "Heightmap.h"
#include "Job.h"
#include "ListHelpers.h"
#include "PrinterStatus.h"
#include "Spindle.h"
#include "Tool.h"
#include "Utils.h"

namespace OM
{
	void RemoveAll()
	{
		g_currentAlert.Reset();
		g_lastAlertSeq = 0;
		Move::RemoveAxis(0, true);
		Move::RemoveExtruderAxis(0, true);
		RemoveBed(0, true);
		RemoveChamber(0, true);
		RemoveFan(0, true);
		Heat::RemoveHeater(0, true);
		RemoveAnalogSensor(0, true);
		RemoveEndstop(0, true);
		RemoveSpindle(0, true);
		RemoveTool(0, true);
		ClearCurrentHeightmap();
		UI::ToolsList::RefreshAllToolLists(true);
	}

	static Debug::DebugCommand s_logOM("dbg_log_OM", []() {
		UI::CONSOLE.AddResponse("ObjectModel:");
		UI::CONSOLE.AddResponse("  Axes:");
		for (size_t i = 0; i < MAX_TOTAL_AXES; ++i)
		{
			Move::Axis* axis = Move::GetAxis(i);
			if (axis != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: (%s), babystep(%.3f), user(%.2f), machine(%.2f), homed(%d)",
								  axis->index,
								  axis->letter,
								  axis->babystep,
								  axis->userPosition,
								  axis->machinePosition,
								  axis->homed)
						.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Extruders:");
		for (size_t i = 0; i < MAX_TOTAL_AXES; ++i)
		{
			Move::ExtruderAxis* extruder = Move::GetExtruderAxis(i);
			if (extruder != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: pos(%.2f), f(%.2f), spmm(%.1f), pa(%.2f), fil(%.2f, %s)",
								  extruder->index,
								  extruder->position,
								  extruder->factor,
								  extruder->stepsPerMm,
								  extruder->pressureAdvance,
								  extruder->filamentDiameter,
								  extruder->filamentName.c_str())
						.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Beds:");
		for (size_t i = 0; i < MAX_SLOTS; ++i)
		{
			BedOrChamber* bed = GetBed(i);
			if (bed != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: heater(%d), slot(%d)", bed->index, bed->heater, bed->slot).c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Chambers:");
		for (size_t i = 0; i < MAX_SLOTS; ++i)
		{
			BedOrChamber* chamber = GetChamber(i);
			if (chamber != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: heater(%d), slot(%d)", chamber->index, chamber->heater, chamber->slot)
						.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Fans:");
		for (size_t i = 0; i < MAX_FANS; ++i)
		{
			Fan* fan = GetFan(i);
			if (fan != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("    [%u]: actual(%.2f), requested(%.2f), rpm(%d)",
													  fan->index,
													  fan->actualValue,
													  fan->requestedValue,
													  fan->rpm)
											.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Files:");
		for (size_t i = 0; i < FileSystem::GetItemCount(); ++i)
		{
			FileSystem::FileSystemItem* item = FileSystem::GetItem(i);
			if (item != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("    [%u]: path(%s)", i, item->GetPath().c_str()).c_str());
				UI::CONSOLE.AddResponse(
					utils::format("          date(%s), size(%u)", item->GetDate().c_str(), item->GetSize()).c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Heaters:");
		for (size_t i = 0; i < MAX_HEATERS; ++i)
		{
			Heat::Heater* heater = Heat::GetHeater(i);
			if (heater != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("    [%u]: status(%d), active(%d), standby(%d), current(%.2f),",
													  heater->index,
													  heater->status,
													  heater->activeTemp,
													  heater->standbyTemp,
													  heater->current)
											.c_str());
				UI::CONSOLE.AddResponse(utils::format("             min(%.1f), max(%.1f), pwm(%.2f), sensor(%d)",
													  heater->min,
													  heater->max,
													  heater->avgPwm,
													  heater->sensor == nullptr ? -1 : heater->sensor->index)
											.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Job:");
		UI::CONSOLE.AddResponse(utils::format("    name: %s", GetJobName().c_str()).c_str());
		UI::CONSOLE.AddResponse(utils::format("    Print time: %u", GetPrintTime()).c_str());
		UI::CONSOLE.AddResponse(utils::format("    Print duration: %u", GetPrintDuration()).c_str());
		UI::CONSOLE.AddResponse(utils::format("    Print remaining (slicer): %u", GetPrintRemaining(OM::RemainingTimeType::slicer)).c_str());
		UI::CONSOLE.AddResponse(utils::format("    Current object: %d", GetCurrentJobObjectIndex()).c_str());
		UI::CONSOLE.AddResponse("    Objects:");
		for (size_t i = 0; i < MAX_TRACKED_OBJECTS; ++i)
		{
			JobObject* jobObject = GetJobObject(i);
			if (jobObject != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("      [%u]: cancelled(%d), name(%s)",
													  jobObject->index,
													  jobObject->cancelled,
													  jobObject->name.c_str())
											.c_str());
				UI::CONSOLE.AddResponse(utils::format("            bounds(%d, %d, %d, %d)",
													  jobObject->bounds.x[0],
													  jobObject->bounds.x[1],
													  jobObject->bounds.y[0],
													  jobObject->bounds.y[1])
											.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Sensors:");
		for (size_t i = 0; i < MAX_SENSORS; ++i)
		{
			AnalogSensor* sensor = GetAnalogSensor(i);
			if (sensor != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("    [%u]: %s, %.3f @ time(%lld)",
													  sensor->index,
													  sensor->name.c_str(),
													  sensor->lastReading,
													  sensor->lastReadingTime)
											.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Endstops:");
		for (size_t i = 0; i < MAX_ENDSTOPS; ++i)
		{
			Endstop* endstop = GetEndstop(i);
			if (endstop != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: triggered(%d)", endstop->index, endstop->triggered).c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Spindles:");
		for (size_t i = 0; i < MAX_SLOTS; ++i)
		{
			Spindle* spindle = GetSpindle(i);
			if (spindle != nullptr)
			{
				UI::CONSOLE.AddResponse(
					utils::format("    [%u]: state(%d), active(%u), current(%u), min(%u), max(%u), canReverse(%d)",
								  spindle->index,
								  spindle->state,
								  spindle->active,
								  spindle->current,
								  spindle->min,
								  spindle->max,
								  spindle->canReverse)
						.c_str());
			}
		}
		UI::CONSOLE.AddResponse("  Tools:");
		for (size_t i = 0; i < MAX_SLOTS; ++i)
		{
			Tool* tool = GetTool(i);
			if (tool != nullptr)
			{
				UI::CONSOLE.AddResponse(utils::format("    [%u]: %s, status(%d), filamentExtruder(%d), spindle(%d)",
													  tool->index,
													  tool->name.c_str(),
													  tool->status,
													  tool->filamentExtruder,
													  tool->spindle == nullptr ? -1 : tool->spindle->index)
											.c_str());
				for (size_t j = 0; j < MAX_HEATERS_PER_TOOL; ++j)
				{
					ToolHeater* th = tool->GetHeater(j);
					if (th != nullptr)
					{
						UI::CONSOLE.AddResponse(
							utils::format("      ToolHeater[%u]: active(%d), standby(%d), heater(%d)",
										  j,
										  th->activeTemp,
										  th->standbyTemp,
										  th->heater == nullptr ? -1 : th->heater->index)
								.c_str());
					}
				}
				string extruders = "      Extruders: [";
				bool first = true;
				for (size_t j = 0; j < MAX_EXTRUDERS_PER_TOOL; ++j)
				{
					Move::ExtruderAxis* ea = tool->GetExtruder(j);
					if (ea != nullptr)
					{
						extruders += utils::format("%s%u", first ? "" : ", ", ea->index);
						first = false;
					}
				}
				extruders += "]";
				UI::CONSOLE.AddResponse(extruders.c_str());

				string fans = "      Fans: [";
				first = true;
				for (size_t j = 0; j < MAX_FANS; ++j)
				{
					Fan* fan = tool->GetFan(j);
					if (fan != nullptr)
					{
						fans += utils::format("%s%u", first ? "" : ", ", fan->index);
						first = false;
					}
				}
				fans += "]";
				UI::CONSOLE.AddResponse(fans.c_str());
			}
		}
		UI::CONSOLE.AddResponse(utils::format("  Printer Name: %s", OM::GetPrinterName().c_str()).c_str());
		UI::CONSOLE.AddResponse(utils::format("  Printer State: %s(%d)", OM::GetStatusText(), OM::GetStatus()).c_str());

		UI::CONSOLE.AddResponse("");
	});
}
