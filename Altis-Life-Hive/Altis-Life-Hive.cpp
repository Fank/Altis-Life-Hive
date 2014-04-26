// Altis-Life-Hive.cpp : Definiert die exportierten Funktionen für die DLL-Anwendung.
//

#include "stdafx.h"
#include "HiveLib\HiveLib.hpp"
#include <shellapi.h>

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

std::string getProfileFolder() {
	std::string profileFolder = "";
	LPTSTR cmdLine = GetCommandLine();
	int numCmdLineArgs = 0;
	LPTSTR *cmdLineArgs = CommandLineToArgvW(cmdLine, &numCmdLineArgs);

	std::vector<std::string> commandLine;
	commandLine.reserve(numCmdLineArgs);

	for (int i = 0; i < numCmdLineArgs; i++) {
		std::wstring args(cmdLineArgs[i]);
		std::string utf8(args.begin(), args.end());
		commandLine.push_back(utf8);
	}

	for (std::vector<std::string>::iterator it = commandLine.begin(); it != commandLine.end(); it++) {
		std::string starter = "-profiles=";
		if (it->length() < starter.length()) {
			continue;
		}

		std::string compareMe = it->substr(0, starter.length());
		if (compareMe.compare(starter) != 0) {
			continue;
		}

		profileFolder = it->substr(compareMe.length());
	}

	return profileFolder;
}

// Init HiveLib
std::string handler0(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		HiveLibrary = new HiveLib((char *)getProfileFolder().c_str());
	}
	return "";
}

// Get player information
std::string handler100(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
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
		exit(1);
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
		exit(1);
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
		exit(1);
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
		exit(1);
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
		exit(1);
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
		exit(1);
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

// Update player cash
std::string handler114(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}
	if (_param.size() == 3) {
		try {
			HiveLibrary->setPlayerCash(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}
	return "";
}

// Update player bankacc
std::string handler115(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}
	if (_param.size() == 3) {
		try {
			HiveLibrary->setPlayerBankAcc(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}
	return "";
}

// Get vehicles
std::string handler200(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
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
		exit(1);
	}

	if (_param.size() == 6) {
		try {
			return HiveLibrary->insertVehicle(_atoi64(_param[1].c_str()), (char *)_param[2].c_str(), (char *)_param[3].c_str(), (char *)_param[4].c_str(), atoi(_param[5].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// set vehicle active
std::string handler202(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 4) {
		try {
			HiveLibrary->setVehicleActive(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), (_param[3].compare("1") ? false : true));
		}
		catch (...) {

		}
	}

	return "";
}

// set vehicle alive
std::string handler203(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 4) {
		try {
			HiveLibrary->setVehicleAlive(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()), (_param[3].compare("1") ? false : true));
		}
		catch (...) {

		}
	}

	return "";
}

// get vehicle
std::string handler204(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->getVehicle(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// set vehicle alive
std::string handler299(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	try {
		HiveLibrary->resetVehicles();
	}
	catch (...) {

	}

	return "";
}

// get house
std::string handler300(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 2) {
		try {
			return HiveLibrary->getHouse(atoi(_param[1].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// get houses
std::string handler301(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 2) {
		try {
			return HiveLibrary->getHouses(_atoi64(_param[1].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// buy house
std::string handler302(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->buyHouse(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// sell house
std::string handler303(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->sellHouse(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// upgrade house
std::string handler304(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->upgradeHouse(_atoi64(_param[1].c_str()), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

// update house inventory
std::string handler305(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			HiveLibrary->updateHouseInventory(atoi(_param[1].c_str()), (char *)_param[2].c_str());
		}
		catch (...) {

		}
	}

	return "";
}

// update house inventory arma
std::string handler306(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			HiveLibrary->updateHouseInventoryArma(atoi(_param[1].c_str()), (char *)_param[2].c_str());
		}
		catch (...) {

		}
	}

	return "";
}

std::string handler400(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->getMod1Vehicles((char *)_param[1].c_str(), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

std::string handler401(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 2) {
		try {
			return HiveLibrary->getMod1Vehicle(atoi(_param[1].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

std::string handler402(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 3) {
		try {
			return HiveLibrary->getMod1Weapons((char *)_param[1].c_str(), atoi(_param[2].c_str()));
		}
		catch (...) {

		}
	}

	return "";
}

std::string handler403(std::vector<std::string> _param) {
	if (HiveLibrary == NULL) {
		exit(1);
	}

	if (_param.size() == 2) {
		try {
			return HiveLibrary->getMod1Weapon(atoi(_param[1].c_str()));
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
		else if (rawCmd[0] == "114") {
			hiveOutput = handler114(rawCmd);
		}
		else if (rawCmd[0] == "115") {
			hiveOutput = handler115(rawCmd);
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
		else if (rawCmd[0] == "204") {
			hiveOutput = handler204(rawCmd);
		}
		else if (rawCmd[0] == "299") {
			hiveOutput = handler299(rawCmd);
		}
		else if (rawCmd[0] == "300") {
			hiveOutput = handler300(rawCmd);
		}
		else if (rawCmd[0] == "301") {
			hiveOutput = handler301(rawCmd);
		}
		else if (rawCmd[0] == "302") {
			hiveOutput = handler302(rawCmd);
		}
		else if (rawCmd[0] == "303") {
			hiveOutput = handler303(rawCmd);
		}
		else if (rawCmd[0] == "304") {
			hiveOutput = handler304(rawCmd);
		}
		else if (rawCmd[0] == "305") {
			hiveOutput = handler305(rawCmd);
		}
		else if (rawCmd[0] == "306") {
			hiveOutput = handler306(rawCmd);
		}
		else if (rawCmd[0] == "400") {
			hiveOutput = handler400(rawCmd);
		}
		else if (rawCmd[0] == "401") {
			hiveOutput = handler401(rawCmd);
		}
		else if (rawCmd[0] == "402") {
			hiveOutput = handler402(rawCmd);
		}
		else if (rawCmd[0] == "403") {
			hiveOutput = handler403(rawCmd);
		}
	}

	strncpy_s(output, outputSize, hiveOutput.c_str(), _TRUNCATE);
}