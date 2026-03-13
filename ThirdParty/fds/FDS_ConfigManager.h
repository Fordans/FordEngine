/*
		Copyright(C) 2025 Fordans
*/

#pragma once

#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <stdexcept>
#include <system_error>

class FDS_ConfigManager
{
public:
	enum class LoadStatus
	{
		Success,           // Config file loaded successfully
		FileNotFound,      // Config file does not exist (will be created on save)
		ReadError,         // Error reading from file
		NotLoaded          // Config not yet loaded
	};

	FDS_ConfigManager(const std::string& file_name = "settings.cfg");
	~FDS_ConfigManager();

    /*
		The config file follows this structure:
			[Filter]
			key=value
		Value types are provided as follows:
			bool
			int
			float
			std::string
	*/
	template<typename T>
	void setConfig(const std::string& filter, const std::string& key, const T& value);

	/*
		The config file follows this structure:
			[Filter]
			key=value
		Value types are provided as follows:
			bool
			int
			float
			std::string
	*/
	template<typename T>
	T getConfig(const std::string& filter, const std::string& key);

	// Query loading status
	LoadStatus getLoadStatus() const noexcept { return m_loadStatus; }
	bool isLoaded() const noexcept { return m_loadStatus == LoadStatus::Success; }
	bool isFileNotFound() const noexcept { return m_loadStatus == LoadStatus::FileNotFound; }
	const std::string& getLastError() const noexcept { return m_lastError; }

	// Reload config from file
	void reload();

private:
	void loadConfig();
	void saveConfig();

private:
	std::string m_fileName;
	std::map<std::string, std::map<std::string, std::string>> m_configData; // Filter -> (Key -> Value)
	LoadStatus m_loadStatus = LoadStatus::NotLoaded;
	std::string m_lastError;
};

FDS_ConfigManager::FDS_ConfigManager(const std::string& file_name) : m_fileName(file_name)
{
	loadConfig();
}

void FDS_ConfigManager::reload()
{
	m_configData.clear();
	loadConfig();
}

FDS_ConfigManager::~FDS_ConfigManager()
{
	saveConfig();
}

void FDS_ConfigManager::loadConfig()
{
	m_loadStatus = LoadStatus::NotLoaded;
	m_lastError.clear();

	// Try to open file to check if it exists and is readable
	std::ifstream ifs(m_fileName);
	if (!ifs.is_open())
	{
		// File doesn't exist or cannot be opened
		// On most systems, if file doesn't exist, is_open() returns false
		// This is expected behavior for a new config file
		m_loadStatus = LoadStatus::FileNotFound;
		m_lastError = "Config file not found: " + m_fileName + " (will be created on save)";
		return;
	}

	// Check if file is actually readable (not just openable)
	// Try to peek at the first character to verify read access
	ifs.peek();
	if (ifs.fail() && !ifs.eof())
	{
		m_loadStatus = LoadStatus::ReadError;
		m_lastError = "Failed to read from config file: " + m_fileName;
		ifs.close();
		return;
	}
	
	// Reset stream state and position for actual reading
	ifs.clear();
	ifs.seekg(0, std::ios::beg);

	std::string currentFilter;
	std::string line;

	try
	{
		while (std::getline(ifs, line))
		{
			// Remove leading/trailing whitespace
			size_t first = line.find_first_not_of(" \t\r\n");
			if (first == std::string::npos)
			{
				continue; // Empty line.
			}
			line = line.substr(first);

			size_t last = line.find_last_not_of(" \t\r\n");
			line = line.substr(0, last + 1);

			if (line.empty()) continue;

			if (line[0] == '[')
			{
				// Filter line
				size_t endBracket = line.find(']');
				if (endBracket != std::string::npos)
				{
					currentFilter = line.substr(1, endBracket - 1);
				}
			}
			else
			{
				// Key=Value line
				size_t equalsPos = line.find('=');
				if (equalsPos != std::string::npos)
				{
					std::string key = line.substr(0, equalsPos);
					std::string value = line.substr(equalsPos + 1);

					first = key.find_first_not_of(" \t\r\n");
					key = key.substr(first);
					last = key.find_last_not_of(" \t\r\n");
					key = key.substr(0, last + 1);

					first = value.find_first_not_of(" \t\r\n");
					value = value.substr(first);
					last = value.find_last_not_of(" \t\r\n");
					value = value.substr(0, last + 1);

					m_configData[currentFilter][key] = value;
				}
			}
		}

		// Check for read errors
		if (ifs.bad())
		{
			m_loadStatus = LoadStatus::ReadError;
			m_lastError = "Error reading from config file: " + m_fileName;
			ifs.close();
			return;
		}

		ifs.close();
		m_loadStatus = LoadStatus::Success;
		m_lastError.clear();
	}
	catch (const std::exception& e)
	{
		m_loadStatus = LoadStatus::ReadError;
		m_lastError = "Exception while reading config file: " + std::string(e.what());
		ifs.close();
	}
}

void FDS_ConfigManager::saveConfig()
{
	std::ofstream ofs(m_fileName);
	if (!ofs.is_open())
	{
		throw std::runtime_error("Failed to open: " + m_fileName);
		return;
	}

	for (const auto& filterPair : m_configData)
	{
		ofs << "[" << filterPair.first << "]" << std::endl;
		for (const auto& keyValuePair : filterPair.second)
		{
			ofs << keyValuePair.first << "=" << keyValuePair.second << std::endl;
		}
		ofs << std::endl;
	}

	ofs.close();
}

template<typename T>
void FDS_ConfigManager::setConfig(const std::string& filter, const std::string& key, const T& value)
{
	std::stringstream ss;
	ss << value;
	m_configData[filter][key] = ss.str();
}

template<>
void FDS_ConfigManager::setConfig(const std::string& filter, const std::string& key, const bool& value)
{
	m_configData[filter][key] = value ? "true" : "false";
}


template<typename T>
T FDS_ConfigManager::getConfig(const std::string& filter, const std::string& key)
{
	auto filterIt = m_configData.find(filter);
	if (filterIt == m_configData.end())
	{
		throw std::runtime_error("Filter not found: " + filter);
	}

	auto keyIt = filterIt->second.find(key);
	if (keyIt == filterIt->second.end())
	{
		throw std::runtime_error("Key not found: " + key + " in filter: " + filter);
	}

	std::stringstream ss(keyIt->second);
	T value;
	ss >> value;

	if (ss.fail())
	{
		throw std::runtime_error("Failed to convert value to requested type for key: " + key + " in filter: " + filter);
	}

	return value;
}

template<>
bool FDS_ConfigManager::getConfig(const std::string& filter, const std::string& key)
{
	auto filterIt = m_configData.find(filter);
	if (filterIt == m_configData.end())
	{
		throw std::runtime_error("Filter not found: " + filter);
	}

	auto keyIt = filterIt->second.find(key);
	if (keyIt == filterIt->second.end())
	{
		throw std::runtime_error("Key not found: " + key + " in filter: " + filter);
	}

	std::string value = keyIt->second;
	if (value == "true" || value == "True" || value == "1")
	{
		return true;
	}
	else if (value == "false" || value == "False" || value == "0")
	{
		return false;
	}
	else
	{
		throw std::runtime_error("Invalid boolean value for key: " + key + " in filter: " + filter);
	}
}