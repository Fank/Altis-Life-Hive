#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <my_global.h>
#include <mysql.h>

#ifndef HIVELIB_H
#define HIVELIB_H

#define HIVELIB_MYSQL_CONNECTION_COUNT 4
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

public:
	HiveLib();
	~HiveLib();
	std::string getPlayer(__int64 SteamId);
};

#endif
