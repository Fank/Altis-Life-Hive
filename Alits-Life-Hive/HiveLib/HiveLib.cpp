#include "stdafx.h"
#include "HiveLib.hpp"
#include "SQF.hpp"

HiveLib::HiveLib() {
	// Disable debug
	this->debugLogQuery = false;
	this->debugLogResult = false;

	// Open configfile
	this->configuration = config4cpp::Configuration::create();
	try {
		this->configuration->parse("AltisLifeHive.cfg");
		this->dbConnection.Hostname = (char *)this->configuration->lookupString("", "Hostname");
		this->dbConnection.Username = (char *)this->configuration->lookupString("", "Username");
		this->dbConnection.Password = (char *)this->configuration->lookupString("", "Password");
		this->dbConnection.Database = (char *)this->configuration->lookupString("", "Database");
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

		this->MySQLStack.push_back(con);

		if (!this->connectDB(i)) {
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

bool HiveLib::connectDB(int _stackIndex) {
	if (!mysql_real_connect(this->MySQLStack[_stackIndex], this->dbConnection.Hostname, this->dbConnection.Username, this->dbConnection.Password, this->dbConnection.Database, this->dbConnection.Port, NULL, 0)) {
		return false;
	}
	else {
		return true;
	}
}

// Get player
std::string HiveLib::getPlayer(__int64 _steamId) {
	std::string playerString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "playerid, name, adminlevel, blacklist, donatorlvl, arrested, "; // Player
	sqlQuery << "cash, bankacc, "; // Money
	sqlQuery << "coplevel, REPLACE(cop_licenses, '\"', ''), REPLACE(cop_gear, '\"', ''), "; // COP
	sqlQuery << "REPLACE(civ_licenses, '\"', ''), REPLACE(civ_gear, '\"', ''), "; // Civ
	sqlQuery << "reblevel, REPLACE(reb_gear, '\"', '') "; // Reb
	sqlQuery << "FROM players ";
	sqlQuery << "WHERE playerid = '" << _steamId << "'";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYER])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_PLAYER);
			reconnectTry++;
		}
	}

	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYER], sqlQuery.str().c_str());
	if (queryState == 0) {
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
	}
	else {
		// error
		/*
		printf(mysql_error(connection));
		return 1;
		*/
	}

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
	sqlQuery << "? "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`cop_gear` = ?, ";
	sqlQuery << "`cop_licenses` = ?, ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);
			reconnectTry++;
		}
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
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

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
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

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute() failed: " << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
					if (this->debugLogResult) {
						std::stringstream result;
						result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
						this->log(result.str().c_str(), __FUNCTION__);
					}

					mysql_stmt_free_result(sqlStatement);
				}

				mysql_free_result(sqlResult);
			}
		}
		else {
			this->log("Could not prepare statement", __FUNCTION__);
		}
	}
	else {
		this->log("Could not initialize statement handler", __FUNCTION__);
	}
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
	sqlQuery << "? "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`civ_gear` = ?, ";
	sqlQuery << "`civ_licenses` = ?, ";
	sqlQuery << "`arrested` = " << (_arrested ? 1 : 0) << ", ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);
			reconnectTry++;
		}
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
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

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
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

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute() failed: " << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
					if (this->debugLogResult) {
						std::stringstream result;
						result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
						this->log(result.str().c_str(), __FUNCTION__);
					}

					mysql_stmt_free_result(sqlStatement);
				}

				mysql_free_result(sqlResult);
			}
		}
		else {
			this->log("Could not prepare statement", __FUNCTION__);
		}
	}
	else {
		this->log("Could not initialize statement handler", __FUNCTION__);
	}
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
	sqlQuery << "? "; // Insert gear
	sqlQuery << ") ON DUPLICATE KEY UPDATE ";
	sqlQuery << "`name` = ?, ";
	sqlQuery << "`cash` = " << _cash << ", ";
	sqlQuery << "`bankacc` = " << _bank << ", ";
	sqlQuery << "`reb_gear` = ?, ";
	//sqlQuery << "`civ_licenses` = ?, ";
	sqlQuery << "`arrested` = " << (_arrested ? 1 : 0) << ", ";
	sqlQuery << "`lastupdate` = NOW()";
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE);
			reconnectTry++;
		}
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
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

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
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

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute() failed: " << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
					if (this->debugLogResult) {
						std::stringstream result;
						result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
						this->log(result.str().c_str(), __FUNCTION__);
					}

					mysql_stmt_free_result(sqlStatement);
				}

				mysql_free_result(sqlResult);
			}
		}
		else {
			this->log("Could not prepare statement", __FUNCTION__);
		}
	}
	else {
		this->log("Could not initialize statement handler", __FUNCTION__);
	}
}

// Get vehicle
std::string HiveLib::getVehicles(__int64 _steamId, const char *_side, const char *_type) {
	std::string vehicleString = "[]";
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[2], sqlResult[8];

	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "`id`, `classname`, `pid`, `alive`, `active`, `plate`, `color`, `inventory` ";
	sqlQuery << "FROM `vehicles` ";
	sqlQuery << "WHERE `pid` = '" << _steamId << "' ";
	sqlQuery << "AND `side` = ? ";
	sqlQuery << "AND `type` = ?";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_VEHICLE);
			reconnectTry++;
		}
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
			memset(sqlParam, 0, sizeof(sqlParam));

			// Side
			long unsigned int sideLength;
			sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[0].buffer_length = 15;
			sqlParam[0].buffer = (char *)_side;
			sqlParam[0].is_null = 0;
			sqlParam[0].length = &sideLength;

			// Type
			long unsigned int typeLength;
			sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[1].buffer_length = 15;
			sqlParam[1].buffer = (char *)_type;
			sqlParam[1].is_null = 0;
			sqlParam[1].length = &typeLength;

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
				sideLength = strlen(_side);
				typeLength = strlen(_type);

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute() failed: " << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					memset(sqlResult, 0, sizeof(sqlResult));

					char *vehicleId = new char[12];
					char *vehicleClassname = new char[32];
					char *vehiclePid = new char[32];
					char *vehicleAlive = new char[1];
					char *vehicleActive = new char[1];
					char *vehiclePlate = new char[20];
					char *vehicleColor = new char[20];
					char *vehicleInventory = new char[500];

					sqlResult[0].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[0].buffer_length = sizeof(vehicleId);
					sqlResult[0].buffer = &vehicleId;

					sqlResult[1].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[1].buffer_length = sizeof(vehicleClassname);
					sqlResult[1].buffer = &vehicleClassname;

					sqlResult[2].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[2].buffer_length = sizeof(vehiclePid);
					sqlResult[2].buffer = &vehiclePid;

					sqlResult[3].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[3].buffer_length = sizeof(vehicleAlive);
					sqlResult[3].buffer = &vehicleAlive;

					sqlResult[4].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[4].buffer_length = sizeof(vehicleActive);
					sqlResult[4].buffer = &vehicleActive;

					sqlResult[5].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[5].buffer_length = sizeof(vehiclePlate);
					sqlResult[5].buffer = &vehiclePlate;

					sqlResult[6].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[6].buffer_length = sizeof(vehicleColor);
					sqlResult[6].buffer = &vehicleColor;

					sqlResult[7].buffer_type = MYSQL_TYPE_VARCHAR;
					sqlResult[7].buffer_length = sizeof(vehicleInventory);
					sqlResult[7].buffer = &vehicleInventory;

					if (mysql_stmt_bind_result(sqlStatement, sqlResult)) {
						std::stringstream errorMsg;
						errorMsg << "mysql_stmt_bind_result() failed: " << mysql_stmt_error(sqlStatement);
						this->log(errorMsg.str().c_str(), __FUNCTION__);
					}
					else {
						while (!mysql_stmt_fetch(sqlStatement)) {

						}
					}

					mysql_stmt_free_result(sqlStatement);
				}
			}
		}
		else {
			this->log("Could not prepare statement", __FUNCTION__);
		}
	}
	else {
		this->log("Could not initialize statement handler", __FUNCTION__);
	}

	return vehicleString;
}
void HiveLib::insertVehicle(__int64 _steamId, char *_side, char *_type, char *_className, int _color, int _plate) {
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
	sqlQuery << "'" << _plate << "'"; // plate
	sqlQuery << ";";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	// keep alive check
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_VEHICLE);
			reconnectTry++;
		}
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
			memset(sqlParam, 0, sizeof(sqlParam));

			// insert bind player name
			long unsigned int insertSideLength;
			sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[0].buffer_length = 32;
			sqlParam[0].buffer = _side;
			sqlParam[0].is_null = 0;
			sqlParam[0].length = &insertSideLength;

			// insert bind gear
			long unsigned int insertTypeLength;
			sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[1].buffer_length = 4096;
			sqlParam[1].buffer = _type;
			sqlParam[1].is_null = 0;
			sqlParam[1].length = &insertTypeLength;

			// update bind player name
			long unsigned int insertClassNameLength;
			sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[2].buffer_length = 32;
			sqlParam[2].buffer = _className;
			sqlParam[2].is_null = 0;
			sqlParam[2].length = &insertClassNameLength;

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
				insertSideLength = strlen(_side);
				insertTypeLength = strlen(_type);
				insertClassNameLength = strlen(_className);

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute() failed: " << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
					if (this->debugLogResult) {
						std::stringstream result;
						result << "affected rows " << mysql_stmt_affected_rows(sqlStatement);
						this->log(result.str().c_str(), __FUNCTION__);
					}

					mysql_stmt_free_result(sqlStatement);
				}
			}
		}
		else {
			this->log("Could not prepare statement", __FUNCTION__);
		}
	}
	else {
		this->log("Could not initialize statement handler", __FUNCTION__);
	}
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
	int reconnectTry = 0;
	while (mysql_ping(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE])) {
		if (reconnectTry == HIVELIB_MYSQL_CONNECTION_TRY) {
			exit(1);
		}
		else {
			this->log("Error, attempting reconnection...", __FUNCTION__);
			this->connectDB(HIVELIB_MYSQL_CONNECTION_VEHICLE);
			reconnectTry++;
		}
	}

	int queryState = mysql_query(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_VEHICLE], sqlQuery.str().c_str());
	if (queryState == 0) {

	}
	else {
		// error
		/*
		printf(mysql_error(connection));
		return 1;
		*/
	}
}