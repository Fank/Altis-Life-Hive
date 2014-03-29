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
	}

	strncpy_s(output, outputSize, hiveOutput.c_str(), _TRUNCATE);
}