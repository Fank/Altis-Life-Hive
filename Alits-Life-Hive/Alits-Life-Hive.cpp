// Alits-Life-Hive.cpp : Definiert die exportierten Funktionen für die DLL-Anwendung.
//

#include "stdafx.h"
#include "HiveLib\HiveLib.hpp"

HiveLib *HiveLibrary;

extern "C" {
	__declspec (dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

void __stdcall RVExtension(char *output, int outputSize, const char *function) {
	std::vector<std::string> rawCmd = split(std::string(function), ':');
	std::string hiveOutput = "";

	if (rawCmd.size() > 0) {
		// Init HiveLib
		if (rawCmd[0] == "0") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
		}
		// Get player information
		else if (rawCmd[0] == "100") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() == 2) {
				hiveOutput = HiveLibrary->getPlayer(_atoi64(rawCmd[1].c_str()));
			}
		}

		// Update civilian information
		else if (rawCmd[0] == "101") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 7) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 7;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 7) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerCiv(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), rawCmd[4].c_str(), rawCmd[5].c_str(), (rawCmd[6].compare("0") ? true : false), playerName.str().c_str());
			}
		}
		// Update resistance information
		else if (rawCmd[0] == "102") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 7) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 7;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 7) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerReb(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), rawCmd[4].c_str(), rawCmd[5].c_str(), (rawCmd[6].compare("0") ? true : false), playerName.str().c_str());
			}
		}
		// Update cop information
		else if (rawCmd[0] == "103") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 6) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 6;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 6) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerCop(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), rawCmd[4].c_str(), rawCmd[5].c_str(), playerName.str().c_str());
			}
		}

		// Add civilian information
		else if (rawCmd[0] == "111") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 5) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 5;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 5) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerCiv(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), "[]", "[]", (rawCmd[4].compare("0") ? true : false), playerName.str().c_str());
			}
		}
		// Add resistance information
		else if (rawCmd[0] == "112") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 5) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 5;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 5) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerReb(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), "[]", "[]", (rawCmd[4].compare("0") ? true : false), playerName.str().c_str());
			}
		}
		// Add cop information
		else if (rawCmd[0] == "113") {
			if (HiveLibrary == NULL) {
				HiveLibrary = new HiveLib();
			}
			if (rawCmd.size() >= 5) {
				std::stringstream playerName;
				for (std::vector<std::string>::iterator
					it = rawCmd.begin() + 5;
					it != rawCmd.end();
				) {
					playerName << ((rawCmd.begin() + 5) != it ? ":" : "") << *it;
					it++;
				}
				HiveLibrary->setPlayerCop(_atoi64(rawCmd[1].c_str()), atoi(rawCmd[2].c_str()), atoi(rawCmd[3].c_str()), rawCmd[4].c_str(), "[]", playerName.str().c_str());
			}
		}
	}

	strncpy_s(output, outputSize, hiveOutput.c_str(), _TRUNCATE);
}