/*
 * Heightmap.cpp
 *
 *  Created on: 26 Mar 2024
 *      Author: andy
 */

#include "Debug.h"

#include "Heightmap.h"

#include "Hardware/Duet.h"
#include "utils/csv.h"
#include <cmath>
#include <map>
#include <sstream>

namespace OM
{
	static std::string s_currentHeightmapName;
	static std::map<std::string, Heightmap> s_heightmapCache;
	static std::string s_emptyStr = "";

	static std::string GetLocalFilePath(const char* filename)
	{
		return utils::format("/tmp/heightmaps/%s", filename);
	}

	HeightmapMeta::HeightmapMeta()
	{
		Reset();
	}

	HeightmapMeta::~HeightmapMeta() {}

	void HeightmapMeta::Reset()
	{
		m_axis[0] = "X";
		m_axis[1] = "Y";
		m_min[0] = 0.0f;
		m_min[1] = 0.0f;
		m_max[0] = 0.0f;
		m_max[1] = 0.0f;
		m_radius = 0.0f;
		m_spacing[0] = 0.0f;
		m_spacing[1] = 0.0f;
		m_samples[0] = 0;
		m_samples[1] = 0;
	}

	void HeightmapMeta::Parse(const std::string& meta)
	{
		utils::CSV doc(meta, true);
		const std::vector<std::string> headers = doc.GetHeaders();
		for (const std::string& header : headers)
		{
			dbg("Header: \"%s\"", header.c_str());
		}

		doc.GetCell("axis0", 0, m_axis[0]);
		doc.GetCell("axis1", 0, m_axis[1]);

		doc.GetCell("min0", 0, m_min[0]);
		doc.GetCell("min1", 0, m_min[1]);
		doc.GetCell("max0", 0, m_max[0]);
		doc.GetCell("max1", 0, m_max[1]);

		doc.GetCell("radius", 0, m_radius);

		doc.GetCell("spacing0", 0, m_spacing[0]);
		doc.GetCell("spacing1", 0, m_spacing[1]);

		doc.GetCell("num0", 0, m_samples[0]);
		doc.GetCell("num1", 0, m_samples[1]);

		dbg("Axes: %s, %s", m_axis[0].c_str(), m_axis[1].c_str());
		dbg("Min: %f, %f", m_min[0], m_min[1]);
		dbg("Max: %f, %f", m_max[0], m_max[1]);
		dbg("Radius: %f", m_radius);
		dbg("Spacing: %f, %f", m_spacing[0], m_spacing[1]);
		dbg("Samples: %u, %u", m_samples[0], m_samples[1]);
	}

	Move::Axis* HeightmapMeta::GetAxis(size_t index) const
	{
		const std::string& label = m_axis[index];
		Move::Axis* axis = Move::GetAxisByLetter(label[0]);
		if (axis == nullptr)
		{
			error("Axis %s not found", label.c_str());
		}
		return axis;
	}

	Heightmap::Heightmap()
	{
		m_fileName = "";
	}

	Heightmap::Heightmap(const char* filename)
	{
		LoadFromDuet(filename);
	}

	void Heightmap::Reset()
	{
		m_fileName = "";
		m_heightmap.clear();
		meta.Reset();
		m_minError = 0.0f;
		m_maxError = 0.0f;
		m_meanError = 0.0f;
		m_stdDev = 0.0f;
		m_area = 0.0f;
	}

	bool Heightmap::LoadFromDuet(const char* filename)
	{
		Reset();
		m_fileName = filename;
		std::string csvContents;
		if (!Comm::DUET.DownloadFile(utils::format("/sys/%s", filename).c_str(), csvContents))
		{
			error("Failed to download heightmap file %s", filename);
			return false;
		}

		if (csvContents.find("RepRapFirmware height map") == std::string::npos)
		{
			warn("CSV file \"%s\" not a heightmap", filename);
			return false;
		}

		// Write the file to disk
		std::string localFilePath = GetLocalFilePath(filename);
		std::ofstream file(localFilePath.c_str(), std::ios::out | std::ios::binary);
		if (!file.is_open())
		{
			error("Failed to open file %s for writing", localFilePath.c_str());
			return false;
		}

		info("Writing heightmap to %s", localFilePath.c_str());
		file.write(csvContents.c_str(), csvContents.length());

		if (!ParseMeta(csvContents))
		{
			error("Failed to parse meta data for heightmap %s", m_fileName.c_str());
			return false;
		}

		if (!ParseData(csvContents))
		{
			error("Failed to parse data for heightmap %s", m_fileName.c_str());
			return false;
		}

		return true;
	}

	const Heightmap::Point* Heightmap::GetPoint(size_t x, size_t y) const
	{
		if (x >= GetWidth() || y >= GetHeight())
		{
			error("Invalid point %u, %u, heightmap size (%u, %u)", x, y, GetWidth(), GetHeight());
			return nullptr;
		}
		return &m_heightmap[y][x];
	}

	size_t Heightmap::GetPointCount() const
	{
		size_t count = 0;
		for (const std::vector<Point>& row : m_heightmap)
		{
			count += row.size();
		}
		return count;
	}

	bool Heightmap::ParseMeta(const std::string& csvContents)
	{
		info("Parsing meta data for heightmap %s", m_fileName.c_str());
		size_t metaStart = utils::findInstance(csvContents, "\n", 1);
		size_t metaEnd = utils::findInstance(csvContents, "\n", 3);
		if (metaStart == std::string::npos || metaEnd == std::string::npos)
		{
			error("Corrupt heightmap file %s", m_fileName.c_str());
			return false;
		}

		std::string metaStr = csvContents.substr(metaStart, metaEnd - metaStart);
		dbg("Meta:\n%s", metaStr.c_str());

		meta.Parse(metaStr);
		return true;
	}

	bool Heightmap::ParseData(const std::string& csvContents)
	{
		info("Parsing data for heightmap %s", m_fileName.c_str());
		size_t dataStart = utils::findInstance(csvContents, "\n", 3) + 1;
		if (dataStart == std::string::npos)
		{
			error("Corrupt heightmap file %s", m_fileName.c_str());
			return false;
		}

		bool parseError = false;
		std::string dataStr = csvContents.substr(dataStart);
		dbg("Data:\n%s", dataStr.c_str());
		utils::CSV doc(dataStr, false);

		m_heightmap.clear();
		m_minError = 9999.9f;
		m_maxError = -9999.9f;

		size_t rows = doc.GetRowCount();
		size_t cols = doc.GetColumnCount();

		double errorSum = 0.0f;
		double errorSqrSum = 0.0f;
		double xMin = 9999.9f;	// Used to calculate area
		double yMin = 9999.9f;	// Used to calculate area
		double xMax = -9999.9f; // Used to calculate area
		double yMax = -9999.9f; // Used to calculate area

		for (size_t rowIdx = 0; rowIdx < rows; rowIdx++)
		{
			std::vector<Point> row(cols);
			for (size_t colIdx = 0; colIdx < cols; colIdx++)
			{
				Point point;
				std::string val;
				point.x = meta.GetMin(0) + colIdx * meta.GetSpacing(0);
				point.y = meta.GetMin(1) + rowIdx * meta.GetSpacing(1);
				if (point.x < xMin)
				{
					dbg("New xMin: %.3f", point.x);
					xMin = point.x;
				}
				if (point.x > xMax)
				{
					dbg("New xMax: %.3f", point.x);
					xMax = point.x;
				}
				if (point.y < yMin)
				{
					dbg("New yMin: %.3f", point.y);
					yMin = point.y;
				}
				if (point.y > yMax)
				{
					dbg("New yMax: %.3f", point.y);
					yMax = point.y;
				}
				if (!doc.GetCell(colIdx, rowIdx, val))
				{
					error("Failed to get cell %u, %u", colIdx, rowIdx);
					point.isNull = true;
					parseError = true;
				}
				utils::removeCharFromString(val, ' ');
				if (val == "0")
				{
					point.isNull = true;
				}
				point.z = strtod(val.c_str(), NULL);

				dbg("Cell %u, %u%s: (%.3f, %.3f, %.3f) raw=\"%s\"",
					colIdx,
					rowIdx,
					point.isNull ? "[INVALID]" : "",
					point.x,
					point.y,
					point.z,
					val.c_str());
				if (point.z < m_minError)
				{
					m_minError = point.z;
				}
				if (point.z > m_maxError)
				{
					m_maxError = point.z;
				}
				errorSum += point.z;
				errorSqrSum += point.z * point.z;

				if (colIdx >= row.size())
				{
					warn("Heightmap column index out of range: %u >= %u", colIdx, row.size());
					parseError = true;
					continue;
				}
				row[colIdx] = point;
			}
			m_heightmap.push_back(row);
		}

		dbg("xMin=%.3f, xMax=%.3f, yMin=%.3f, yMax=%.3f", xMin, xMax, yMin, yMax);
		m_area =
			meta.GetRadius() > 0 ? meta.GetRadius() * meta.GetRadius() * M_PI : std::abs((xMax - xMin) * (yMax - yMin));
		m_meanError = errorSum / (rows * cols);
		m_stdDev = sqrt(errorSqrSum * GetPointCount() - errorSum * errorSum) / GetPointCount();

		if (rows != m_heightmap.size())
		{
			warn("Heightmap row count mismatch: %u != %u", rows, m_heightmap.size());
			parseError = true;
		}
		if (cols != m_heightmap.empty() ? 0 : m_heightmap[0].size())
		{
			warn("Heightmap column count mismatch: %u != %u", cols, m_heightmap.empty() ? 0 : m_heightmap[0].size());
			parseError = true;
		}

		dbg("Heightmap: %u (%u) rows, %u (%u) cols, area=%.3f mm^2, minError=%.3f mm, maxError=%.3f mm, meanError=%.3f "
			"mm, stdDev=%.3f mm",
			rows,
			m_heightmap.size(),
			cols,
			m_heightmap.empty() ? 0 : m_heightmap[0].size(),
			m_area,
			m_minError,
			m_maxError,
			m_meanError,
			m_stdDev);

		return !parseError;
	}

	const std::string& GetHeightmapNameAt(int index)
	{
		std::vector<FileSystem::FileSystemItem*> filenames = GetHeightmapFiles();
		if (index < 0 || index >= (int)filenames.size())
		{
			error("Invalid heightmap index %d", index);
			return s_emptyStr;
		}
		FileSystem::FileSystemItem* item = filenames[index];
		if (item == nullptr)
		{
			error("Filesystem item at index %d is null", index);
			return s_emptyStr;
		}
		return item->GetName();
	}

	void SetCurrentHeightmap(const std::string& filename)
	{
		size_t pos = filename.find_last_of('/');
		if (pos != std::string::npos)
		{
			s_currentHeightmapName = filename.substr(pos + 1);
		}
		else
		{
			s_currentHeightmapName = filename;
		}
		dbg("Set current heightmap to \"%s\" (%s)", s_currentHeightmapName.c_str(), filename.c_str());
	}

	void ClearCurrentHeightmap()
	{
		s_currentHeightmapName = "";
	}

	/* Sends command to Duet to use the heightmap called `filename` */
	void LoadHeightmap(const char* filename)
	{
		info("Loading heightmap %s", filename);
		Comm::DUET.SendGcodef("G29 S1 P\"%s\"", filename);
	}

	/* Sends command to Duet to unload the heightmap */
	void UnloadHeightmap()
	{
		info("Unloading heightmap");
		Comm::DUET.SendGcode("G29 S2");
	}

	void ToggleHeightmap(const char* filename)
	{
		if (s_currentHeightmapName != filename)
		{
			LoadHeightmap(filename);
			return;
		}
		UnloadHeightmap();
		ClearCurrentHeightmap();
	}

	const std::string& GetCurrentHeightmap()
	{
		return s_currentHeightmapName;
	}

	const Heightmap& GetHeightmapData(const char* filename)
	{
		auto it = s_heightmapCache.find(filename);
		if (it == s_heightmapCache.end())
		{
			Heightmap heightmap(filename);
			s_heightmapCache[filename] = heightmap;
			return s_heightmapCache[filename];
		}
		return it->second;
	}

	size_t ClearHeightmapCache()
	{
		size_t count = s_heightmapCache.size();
		s_heightmapCache.clear();
		return count - s_heightmapCache.size();
	}

	void RequestHeightmapFiles()
	{
		FileSystem::RequestFiles("/sys");
		ClearHeightmapCache();
	}

	std::vector<FileSystem::FileSystemItem*> GetHeightmapFiles()
	{
		std::vector<FileSystem::FileSystemItem*> files = FileSystem::GetItems();
		std::vector<FileSystem::FileSystemItem*> csvFiles;

		for (FileSystem::FileSystemItem* item : files)
		{
			if (item->GetType() != FileSystem::FileSystemItemType::file)
				continue;

			// Ignore non CSV files
			if (item->GetName().find(".csv\0") == std::string::npos)
				continue;

			if (item->GetName().find(DEFAULT_FILAMENTS_FILE) != std::string::npos)
				continue;

			csvFiles.push_back(item);
		}

		return csvFiles;
	}
} // namespace OM
