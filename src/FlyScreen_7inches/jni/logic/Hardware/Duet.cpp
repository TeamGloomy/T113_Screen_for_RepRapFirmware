/*
 * Duet.cpp
 *
 *  Created on: 26 Jan 2024
 *      Author: Andy Everitt
 */
#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_VERBOSE
#include "timer.h"

#include "Communication.h"
#include "Debug.h"
#include "Duet.h"
#include "Hardware/SerialIo.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/UserInterface.h"
#include "manager/ConfigManager.h"
#include "storage/StoragePreferences.h"
#include "uart/UartContext.h"
#include "utils.h"
#include "utils/TimeHelper.h"
#include "json/json.h"
#include <map>
#include <string>

namespace Comm
{
	Duet duet;

	Duet::Duet()
		: m_communicationType(CommunicationType::none), m_ipAddress(""), m_hostname(""), m_password(""),
		  m_sessionTimeout(0), m_lastRequestTime(0), m_sessionKey(noSessionKey),
		  m_pollInterval(defaultPrinterPollInterval)
	{
	}

	void Duet::Init()
	{
		SetPollInterval((uint32_t)StoragePreferences::getInt("poll_interval", defaultPrinterPollInterval));
		SetBaudRate((unsigned int)StoragePreferences::getInt("baud_rate", CONFIGMANAGER->getUartBaudRate()));
		SetIPAddress(StoragePreferences::getString("ip_address", "192.168.0."));
		SetHostname(StoragePreferences::getString("hostname", ""));
		SetPassword(StoragePreferences::getString("password", ""));
		SetCommunicationType((CommunicationType)StoragePreferences::getInt("communication_type", 0));
	}

	void Duet::Reset()
	{
		verbose("");
		m_sessionKey = noSessionKey;
		m_sessionTimeout = 0;
		m_lastRequestTime = 0;
	}

	void Duet::SetCommunicationType(CommunicationType type)
	{
		if (type == m_communicationType)
			return;
		info("Setting communication type to %d", (int)type);
		StoragePreferences::putInt("communication_type", (int)type);
		Disconnect();

		m_communicationType = type;
		if (type == CommunicationType::uart)
		{
			info("Opening UART %s at %u", CONFIGMANAGER->getUartName().c_str(), m_baudRate.rate);
			UARTCONTEXT->openUart(CONFIGMANAGER->getUartName().c_str(), m_baudRate.internal);
		}

		if (type == CommunicationType::network)
		{
			Connect();
		}
	}

	void Duet::SetPollInterval(uint32_t interval)
	{
		if (interval < minPrinterPollInterval)
		{
			info("Poll interval too low, setting to %d", minPrinterPollInterval);
			interval = minPrinterPollInterval;
		}
		info("Setting poll interval to %d", interval);
		StoragePreferences::putInt("poll_interval", (int)interval);
		resetUserTimer(TIMER_UPDATE_DATA, (int)interval);
		m_pollInterval = interval;
	}

	void Duet::ScalePollInterval(float scale)
	{
		SetPollInterval(static_cast<uint32_t>(m_pollInterval * scale));
	}

	/*
	Tries to make a get request to Duet, if it returns 401 or 403 then it will run `rr_connect` and send the request
	again
	*/
	bool Duet::Get(const char* subUrl, RestClient::Response& r, QueryParameters_t& queryParameters)
	{
		if (m_sessionKey == noSessionKey || (TimeHelper::getCurrentTime() - m_lastRequestTime > m_sessionTimeout))
		{
			if (!Connect())
			{
				warn("Failed to connect to Duet, cannot send get request %s", subUrl);
				r.code = -1;
				return false;
			}
		}
		if (!Comm::Get(m_ipAddress, subUrl, r, queryParameters, m_sessionKey))
		{
			if (r.code == 401 || r.code == 403)
			{
				error("HTTP error %d: Likely invalid sessionKey %u. Running rr_connect", r.code, m_sessionKey);
				Connect();
				return Comm::Get(m_ipAddress, subUrl, r, queryParameters, m_sessionKey);
			}
			return false;
		}
		m_lastRequestTime = TimeHelper::getCurrentTime();
		return true;
	}

	/*
	Tries to make a post request to Duet, if it returns 401 or 403 then it will run `rr_connect` and send the request
	again
	*/
	bool Duet::Post(const char* subUrl,
					RestClient::Response& r,
					QueryParameters_t& queryParameters,
					const std::string& data)
	{
		if (m_sessionKey == noSessionKey || (TimeHelper::getCurrentTime() - m_lastRequestTime > m_sessionTimeout))
		{
			if (!Connect())
			{
				warn("Failed to connect to Duet, cannot send post request %s", subUrl);
				return false;
			}
		}
		if (!Comm::Post(m_ipAddress, subUrl, r, queryParameters, data, m_sessionKey))
		{
			if (r.code == 401 || r.code == 403)
			{
				error("HTTP error %d: Likely invalid sessionKey %d. Running rr_connect", r.code, m_sessionKey);
				Connect();
				return Comm::Post(m_ipAddress, subUrl, r, queryParameters, data, m_sessionKey);
			}
			return false;
		}
		m_lastRequestTime = TimeHelper::getCurrentTime();
		return true;
	}

	void Duet::SendGcode(const char* gcode)
	{
		switch (m_communicationType)
		{
		case CommunicationType::uart:
			SerialIo::Sendf("%s\n", gcode);
			break;
		case CommunicationType::network: {
			RestClient::Response r;
			QueryParameters_t query;
			query["gcode"] = gcode;
			if (!Get("/rr_gcode", r, query))
			{
				UI::CONSOLE->AddResponse(
					utils::format("HTTP error %d: Failed to send gcode: %s", r.code, gcode).c_str());
				break;
			}
			RequestReply(r);
			ProcessReply(r);
			break;
		}
		case CommunicationType::usb:
			// TODO
			break;
        default:
            break;
		}
	}

	bool Duet::UploadFile(const char* filename, std::string& contents)
	{
		info("Uploading file %s: %d bytes", filename, contents.size());
		UI::POPUP_WINDOW->Open();
		UI::POPUP_WINDOW->SetTextf(LANGUAGEMANAGER->getValue("uploading_file").c_str(), filename);
		switch (m_communicationType)
		{
		case CommunicationType::uart: {
			SendGcodef("M28 \"%s\"", filename);
			size_t prevPosition = 0;
			size_t position = contents.find("\n"); // Find the first occurrence of \n
			std::string line;
			while (position != std::string::npos)
			{
				line = contents.substr(prevPosition, position - prevPosition);
				prevPosition = position + 1;
				position = contents.find("\n", position + 1); // Find the next occurrence, if any
				SendGcode(line.c_str());
			}
			SendGcode("M29");
			break;
		}
		case CommunicationType::network: {
			RestClient::Response r;
			QueryParameters_t query;
			query["name"] = filename;
			if (!Post("/rr_upload", r, query, contents))
			{
				UI::CONSOLE->AddResponse(
					utils::format("HTTP error %d %s: Failed to upload file: %s", r.code, r.body, filename).c_str());
				return false;
			}
			break;
		}
		default:
			break;
		}
		return true;
	}

	void Duet::RequestModel(const char* flags)
	{
		switch (m_communicationType)
		{
		case CommunicationType::uart:
			SendGcodef("M409 F\"%s\"\n", flags);
			break;
		case CommunicationType::network: {
			RestClient::Response r;
			QueryParameters_t query;
			query["flags"] = flags;
			if (!Get("/rr_model", r, query))
			{
				UI::CONSOLE->AddResponse(
					utils::format("HTTP error %d: Failed to get model update for flags: %s", r.code, flags).c_str());
				break;
			}
			SerialIo::CheckInput((const unsigned char*)r.body.c_str(), r.body.length() + 1);
			break;
		}
		default:
			break;
		}
	}

	void Duet::RequestModel(const char* key, const char* flags)
	{
		switch (m_communicationType)
		{
		case CommunicationType::uart:
			SendGcodef("M409 K\"%s\" F\"%s\"\n", key, flags);
			break;
		case CommunicationType::network: {
			RestClient::Response r;
			QueryParameters_t query;
			query["key"] = key;
			query["flags"] = flags;
			if (!Get("/rr_model", r, query))
			{
				UI::CONSOLE->AddResponse(
					utils::format(
						"HTTP error %d: Failed to get model update for key: %s, flags: %s", r.code, key, flags)
						.c_str());
				break;
			}
			SerialIo::CheckInput((const unsigned char*)r.body.c_str(), r.body.length() + 1);
			break;
		}
		default:
			break;
		}
		return;
	}

	void Duet::RequestFileList(const char* dir, const size_t first)
	{
		switch (m_communicationType)
		{
		case CommunicationType::uart:
			SendGcodef("M20 S3 P\"%s\" R%d\n", dir, first);
			break;
		case CommunicationType::network: {
			RestClient::Response r;
			QueryParameters_t query;
			query["dir"] = dir;
			query["first"] = utils::format("%d", first);
			if (!Get("/rr_filelist", r, query))
				break;
			SerialIo::CheckInput((const unsigned char*)r.body.c_str(), r.body.length() + 1);
			break;
		}
		default:
			break;
		}
		return;
	}

	void Duet::ProcessReply(const RestClient::Response& reply)
	{
		if (m_communicationType != CommunicationType::network)
			return;

		if (reply.body.empty())
		{
			warn("Empty reply received");
			return;
		}
		if (reply.body[0] != '{')
		{
			dbg("Reply not json: assuming it is a gcode response");
			size_t prevPosition = 0;
			size_t position = reply.body.find("\n"); // Find the first occurrence of \n

			while (position != std::string::npos)
			{
				StringRef ref((char*)"resp", 5);
				std::string line = reply.body.substr(prevPosition, position - prevPosition);
				dbg("line: %s", line.c_str());
				prevPosition = position + 1;
				position = reply.body.find("\n", position + 1); // Find the next occurrence, if any
				if (line.empty())
					continue;
				Comm::ProcessReceivedValue(ref, line.c_str(), {});
			}
			return;
		}
		SerialIo::CheckInput((const unsigned char*)reply.body.c_str(), reply.body.length() + 1);
		return;
	}

	void Duet::RequestReply(RestClient::Response& r)
	{
		QueryParameters_t query;
		Get("/rr_reply", r, query);
	}

	const bool Duet::Connect(bool useSessionKey)
	{
		if (m_communicationType != CommunicationType::network)
			return 0;

		Disconnect();
		Reset();

		RestClient::Response r;
		QueryParameters_t query;
		query["password"] = m_password;
		if (useSessionKey)
			query["sessionKey"] = "yes";

		if (!Comm::Get(m_ipAddress, "/rr_connect", r, query))
		{
			error("rr_connect failed, returned response %d", r.code);
			return false;
		}

		verbose("parsing rr_connect response");
		Json::Reader reader;
		Json::Value body;

		if (!reader.parse(r.body, body))
		{
			error("Failed to parse JSON response from rr_connect");
			return false;
		}

		if (body.isMember("err") && body["err"].asInt() != 0)
		{
			error("rr_connect failed, returned error %d", body["err"].asInt());
			return false;
		}

		if (body.isMember("sessionTimeout"))
		{
			m_sessionTimeout = body["sessionTimeout"].asInt();
			m_lastRequestTime = TimeHelper::getCurrentTime();
			info("Duet session timeout set to %d", m_sessionTimeout);
		}

		if (body.isMember("sessionKey"))
		{
			m_sessionKey = body["sessionKey"].asUInt();
			info("Duet session key = %u", m_sessionKey);
		}
		info("rr_connect succeeded");
		return true;
	}

	const Duet::error_code Duet::Disconnect()
	{
		SetStatus(OM::PrinterStatus::connecting);
		switch (m_communicationType)
		{
		case CommunicationType::uart:
			UARTCONTEXT->closeUart();
			Thread::sleep(50);
			break;
		case CommunicationType::network: {
			if (m_sessionKey == noSessionKey)
			{
				Reset();
				return 0;
			}
			RestClient::Response r;
			QueryParameters_t query;
			if (!Comm::Get(m_ipAddress, "/rr_disconnect", r, query, m_sessionKey))
			{
				error("rr_disconnect failed, returned response %d", r.code);
				return r.code;
			}
			Reset();
			return r.code;
		}
		default:
			break;
		}
		return 0;
	}

	void Duet::SetBaudRate(const unsigned int baudRateCode)
	{
		for (unsigned int i = 0; i < ARRAY_SIZE(baudRates); i++)
		{
			if (baudRates[i].internal == baudRateCode)
			{
				SetBaudRate(baudRates[i]);
				return;
			}
		}
		warn("Baud rate %u not found", baudRateCode);
	}

	void Duet::SetBaudRate(const baudrate_t& baudRate)
	{
		info("Setting baud rate to %u (%u)", baudRate.rate, baudRate.internal);
		StoragePreferences::putInt("baud_rate", baudRate.internal);
		m_baudRate = baudRate;
		if (UARTCONTEXT->isOpen())
		{
			UARTCONTEXT->closeUart();
			Thread::sleep(50);
			UARTCONTEXT->openUart(CONFIGMANAGER->getUartName().c_str(), baudRate.internal);
		}
	}

	void Duet::SetIPAddress(const std::string& ipAddress)
	{
		if (m_communicationType == CommunicationType::network)
			Disconnect();

		StoragePreferences::putString("ip_address", ipAddress);
		m_ipAddress = ipAddress;
		Connect();
	}

	void Duet::SetHostname(const std::string& hostname)
	{
		StoragePreferences::putString("hostname", hostname);
		m_hostname.clear();
		if (!m_hostname.rfind("http://", 0))
		{
			m_hostname += "http://";
		}
		m_hostname += hostname;
	}

	void Duet::SetPassword(const std::string& password)
	{
		StoragePreferences::putString("password", password);
		m_password = password;
	}

	void Duet::SetSessionKey(const uint32_t key)
	{
		m_sessionKey = key;
		info("Set Duet session key = %u", m_sessionKey);
	}
} // namespace Comm
