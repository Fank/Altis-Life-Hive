#include "stdafx.h"
#include "HiveLib.hpp"
#include "SQF.hpp"

HiveLib::HiveLib(char *_profilePath) {
	// Disable debug
	this->debugLogQuery = false;
	this->debugLogResult = false;

	// Set profile path
	const char *configFile = "AltisLifeHive.cfg";
	this->profilePath = _profilePath;

	// Open configfile
	this->configuration = config4cpp::Configuration::create();
	std::stringstream configFilePath;
	configFilePath << this->profilePath << "/" << configFile;

	// Get config from profile folder
	try {
		this->configuration->parse(configFilePath.str().c_str());
	}
	catch (const config4cpp::ConfigurationException & exProfile) {
		std::stringstream errorMsg;
		errorMsg << exProfile.c_str() << " -- fallback to config in root folder";
		this->log(errorMsg.str().c_str(), __FUNCTION__);
		
		// Get config from root folder
		try {
			this->configuration->parse(configFile);
		}
		catch (const config4cpp::ConfigurationException & exRoot) {
			this->log(exRoot.c_str(), __FUNCTION__);
			this->configuration->destroy();
			exit(1);
		}
	}

	try {
		// Parse configfile
		this->configuration->parse(configFilePath.str().c_str());

		// Read config
		this->dbConnection.Hostname = this->configuration->lookupString("", "Hostname");
		this->dbConnection.Username = this->configuration->lookupString("", "Username");
		this->dbConnection.Password = this->configuration->lookupString("", "Password");
		this->dbConnection.Database = this->configuration->lookupString("", "Database");
		this->dbConnection.Port = this->configuration->lookupInt("", "Port");
	}
	catch (const config4cpp::ConfigurationException & ex) {
		this->log(ex.c_str(), __FUNCTION__);
		this->configuration->destroy();
		exit(1);
	}

	// Init MySQL connections
	for (int i = 0; i < HIVELIB_MYSQL_CONNECTION_COUNT; i++) {
		MYSQL *con = mysql_init(NULL);
		if (con == NULL) {
			exit(1);
		}

		mysql_options(con, MYSQL_SET_CHARSET_NAME, "utf8");
		mysql_options(con, MYSQL_INIT_COMMAND, "SET NAMES utf8");

		this->MySQLStack.push_back(con);

		if (!this->dbConnect(i)) {
			std::stringstream errorMsg;
			errorMsg << "Failed to connect to database: " << mysql_error(this->MySQLStack[i]);
			this->log(errorMsg.str().c_str(), __FUNCTION__);
			exit(1);
		}

	}

	this->configuration->destroy();
}
HiveLib::~HiveLib() {
	// Close all MySQL connections
	for (int i = 0; i < HIVELIB_MYSQL_CONNECTION_COUNT; i++) {
		mysql_close(this->MySQLStack[i]);
	}
}

void HiveLib::log(const char *_logMessage) {
	std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct std::tm *time = std::localtime(&currentTime);

	std::ofstream logFile;
	logFile.open("HiveLib.log", std::ios::out | std::ios::app);
	logFile << "[" << std::put_time(time, "%Y-%m-%d %H:%M:%S") << "] " << _logMessage << std::endl;
	logFile.close();
}
void HiveLib::log(const char *_logMessage, const char *_functionName) {
	std::stringstream logMessage;
	logMessage << _functionName << ": " << _logMessage;
	this->log(logMessage.str().c_str());
}
void HiveLib::log(const char *_logMessage, const char *_functionName, MYSQL *_sqlConnection) {
	std::stringstream logMessage;
	logMessage << _logMessage << " (" << mysql_errno(_sqlConnection) << ") " << mysql_error(_sqlConnection);
	this->log(logMessage.str().c_str(), _functionName);
}
void HiveLib::log(const char *_logMessage, const char *_functionName, MYSQL_STMT *_sqlStatement) {
	std::stringstream logMessage;
	logMessage << _logMessage << " (" <<  mysql_stmt_errno(_sqlStatement) << ") " << mysql_stmt_error(_sqlStatement);
	this->log(logMessage.str().c_str(), _functionName);
}

bool HiveLib::dbConnect(int _stackIndex) {
	if (!mysql_real_connect(this->MySQLStack[_stackIndex], this->dbConnection.Hostname.c_str(), this->dbConnection.Username.c_str(), this->dbConnection.Password.c_str(), this->dbConnection.Database.c_str(), this->dbConnection.Port, NULL, 0)) {
		this->log("Failed to connect to database: ", __FUNCTION__, this->MySQLStack[_stackIndex]);
		return false;
	}
	else {
		return true;
	}
}
void HiveLib::dbCheck(int _stackIndex) {
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[_stackIndex])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			if (reconnectTry > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->dbConnect(_stackIndex);
			reconnectTry++;
		}
	}
}

// Get player
std::string HiveLib::getPlayer(__int64 _steamId) {
	std::string playerString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "`playerid`, REPLACE(`name`, '\"', '\"\"'), `adminlevel`, `blacklist`, `donatorlvl`, `arrested`, "; // Player
	sqlQuery << "`cash`, `bankacc`, "; // Money
	sqlQuery << "`coplevel`, REPLACE(`cop_licenses`, '\"', ''), REPLACE(`cop_gear`, '\"', ''), "; // COP
	sqlQuery << "REPLACE(`civ_licenses`, '\"', ''), REPLACE(`civ_gear`, '\"', ''), "; // Civ
	sqlQuery << "`reblevel`, REPLACE(`reb_gear`, '\"', '') "; // Reb
	sqlQuery << "FROM `players` ";
	sqlQuery << "WHERE `playerid` = '" << _steamId << "'";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_PLAYER);

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYER], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYER]);
		return playerString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYER]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF playerRow;
		playerRow.push_str(queryRow[0]);
		playerRow.push_str(queryRow[1]);
		playerRow.push_str(queryRow[2]);
		playerRow.push_str(queryRow[3]);
		playerRow.push_str(queryRow[4]);
		playerRow.push_str(queryRow[5]);
		playerRow.push_str(queryRow[6]);
		playerRow.push_str(queryRow[7]);
		playerRow.push_str(queryRow[8]);
		playerRow.push_str(queryRow[9]);
		playerRow.push_str(queryRow[10]);
		playerRow.push_str(queryRow[11]);
		playerRow.push_str(queryRow[12]);
		playerRow.push_str(queryRow[13]);
		playerRow.push_str(queryRow[14]);
		playerString = playerRow.toArray();
		if (this->debugLogResult) {
			this->log(playerString.c_str(), __FUNCTION__);
		}
	}

	mysql_free_result(queryResult);

	return playerString;
}
// Set player
void HiveLib::setPlayerCop(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[5];

	std::stringstream sqlQuery;
	sqlQuery << "INSERT INTO players (`playerid`, `name`, `cash`, `bankacc`, `cop_gear`) VALUES ( ";
	sqlQuery << "'" << _steamId << "', "; // Insert steamId
	sqlQuery << "?, "; // Insert name
	sqlQuery << "'" << _cash << "', "; // Insert cash
	sqlQuery << "'" << _bank << "', "; // Insert bank
	sqlQuery << "REPLACE(?, '\"', '') "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`cop_gear` = REPLACE(?, '\"', ''), ";
	sqlQuery << "`cop_licenses` = REPLACE(?, '\"', ''), ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
		return;
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// Zero out the sqlParam data structures
	memset(sqlParam, 0, sizeof(sqlParam));

	// insert bind player name
	long unsigned int insertPlayerNameLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 32;
	sqlParam[0].buffer = (char *)_playerName;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &insertPlayerNameLength;

	// insert bind gear
	long unsigned int InsertGearLength;
	sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[1].buffer_length = 4096;
	sqlParam[1].buffer = (char *)_gear;
	sqlParam[1].is_null = 0;
	sqlParam[1].length = &InsertGearLength;

	// update bind player name
	long unsigned int updatePlayerNameLength;
	sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[2].buffer_length = 32;
	sqlParam[2].buffer = (char *)_playerName;
	sqlParam[2].is_null = 0;
	sqlParam[2].length = &updatePlayerNameLength;

	// update bind gear
	long unsigned int updateGearLength;
	sqlParam[3].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[3].buffer_length = 4096;
	sqlParam[3].buffer = (char *)_gear;
	sqlParam[3].is_null = 0;
	sqlParam[3].length = &updateGearLength;

	// update bind licenses
	long unsigned int updateLicensesLength;
	sqlParam[4].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[4].buffer_length = 4096;
	sqlParam[4].buffer = (char *)_licenses;
	sqlParam[4].is_null = 0;
	sqlParam[4].length = &updateLicensesLength;

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	insertPlayerNameLength = strlen(_playerName);
	InsertGearLength = strlen(_gear);
	updatePlayerNameLength = strlen(_playerName);
	updateGearLength = strlen(_gear);
	updateLicensesLength = strlen(_licenses);

	// Request meta data information
	MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

	// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
	my_bool aBool = 1;
	mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// success :)
	if (this->debugLogResult) {
		std::stringstream result;
		result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
		this->log(result.str().c_str(), __FUNCTION__);
	}

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);
	mysql_free_result(sqlResult);
}
void HiveLib::setPlayerCiv(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, bool _arrested, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[5];

	std::stringstream sqlQuery;
	sqlQuery << "INSERT INTO players (`playerid`, `name`, `cash`, `bankacc`, `civ_gear`) VALUES ( ";
	sqlQuery << "'" << _steamId << "', "; // Insert steamId
	sqlQuery << "?, "; // Insert name
	sqlQuery << "'" << _cash << "', "; // Insert cash
	sqlQuery << "'" << _bank << "', "; // Insert bank
	sqlQuery << "REPLACE(?, '\"', '') "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`civ_gear` = REPLACE(?, '\"', ''), ";
	sqlQuery << "`civ_licenses` = REPLACE(?, '\"', ''), ";
	sqlQuery << "`arrested` = " << (_arrested ? 1 : 0) << ", ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
		return;
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// Zero out the sqlParam and sqlResult data structures
	memset(sqlParam, 0, sizeof(sqlParam));

	// insert bind player name
	long unsigned int insertPlayerNameLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 32;
	sqlParam[0].buffer = (char *)_playerName;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &insertPlayerNameLength;

	// insert bind gear
	long unsigned int InsertGearLength;
	sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[1].buffer_length = 4096;
	sqlParam[1].buffer = (char *)_gear;
	sqlParam[1].is_null = 0;
	sqlParam[1].length = &InsertGearLength;

	// update bind player name
	long unsigned int updatePlayerNameLength;
	sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[2].buffer_length = 32;
	sqlParam[2].buffer = (char *)_playerName;
	sqlParam[2].is_null = 0;
	sqlParam[2].length = &updatePlayerNameLength;

	// update bind gear
	long unsigned int updateGearLength;
	sqlParam[3].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[3].buffer_length = 4096;
	sqlParam[3].buffer = (char *)_gear;
	sqlParam[3].is_null = 0;
	sqlParam[3].length = &updateGearLength;

	// update bind licenses
	long unsigned int updateLicensesLength;
	sqlParam[4].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[4].buffer_length = 4096;
	sqlParam[4].buffer = (char *)_licenses;
	sqlParam[4].is_null = 0;
	sqlParam[4].length = &updateLicensesLength;

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed", __FUNCTION__, sqlStatement);
		return;
	}

	insertPlayerNameLength = strlen(_playerName);
	InsertGearLength = strlen(_gear);
	updatePlayerNameLength = strlen(_playerName);
	updateGearLength = strlen(_gear);
	updateLicensesLength = strlen(_licenses);

	// Request meta data information
	MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

	// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
	my_bool aBool = 1;
	mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed", __FUNCTION__, sqlStatement);
		return;
	}

	// success :)
	if (this->debugLogResult) {
		std::stringstream result;
		result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
		this->log(result.str().c_str(), __FUNCTION__);
	}

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);
	mysql_free_result(sqlResult);
}
void HiveLib::setPlayerReb(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, bool _arrested, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[5];

	std::stringstream sqlQuery;
	sqlQuery << "INSERT INTO players (`playerid`, `name`, `cash`, `bankacc`, `reb_gear`) VALUES ( ";
	sqlQuery << "'" << _steamId << "', "; // Insert steamId
	sqlQuery << "?, "; // Insert name
	sqlQuery << "'" << _cash << "', "; // Insert cash
	sqlQuery << "'" << _bank << "', "; // Insert bank
	sqlQuery << "REPLACE(?, '\"', '') "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`reb_gear` = REPLACE(?, '\"', ''), ";
	//sqlQuery << "`civ_licenses` = REPLACE(?, '\"', ''), ";
	sqlQuery << "`arrested` = " << (_arrested ? 1 : 0) << ", ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
		return;
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// Zero out the sqlParam data structures
	memset(sqlParam, 0, sizeof(sqlParam));

	// insert bind player name
	long unsigned int insertPlayerNameLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 32;
	sqlParam[0].buffer = (char *)_playerName;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &insertPlayerNameLength;

	// insert bind gear
	long unsigned int InsertGearLength;
	sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[1].buffer_length = 4096;
	sqlParam[1].buffer = (char *)_gear;
	sqlParam[1].is_null = 0;
	sqlParam[1].length = &InsertGearLength;

	// update bind player name
	long unsigned int updatePlayerNameLength;
	sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[2].buffer_length = 32;
	sqlParam[2].buffer = (char *)_playerName;
	sqlParam[2].is_null = 0;
	sqlParam[2].length = &updatePlayerNameLength;

	// update bind gear
	long unsigned int updateGearLength;
	sqlParam[3].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[3].buffer_length = 4096;
	sqlParam[3].buffer = (char *)_gear;
	sqlParam[3].is_null = 0;
	sqlParam[3].length = &updateGearLength;

	//// update bind licenses
	//long unsigned int updateLicensesLength;
	//sqlParam[4].buffer_type = MYSQL_TYPE_STRING;
	//sqlParam[4].buffer_length = 4096;
	//sqlParam[4].buffer = (char *)_licenses;
	//sqlParam[4].is_null = 0;
	//sqlParam[4].length = &updateLicensesLength;

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	insertPlayerNameLength = strlen(_playerName);
	InsertGearLength = strlen(_gear);
	updatePlayerNameLength = strlen(_playerName);
	updateGearLength = strlen(_gear);
	//updateLicensesLength = strlen(_licenses);

	// Request meta data information
	MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

	// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
	my_bool aBool = 1;
	mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// success :)
	if (this->debugLogResult) {
		std::stringstream result;
		result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
		this->log(result.str().c_str(), __FUNCTION__);
	}

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);
	mysql_free_result(sqlResult);
}

// Get vehicle
std::string HiveLib::getVehicle(__int64 _steamId, int _vehicleId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "`id`, ";
	sqlQuery << "`side`, ";
	sqlQuery << "`classname`, ";
	sqlQuery << "`type`, ";
	sqlQuery << "`pid`, ";
	sqlQuery << "`alive`, ";
	sqlQuery << "`active`, ";
	sqlQuery << "`plate`, ";
	sqlQuery << "`color`, ";
	sqlQuery << "REPLACE(`inventory`, '\"', '') ";
	sqlQuery << "FROM `vehicles` ";
	sqlQuery << "WHERE `id` = '" << _vehicleId << "' AND `pid` = '" << _steamId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;
		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		sqfRow.push_str(queryRow[2]);
		sqfRow.push_str(queryRow[3]);
		sqfRow.push_str(queryRow[4]);
		sqfRow.push_str(queryRow[5]);
		sqfRow.push_str(queryRow[6]);
		sqfRow.push_str(queryRow[7]);
		sqfRow.push_str(queryRow[8]);
		sqfRow.push_str(queryRow[9]);
		returnString = sqfRow.toArray();
		if (this->debugLogResult) {
			this->log(returnString.c_str(), __FUNCTION__);
		}
	}

	mysql_free_result(queryResult);

	return returnString;
}
std::string HiveLib::getVehicles(__int64 _steamId, const char *_side, const char *_type) {
	std::string vehicleString = "[]";
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[2], sqlResult[8];
	unsigned long sqlResultLength[8];
	my_bool sqlResultIsNull[8];

	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "`id`, `classname`, `pid`, `alive`, `active`, `plate`, `color`, REPLACE(`inventory`, '\"', '') ";
	sqlQuery << "FROM `vehicles` ";
	sqlQuery << "WHERE `pid` = '" << _steamId << "' ";
	sqlQuery << "AND `side` = ? ";
	sqlQuery << "AND `type` = ? ";
	sqlQuery << "AND `alive` = '1' ";
	sqlQuery << "AND `active` = '0' ";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return vehicleString;
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return vehicleString;
	}

	// Zero out the sqlParam and sqlResult data structures
	memset(sqlParam, 0, sizeof(sqlParam));
	memset(sqlResult, 0, sizeof(sqlResult));

	// Declare result variables
	char vehicleId[12];
	char vehicleClassname[32];
	char vehiclePid[32];
	short int vehicleAlive;
	short int vehicleActive;
	char vehiclePlate[20];
	char vehicleColor[20];
	char vehicleInventory[500];

	// SQL param
	long unsigned int sideLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 15;
	sqlParam[0].buffer = (char *)_side;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &sideLength;

	long unsigned int typeLength;
	sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[1].buffer_length = 15;
	sqlParam[1].buffer = (char *)_type;
	sqlParam[1].is_null = 0;
	sqlParam[1].length = &typeLength;

	// SQL result
	sqlResult[0].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[0].buffer_length = sizeof(vehicleId);
	sqlResult[0].buffer = (void *)&vehicleId;
	sqlResult[0].is_null = &sqlResultIsNull[0];
	sqlResult[0].length = &sqlResultLength[0];

	sqlResult[1].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[1].buffer_length = sizeof(vehicleClassname);
	sqlResult[1].buffer = (void *)&vehicleClassname;
	sqlResult[1].is_null = &sqlResultIsNull[1];
	sqlResult[1].length = &sqlResultLength[1];

	sqlResult[2].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[2].buffer_length = sizeof(vehiclePid);
	sqlResult[2].buffer = (void *)&vehiclePid;
	sqlResult[2].is_null = &sqlResultIsNull[2];
	sqlResult[2].length = &sqlResultLength[2];

	sqlResult[3].buffer_type = MYSQL_TYPE_TINY;
	sqlResult[3].buffer = (void *)&vehicleAlive;
	sqlResult[3].is_null = &sqlResultIsNull[3];
	sqlResult[3].length = &sqlResultLength[3];

	sqlResult[4].buffer_type = MYSQL_TYPE_TINY;
	sqlResult[4].buffer = (void *)&vehicleActive;
	sqlResult[4].is_null = &sqlResultIsNull[4];
	sqlResult[4].length = &sqlResultLength[4];

	sqlResult[5].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[5].buffer_length = sizeof(vehiclePlate);
	sqlResult[5].buffer = (void *)&vehiclePlate;
	sqlResult[5].is_null = &sqlResultIsNull[5];
	sqlResult[5].length = &sqlResultLength[5];

	sqlResult[6].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[6].buffer_length = sizeof(vehicleColor);
	sqlResult[6].buffer = (void *)&vehicleColor;
	sqlResult[6].is_null = &sqlResultIsNull[6];
	sqlResult[6].length = &sqlResultLength[6];

	sqlResult[7].buffer_type = MYSQL_TYPE_STRING;
	sqlResult[7].buffer_length = sizeof(vehicleInventory);
	sqlResult[7].buffer = (void *)&vehicleInventory;
	sqlResult[7].is_null = &sqlResultIsNull[7];
	sqlResult[7].length = &sqlResultLength[7];

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed", __FUNCTION__, sqlStatement);
		return vehicleString;
	}

	sideLength = strlen(_side);
	typeLength = strlen(_type);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed", __FUNCTION__, sqlStatement);
		return vehicleString;
	}

	// Bind result to statement
	if (mysql_stmt_bind_result(sqlStatement, sqlResult) != 0) {
		this->log("mysql_stmt_bind_result() failed", __FUNCTION__, sqlStatement);
		return vehicleString;
	}

	if (mysql_stmt_store_result(sqlStatement) != 0) {
		this->log("mysql_stmt_store_result() failed", __FUNCTION__, sqlStatement);
		return vehicleString;
	}

	SQF sqfVehicles;
	while (!mysql_stmt_fetch(sqlStatement)) {
		SQF sqfVehicle;
		sqfVehicle.push_str(vehicleId);
		sqfVehicle.push_str(vehicleClassname);
		sqfVehicle.push_str(vehiclePid);
		sqfVehicle.push_str(vehicleAlive > 0 ? "1" : "0");
		sqfVehicle.push_str(vehicleActive > 0 ? "1" : "0");
		sqfVehicle.push_str(vehiclePlate);
		sqfVehicle.push_str(vehicleColor);
		sqfVehicle.push_str(vehicleInventory);

		sqfVehicles.push_array((char *)sqfVehicle.toArray().c_str());
	}
	vehicleString = sqfVehicles.toArray();

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);

	return vehicleString;
}
std::string HiveLib::insertVehicle(__int64 _steamId, char *_side, char *_type, char *_className, int _color, int _plate) {
	std::stringstream returnString;
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[3];

	std::stringstream sqlQuery;
	sqlQuery << "INSERT INTO `vehicles` (`side`, `classname`, `type`, `pid`, `alive`, `active`, `inventory`, `color`, `plate`) VALUES ( ";
	sqlQuery << "?, "; // side
	sqlQuery << "?, "; // classname
	sqlQuery << "?, "; // type
	sqlQuery << "'" << _steamId << "', "; // steamId
	sqlQuery << "'1', "; // alive
	sqlQuery << "'1', "; // active
	sqlQuery << "'[]', "; // inventory
	sqlQuery << "'" << _color << "', "; // color
	sqlQuery << "'" << _plate << "' "; // plate
	sqlQuery << ");";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return std::string("");
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return std::string("");
	}

	// Zero out the sqlParam data structures
	memset(sqlParam, 0, sizeof(sqlParam));

	// insert side
	long unsigned int insertSideLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 32;
	sqlParam[0].buffer = _side;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &insertSideLength;

	// insert classname
	long unsigned int insertClassNameLength;
	sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[1].buffer_length = 32;
	sqlParam[1].buffer = _className;
	sqlParam[1].is_null = 0;
	sqlParam[1].length = &insertClassNameLength;

	// insert type
	long unsigned int insertTypeLength;
	sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[2].buffer_length = 4096;
	sqlParam[2].buffer = _type;
	sqlParam[2].is_null = 0;
	sqlParam[2].length = &insertTypeLength;

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed: ", __FUNCTION__, sqlStatement);
		return std::string("");
	}

	insertSideLength = strlen(_side);
	insertTypeLength = strlen(_type);
	insertClassNameLength = strlen(_className);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed: ", __FUNCTION__, sqlStatement);
		return std::string("");
	}

	// success :)
	if (this->debugLogResult) {
		std::stringstream result;
		result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
		this->log(result.str().c_str(), __FUNCTION__);
	}

	my_ulonglong vehicleId = mysql_stmt_insert_id(sqlStatement);
	returnString << "[" << vehicleId << "]";

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);

	return returnString.str();
}
void HiveLib::setVehicleActive(__int64 _steamId, int _id, bool _active) {
	std::stringstream sqlQuery;
	sqlQuery << "UPDATE `vehicles` ";
	sqlQuery << "SET `active` = '" << (_active ? 1 : 0) << "' ";
	sqlQuery << "WHERE `pid` = '" << _steamId << "' ";
	sqlQuery << "AND `id` = '" << _id << "' ";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return;
	}
}
void HiveLib::setVehicleAlive(__int64 _steamId, int _id, bool _alive) {
	std::stringstream sqlQuery;
	sqlQuery << "UPDATE `vehicles` ";
	sqlQuery << "SET `alive` = '" << (_alive ? 1 : 0) << "' ";
	sqlQuery << "WHERE `pid` = '" << _steamId << "' ";
	sqlQuery << "AND `id` = '" << _id << "' ";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return;
	}
}

void HiveLib::resetVehicles() {
	std::stringstream sqlQuery;
	sqlQuery << "CALL resetLifeVehicles();";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return;
	}
}

std::string HiveLib::getHouse(int _houseObjectId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "h.`i_level`, ";
	sqlQuery << "(SELECT MAX(`i_level`) FROM `houselevel` WHERE `i_class_id` = h.`i_class_id`), ";
	sqlQuery << "IFNULL((SELECT SUM(`i_level`) FROM `house2upgrade` WHERE `i_house_id` = h.`id`), 0), ";
	sqlQuery << "hl.`i_upgrademax`, ";
	sqlQuery << "hp.`i_price`, ";
	sqlQuery << "hp.`i_tax`, ";
	sqlQuery << "hl.`i_cost`, ";
	sqlQuery << "hl.`i_upgradeprice`, ";
	sqlQuery << "h.`s_playeruid` ";
	sqlQuery << "FROM ";
	sqlQuery << "`house` h ";
	sqlQuery << "JOIN `houselevel` hl ON hl.`i_class_id` = h.`i_class_id` AND hl.`i_level` = h.`i_level` ";
	sqlQuery << "JOIN `houseprice` hp ON hp.`i_class_id` = h.`i_class_id` ";
	sqlQuery << "WHERE ";
	sqlQuery << " `i_object_id` = '" << _houseObjectId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_HOUSING);

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;

		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		sqfRow.push_str(queryRow[2]);
		sqfRow.push_str(queryRow[3]);
		sqfRow.push_str(queryRow[4]);
		sqfRow.push_str(queryRow[5]);
		sqfRow.push_str(queryRow[6]);
		sqfRow.push_str(queryRow[7]);
		sqfRow.push_str(queryRow[8]);

		if (queryRow[8] != "0") {
			SQF sqfRowUpgrades;
			std::stringstream sqlQueryUpgrades;;
			sqlQueryUpgrades << "SELECT ";
			sqlQueryUpgrades << "hu.`id`, ";
			sqlQueryUpgrades << "hu.`s_name`, ";
			sqlQueryUpgrades << "IFNULL((SELECT h2u.`i_level` FROM `house2upgrade` h2u INNER JOIN `house` h ON h.`id` = h2u.`i_house_id` WHERE h2u.`i_upgrade_id` = hu.`id` AND h.`id` = '" << _houseObjectId << "'),0) ";
			sqlQueryUpgrades << "FROM ";
			sqlQueryUpgrades << "`houseupgrade` hu;";
			if (this->debugLogQuery) {
				this->log(sqlQueryUpgrades.str().c_str(), __FUNCTION__);
			}

			// Init statement
			int queryStateUpgrades = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQueryUpgrades.str().c_str());
			if (queryStateUpgrades != 0) {
				this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
				return returnString;
			}

			MYSQL_RES *queryUpgradesResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
			MYSQL_ROW queryUpgradeRow;

			while ((queryUpgradeRow = mysql_fetch_row(queryUpgradesResult)) != NULL) {
				SQF sqfRowUpgrade;
				sqfRowUpgrade.push_str(queryUpgradeRow[0]);
				sqfRowUpgrade.push_str(queryUpgradeRow[1]);
				sqfRowUpgrade.push_str(queryUpgradeRow[2]);

				sqfRowUpgrades.push_array((char *)sqfRowUpgrade.toArray().c_str());
			}
			
			sqfRow.push_array((char *)sqfRowUpgrades.toArray().c_str());
		}
		else {
			sqfRow.push_array("[]");
		}
		
		returnString = sqfRow.toArray();
		if (this->debugLogResult) {
			this->log(returnString.c_str(), __FUNCTION__);
		}
	}

	mysql_free_result(queryResult);

	return returnString;
}
std::string HiveLib::getHouses(__int64 _steamId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "h.`id`, ";
	sqlQuery << "h.`i_object_id`, ";
	sqlQuery << "h.`s_position`, ";
	sqlQuery << "REPLACE(h.`s_inventory`, '\"', '') ";
	sqlQuery << "FROM ";
	sqlQuery << "`house` h ";
	//sqlQuery << "JOIN `houselevel` hl ON hl.`i_class_id` = h.`i_class_id` AND hl.`i_level` = h.`i_level` ";
	//sqlQuery << "JOIN `houseprice` hp ON hp.`i_class_id` = h.`i_class_id` ";
	sqlQuery << "WHERE ";
	sqlQuery << " h.`s_playeruid` = '" << _steamId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_HOUSING);

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
	MYSQL_ROW queryRow;

	SQF sqfHouses;
	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;
		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		sqfRow.push_str(queryRow[2]);
		sqfRow.push_str(queryRow[3]);
		sqfHouses.push_array((char *)sqfRow.toArray().c_str());
	}
	returnString = sqfHouses.toArray();
	if (this->debugLogResult) {
		this->log(returnString.c_str(), __FUNCTION__);
	}

	mysql_free_result(queryResult);

	return returnString;
}

std::string HiveLib::buyHouse(__int64 _steamId, int _houseObjectId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_HOUSING);

	sqlQuery << "SET @SteamId = '" << _steamId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SET @HouseObjectId = '" << _houseObjectId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "CALL `buyHouse`(@SteamId, @HouseObjectId, @returnCode);";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SELECT ";
	sqlQuery << "@returnCode, ";
	sqlQuery << "`s_position` ";
	sqlQuery << "FROM ";
	sqlQuery << "`house` ";
	sqlQuery << "WHERE ";
	sqlQuery << "`i_object_id` = @HouseObjectId";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;
		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		returnString = sqfRow.toArray();
	}
	if (this->debugLogResult) {
		this->log(returnString.c_str(), __FUNCTION__);
	}

	mysql_free_result(queryResult);

	return returnString;
}
std::string HiveLib::sellHouse(__int64 _steamId, int _houseObjectId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_HOUSING);

	sqlQuery << "SET @SteamId = '" << _steamId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SET @HouseObjectId = '" << _houseObjectId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "CALL `sellHouse`(@SteamId, @HouseObjectId, @returnCode);";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SELECT ";
	sqlQuery << "@returnCode, ";
	sqlQuery << "`s_position` ";
	sqlQuery << "FROM ";
	sqlQuery << "`house` ";
	sqlQuery << "WHERE ";
	sqlQuery << "`i_object_id` = @HouseObjectId";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;
		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		returnString = sqfRow.toArray();
	}
	if (this->debugLogResult) {
		this->log(returnString.c_str(), __FUNCTION__);
	}

	mysql_free_result(queryResult);

	return returnString;
}
std::string HiveLib::upgradeHouse(__int64 _steamId, int _houseObjectId) {
	std::string returnString = "[]";
	std::stringstream sqlQuery;

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_HOUSING);

	sqlQuery << "SET @SteamId = '" << _steamId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SET @HouseObjectId = '" << _houseObjectId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "CALL `upgradeHouse`(@SteamId, @HouseObjectId, @returnCode, @houseLevel);";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	sqlQuery.str("");
	sqlQuery << "SELECT ";
	sqlQuery << "@returnCode, ";
	sqlQuery << "@houseLevel, ";
	sqlQuery << "`s_position` ";
	sqlQuery << "FROM ";
	sqlQuery << "`house` ";
	sqlQuery << "WHERE ";
	sqlQuery << "`i_object_id` = @HouseObjectId";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// Init statement
	queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING], sqlQuery.str().c_str());
	if (queryState != 0) {
		this->log("mysql_query() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
		return returnString;
	}

	MYSQL_RES *queryResult = mysql_store_result(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_HOUSING]);
	MYSQL_ROW queryRow;

	while ((queryRow = mysql_fetch_row(queryResult)) != NULL) {
		SQF sqfRow;
		sqfRow.push_str(queryRow[0]);
		sqfRow.push_str(queryRow[1]);
		sqfRow.push_str(queryRow[2]);
		returnString = sqfRow.toArray();
	}
	if (this->debugLogResult) {
		this->log(returnString.c_str(), __FUNCTION__);
	}

	mysql_free_result(queryResult);

	return returnString;
}

void HiveLib::updateHouseInventory(int _houseObjectId, char *_inventory) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[1];

	std::stringstream sqlQuery;
	sqlQuery << "UPDATE `house` SET ";
	sqlQuery << "`s_inventory` = ? ";
	sqlQuery << "WHERE `i_object_id` = '" << _houseObjectId << "';";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	this->dbCheck(HIVELIB_MYSQL_CONNECTION_VEHICLE);

	// Init statement
	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	if (sqlStatement == NULL) {
		this->log("mysql_stmt_init() failed: ", __FUNCTION__, this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
		return;
	}

	// Prepare statement
	if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) != 0) {
		this->log("mysql_stmt_prepare() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// Zero out the sqlParam data structures
	memset(sqlParam, 0, sizeof(sqlParam));

	// insert side
	long unsigned int updateInventoryLength;
	sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
	sqlParam[0].buffer_length = 32;
	sqlParam[0].buffer = _inventory;
	sqlParam[0].is_null = 0;
	sqlParam[0].length = &updateInventoryLength;

	// Bind buffer to statement
	if (mysql_stmt_bind_param(sqlStatement, sqlParam) != 0) {
		this->log("mysql_stmt_bind_param() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	updateInventoryLength = strlen(_inventory);

	// Execute statement
	if (mysql_stmt_execute(sqlStatement) != 0) {
		this->log("mysql_stmt_execute() failed: ", __FUNCTION__, sqlStatement);
		return;
	}

	// success :)
	if (this->debugLogResult) {
		std::stringstream result;
		result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
		this->log(result.str().c_str(), __FUNCTION__);
	}

	mysql_stmt_free_result(sqlStatement);
	mysql_stmt_close(sqlStatement);
}