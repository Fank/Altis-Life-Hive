#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <mysql.h>
#include <errmsg.h>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <config4cpp\Configuration.h>

#ifndef HIVELIB_H
#define HIVELIB_H

#define HIVELIB_MYSQL_CONNECTION_COUNT 4
#define HIVELIB_MYSQL_CONNECTION_TRY 3
enum {
	HIVELIB_MYSQL_CONNECTION_PLAYER = 0,
	HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE,
	HIVELIB_MYSQL_CONNECTION_VEHICLE,
	HIVELIB_MYSQL_CONNECTION_HOUSING
};

struct HiveLibDB {
	std::string Hostname;
	std::string Username;
	std::string Password;
	std::string Database;
	unsigned short Port;
};

class HiveLib {
private:
	std::vector<MYSQL*> MySQLStack;
	bool debugLogQuery;
	bool debugLogResult;

	// Configfile
	char *profilePath;
	config4cpp::Configuration *configuration;
	HiveLibDB dbConnection;

	// Logging
	void log(const char *LogMessage);
	void log(const char *LogMessage, const char *FunctionName);
	void log(const char *LogMessage, const char *FunctionName, MYSQL *SQLConnection);
	void log(const char *LogMessage, const char *FunctionName, MYSQL_STMT *SQLStatement);

	// Database connection
	bool dbConnect(int StackIndex);
	void dbCheck(int StackIndex);

public:
	HiveLib(char *ProfilePath);
	~HiveLib();

	// Get player
	std::string getPlayer(__int64 SteamId);

	// set player
	void setPlayerCop(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, const char *PlayerName);
	void setPlayerCiv(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, bool Arrested, const char *PlayerName);
	void setPlayerReb(__int64 SteamId, int Cash, int Bank, const char *Gear, const char *Licenses, bool Arrested, const char *PlayerName);
	void setPlayerCash(__int64 SteamId, int Cash);
	void setPlayerBankAcc(__int64 SteamId, int BankAcc);

	// Get vehicle
	std::string getVehicle(__int64 SteamId, int VehicleId);
	std::string getVehicles(__int64 SteamId, const char *Side, const char *Type);

	// Insert new vehicles
	std::string insertVehicle(__int64 SteamId, char *Side, char *Type, char *Classname, int Color);

	// Set vehicle
	void setVehicleActive(__int64 SteamId, int Id, bool Active);
	void setVehicleAlive(__int64 SteamId, int Id, bool Alive);

	// Tools
	void resetVehicles();

	// Get house
	std::string getHouse(int HouseObjectId);
	std::string getHouses(__int64 SteamId);

	// House management
	std::string buyHouse(__int64 SteamId, int HouseObjectId);
	std::string sellHouse(__int64 SteamId, int HouseObjectId);
	std::string upgradeHouse(__int64 SteamId, int HouseObjectId);

	// House actions
	void updateHouseInventory(int HouseObjectId, char *Inventory);
	void updateHouseInventoryArma(int HouseObjectId, char *Inventory);

	// Mod1 Vehicles
	std::string getMod1Vehicle(int VehicleId);
	std::string getMod1Vehicles(char *Side, int VehicleType);

	// Mod1 Weapons
	std::string getMod1Weapon(int WeaponId);
	std::string getMod1Weapons(char *Side, int WeaponType);

};

#endif
