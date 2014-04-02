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

// Init HiveLib
std::string handler0(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	return "";
}

// Get player information
std::string handler100(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() == 2) {
		return HiveLibrary->getPlayer(_atoi64(_param[1].c_str()));
	}
	else {
		return "";
	}
}

// Update civilian information
std::string handler101(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 7) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 7;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 7) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerCiv(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), _param[4].c_str(), _param[5].c_str(), (_param[6].compare("0") ? true : false), playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Update resistance information
std::string handler102(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 7) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 7;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 7) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerReb(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), _param[4].c_str(), _param[5].c_str(), (_param[6].compare("0") ? true : false), playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Update cop information
std::string handler103(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 6) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 6;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 6) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerCop(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), _param[4].c_str(), _param[5].c_str(), playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Add civilian information
std::string handler111(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 5) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 5;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 5) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerCiv(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), "[]", "[]", (_param[4].compare("0") ? true : false), playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Add resistance information
std::string handler112(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 5) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 5;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 5) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerReb(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), "[]", "[]", (_param[4].compare("0") ? true : false), playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Add cop information
std::string handler113(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 5) {
		std::stringstream playerName;
		for (std::vector<std::string>::iterator
			it = _param.begin() + 5;
			it != _param.end();
		) {
			playerName << ((_param.begin() + 5) != it ? ":" : "") << *it;
			it++;
		}

		try {
			HiveLibrary->setPlayerCop(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), atoi(_param[3].c_str()), _param[4].c_str(), "[]", playerName.str().c_str());
		}
		catch (...) {

		}
	}
	return "";
}

// Get vehicles
std::string handler200(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}
	if (_param.size() >= 4) {
		return HiveLibrary->getVehicles(_atoi64(_param[1].c_str()), _param[2].c_str(), _param[3].c_str());
	}
	else {
		return "";
	}
}

// Insert vehicle
std::string handler201(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}

	if (_param.size() == 7) {
		try {
			HiveLibrary->insertVehicle(_atoi64(_param[1].c_str()), (char *)_param[2].c_str(), (char *)_param[3].c_str(), (char *)_param[4].c_str(), atoi(_param[5].c_str()), atoi(_param[6].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// set vehicle active
std::string handler202(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}

	if (_param.size() == 4) {
		try {
			HiveLibrary->setVehicleActive(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), (_param[3].compare("1") ? true : false));
		}
		catch (...) {

		}
	}

	return "";
}

// set vehicle alive
std::string handler203(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib();
	}

	if (_param.size() == 4) {
		try {
			HiveLibrary->setVehicleAlive(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), (_param[3].compare("1") ? true : false));
		}
		catch (...) {

		}
	}

	return "";
}
void __stdcall RVExtension(char *output, int outputSize, const char *function) {
	std::vector<std::string> rawCmd = split(std::string(function), ':');
	std::string hiveOutput = "";

	if (rawCmd.size() > 0) {
		if (rawCmd[0] == "0") {
			hiveOutput = handler0(rawCmd);
		}
		else if (rawCmd[0] == "100") {
			hiveOutput = handler100(rawCmd);
		}
		else if (rawCmd[0] == "101") {
			hiveOutput = handler101(rawCmd);
		}
		else if (rawCmd[0] == "102") {
			hiveOutput = handler102(rawCmd);
		}
		else if (rawCmd[0] == "103") {
			hiveOutput = handler103(rawCmd);
		}
		else if (rawCmd[0] == "111") {
			hiveOutput = handler111(rawCmd);
		}
		else if (rawCmd[0] == "112") {
			hiveOutput = handler112(rawCmd);
		}
		else if (rawCmd[0] == "113") {
			hiveOutput = handler113(rawCmd);
		}
		else if (rawCmd[0] == "200") {
			hiveOutput = handler200(rawCmd);
		}
		else if (rawCmd[0] == "201") {
			hiveOutput = handler201(rawCmd);
		}
		else if (rawCmd[0] == "202") {
			hiveOutput = handler202(rawCmd);
		}
		else if (rawCmd[0] == "203") {
			hiveOutput = handler203(rawCmd);
		}
	}

	strncpy_s(output, outputSize, hiveOutput.c_str(), _TRUNCATE);
}