#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <mysql.h>
#include <errmsg.h>
#include <ctime>
#include <iomanip>
#include <chrono>

#ifndef HIVELIB_H
#define HIVELIB_H

#define HIVELIB_MYSQL_CONNECTION_COUNT 4
#define HIVELIB_MYSQL_CONNECTION_TRY 3
enum {
	HIVELIB_MYSQL_CONNECTION_PLAYER = 0,
	HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE
};

class HiveLib {
private:
	std::vector<MYSQL*> MySQLStack;
	bool debugLogQuery;
	bool debugLogResult;

	void log(const char *LogMessage);
	void log(const char *LogMessage, const char *FunctionName);
	bool connectDB(int StackIndex);

public:
	HiveLib();
	~HiveLib();
	// Get player
	std::string getPlayer(__int64 SteamId);
	// set player
	void setPlayerCop(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, const char *PlayerName);
	void setPlayerCiv(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, bool Arrested, const char *PlayerName);
	void setPlayerReb(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, bool Arrested, const char *PlayerName);
};

#endif
