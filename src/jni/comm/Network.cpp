/*
 * Network.cpp
 *
 *  Created on: 30 Jan 2024
 *      Author: Andy Everitt
 */

#include "DebugLevels.h"
#define DEBUG_LEVEL DEBUG_LEVEL_VERBOSE

#include "Configuration.h"
#include "Debug.h"
#include "Network.h"
#include "curl/curl.h"
#include "restclient-cpp/connection.h"
#include "utils/utils.h"
#include <manager/ConfigManager.h>
#include <system/Thread.h>
#include <vector>

namespace Comm
{
	struct AsyncGetData
	{
		std::string url;
		const char* subUrl;
		QueryParameters_t queryParameters;
		function<bool(RestClient::Response&)> callback;
		uint32_t sessionKey;
	};

	class AsyncGetThread : public Thread
	{
	  public:
		AsyncGetThread(std::string url,
					   const char* subUrl,
					   QueryParameters_t& queryParameters,
					   function<bool(RestClient::Response&)> callback,
					   uint32_t sessionKey)
			: m_url(url), m_subUrl(subUrl), m_queryParameters(queryParameters), m_sessionKey(sessionKey),
			  m_callback(callback)
		{
			dbg("starting thread for %s%s", url.c_str(), subUrl);
			run();
		}
		virtual bool threadLoop()
		{
			verbose("%s%s", m_url.c_str(), m_subUrl);
			if (!Get(m_url, m_subUrl, m_r, m_queryParameters, m_sessionKey))
			{
				return false;
			}

			m_callback(m_r);
			return false;
		}

		void SetRequestParameters(std::string url,
								  const char* subUrl,
								  QueryParameters_t& queryParameters,
								  function<bool(RestClient::Response&)> callback,
								  uint32_t sessionKey)
		{
			m_url = url;
			m_subUrl = subUrl;
			m_queryParameters = queryParameters;
			m_callback = callback;
			m_sessionKey = sessionKey;
		}

	  private:
		std::string m_url;
		const char* m_subUrl;
		RestClient::Response m_r;
		QueryParameters_t m_queryParameters;
		uint32_t m_sessionKey;
		function<bool(RestClient::Response&)> m_callback;
	};

	static std::vector<AsyncGetThread*> s_threadPool;
	static std::vector<AsyncGetData> s_queuedData;

	static void AddQueryParameters(std::string& url, QueryParameters_t& queryParameters)
	{
		if (queryParameters.size() > 0)
			url += "?";

		bool first = true;
		for (auto& query : queryParameters)
		{
			utils::replaceSubstring(query.second, " ", "%20");
			utils::replaceSubstring(query.second, "\"", "%22");
			utils::replaceSubstring(query.second, ":", "%3A");
			utils::replaceSubstring(query.second, "\\", "%5C");
			utils::replaceSubstring(query.second, "/", "%2F");
			utils::replaceSubstring(query.second, "\n", "%0A");
			utils::removeCharFromString(query.second, '\r');

			if (!first)
			{
				url += "&";
			}
			url += query.first;
			url += "=";
			url += query.second;

			first = false;
		}
	}

	static bool AsyncGetInner(std::string url,
							  const char* subUrl,
							  QueryParameters_t& queryParameters,
							  function<bool(RestClient::Response&)> callback,
							  uint32_t sessionKey,
							  bool queue)
	{
		// Attempts to use a thread from the pool if one is not currently in use
		for (auto thread : s_threadPool)
		{
			if (thread->isRunning())
				continue;

			verbose("Reusing thread from pool");
			thread->SetRequestParameters(url, subUrl, queryParameters, callback, sessionKey);
			return thread->run();
		}

		if (queue)
		{
			if (strncmp(subUrl, "/rr_connect", 11) == 0)
			{
				for (auto& data : s_queuedData)
				{
					if (data.url == url && data.subUrl == subUrl)
					{
						info("Request %s already queued, not adding again", (url + subUrl).c_str());
						return false;
					}
				}
			}
			s_queuedData.push_back({url, subUrl, queryParameters, callback, sessionKey});
			info("Queued request %s, size=%d", (url + subUrl).c_str(), s_queuedData.size());
			return true;
		}

		if (s_threadPool.size() >= MAX_THREAD_POOL_SIZE)
		{
			warn("Thread pool is full, cannot add more threads");
			return false;
		}

		// Create a new thread and add it to the pool
		AsyncGetThread* thread = new AsyncGetThread(url, subUrl, queryParameters, callback, sessionKey);
		s_threadPool.push_back(thread);
		info("Added thread to pool, size=%d", s_threadPool.size());
		return true;
	}

	bool AsyncGet(std::string url,
				  const char* subUrl,
				  QueryParameters_t& queryParameters,
				  function<bool(RestClient::Response&)> callback,
				  uint32_t sessionKey,
				  bool queue)
	{
		ProcessQueuedAsyncRequests();
		return AsyncGetInner(url, subUrl, queryParameters, callback, sessionKey, queue);
	}

	void ProcessQueuedAsyncRequests()
	{
		if (s_queuedData.empty())
			return;

		info("Processing queued requests, size=%d", s_queuedData.size());
		auto data = s_queuedData.begin();
		while (data != s_queuedData.end())
		{
			info("Processing queued request %s", (data->url + data->subUrl).c_str());
			if (!AsyncGetInner(data->url, data->subUrl, data->queryParameters, data->callback, data->sessionKey, false))
			{
				warn("Failed to process queued request %s", (data->url + data->subUrl).c_str());
				return;
			}
			info("Processed queued request %s", (data->url + data->subUrl).c_str());
			data = s_queuedData.erase(data);
		}
		info("Processed all queued requests, size=%d", s_queuedData.size());
	}

	int ClearThreadPool()
	{
		int count = s_threadPool.size();
		s_threadPool.clear();
		return count - s_threadPool.size();
	}

	bool Get(std::string url,
			 const char* subUrl,
			 RestClient::Response& r,
			 QueryParameters_t& queryParameters,
			 uint32_t sessionKey)
	{
		url += subUrl;

		AddQueryParameters(url, queryParameters);

		// get a connection object
		RestClient::Connection* conn = new RestClient::Connection(url);

		// set connection timeout in seconds
		conn->SetTimeout(30);

		// enable following of redirects (default is off)
		conn->FollowRedirects(true);
		// and limit the number of redirects (default is -1, unlimited)
		conn->FollowRedirects(true, 3);

		// set headers
		if (sessionKey > 0)
		{
			conn->AppendHeader("X-Session-Key", utils::format("%u", sessionKey));
			dbg("Get: \"%s\", sessionKey=%u", url.c_str(), sessionKey);
		}
		else
		{
			dbg("Get: \"%s\"", url.c_str());
		}
		conn->AppendHeader("Accept", "application/json");
		conn->AppendHeader("Content-Type", "application/json");

		// if using a non-standard Certificate Authority (CA) trust file
		conn->SetCAInfoFilePath(CONFIGMANAGER->getResFilePath("cacert.pem"));

		r = conn->get("");
		if (r.code != 200)
		{
			error("%s failed, returned response %d", url.c_str(), r.code);
			return false;
		}
		dbg("%s succeeded, returned response %d", url.c_str(), r.code);
		verbose("Response body: %s", r.body.c_str());

		delete conn;
		return true;
	}

	bool Post(std::string url,
			  const char* subUrl,
			  RestClient::Response& r,
			  QueryParameters_t& queryParameters,
			  const std::string& data,
			  uint32_t sessionKey)
	{
		url += subUrl;

		AddQueryParameters(url, queryParameters);

		// get a connection object
		RestClient::Connection* conn = new RestClient::Connection(url);

		// set connection timeout in seconds
		conn->SetTimeout(180);

		// enable following of redirects (default is off)
		conn->FollowRedirects(true);
		// and limit the number of redirects (default is -1, unlimited)
		conn->FollowRedirects(true, 3);

		// set headers
		conn->AppendHeader("X-Session-Key", utils::format("%u", sessionKey));

		conn->AppendHeader("Content-Type", "text/plain");
		// if using a non-standard Certificate Authority (CA) trust file
		// conn->SetCAInfoFilePath(ConfigManager::getInstance()->getResFilePath("cacert.pem"));

		verbose("Post: \"%s\", data=\"%s\"", url.c_str(), data.substr(0, 50).c_str());
		r = conn->post("", data);
		if (r.code != 200)
		{
			error("%s failed, returned response %d %s", url.c_str(), r.code, r.body.c_str());
			return false;
		}
		dbg("%s succeeded, returned response %d", url.c_str(), r.code);
		verbose("Response body: %s", r.body.c_str());
		return true;
	}
} // namespace Comm
