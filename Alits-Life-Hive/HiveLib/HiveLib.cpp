#include "stdafx.h"
#include "HiveLib.hpp"
#include "SQF.hpp"

HiveLib::HiveLib() {
	// Disable debug
	this->debugLogQuery = true;
	this->debugLogResult = true;

	// Init MySQL connections
	for (int i = 0; i < HIVELIB_MYSQL_CONNECTION_COUNT; i++) {
		MYSQL *con = mysql_init(NULL);

		if (con == NULL) {
			exit(1);
		}

		if (mysql_real_connect(con, "localhost", "root", "root", "arma3life", 3306, NULL, 0) == NULL) {
			exit(1);
		}

		this->MySQLStack.push_back(con);
	}
}
HiveLib::~HiveLib() {
	// Close all MySQL connections
	for (int i = 0; i < HIVELIB_MYSQL_CONNECTION_COUNT; i++) {
		mysql_close(this->MySQLStack[i]);
	}
}

// Get Player
std::string HiveLib::getPlayer(__int64 _steamId) {
	std::string playerString = "[]";
	std::stringstream sqlQuery;
	sqlQuery << "SELECT ";
	sqlQuery << "playerid, name, adminlevel, blacklist, donatorlvl, arrested, "; // Player
	sqlQuery << "cash, bankacc, "; // Money
	sqlQuery << "coplevel, REPLACE(cop_licenses, '\"', ''), REPLACE(cop_gear, '\"', ''), "; // COP
	sqlQuery << "REPLACE(civ_licenses, '\"', ''), REPLACE(civ_gear, '\"', ''), "; // Civ
	sqlQuery << "reblevel, reb_gear "; // Reb
	sqlQuery << "FROM players ";
	sqlQuery << "WHERE playerid = '" << _steamId << "'";
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str());
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
			playerString = playerRow.toArray();
			if (this->debugLogResult) {
				this->log(playerString.c_str());
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

void HiveLib::setPlayerCop(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[3];

	std::stringstream sqlQuery;
	sqlQuery << "UPDATE players SET ";
	sqlQuery << "name = ?, ";
	sqlQuery << "cash = " << _cash << ", ";
	sqlQuery << "bankacc = " << _bank << ", ";
	sqlQuery << "cop_gear = ?, ";
	sqlQuery << "cop_licenses = ? ";
	sqlQuery << "WHERE playerid = " << _steamId;
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
			memset(sqlParam, 0, sizeof(sqlParam));

			// bind player name
			long unsigned int playerNameLength;
			sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[0].buffer_length = 32;
			sqlParam[0].buffer = (char *)_playerName;
			sqlParam[0].is_null = 0;
			sqlParam[0].length = &playerNameLength;

			// bind gear
			long unsigned int gearLength;
			sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[1].buffer_length = 2048;
			sqlParam[1].buffer = (char *)_gear;
			sqlParam[1].is_null = 0;
			sqlParam[1].length = &gearLength;

			// bind licenses
			long unsigned int licensesLength;
			sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[2].buffer_length = 2048;
			sqlParam[2].buffer = (char *)_licenses;
			sqlParam[2].is_null = 0;
			sqlParam[2].length = &licensesLength;

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
				playerNameLength = strlen(_playerName);
				gearLength = strlen(_gear);
				licensesLength = strlen(_licenses);

				// Request meta data information
				MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

				// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
				my_bool aBool = 1;
				mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute(), 1 failed\n" << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
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
void HiveLib::setPlayerCiv(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, bool _arrested, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[3];

	std::stringstream sqlQuery;
	sqlQuery << "UPDATE players SET ";
	sqlQuery << "name = ?, ";
	sqlQuery << "cash = " << _cash << ", ";
	sqlQuery << "bankacc = " << _bank << ", ";
	sqlQuery << "civ_gear = ?, ";
	sqlQuery << "civ_licenses = ?, ";
	sqlQuery << "arrested = " << (_arrested ? 1 : 0) << " ";
	sqlQuery << "WHERE playerid = " << _steamId;
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
			memset(sqlParam, 0, sizeof(sqlParam));

			// bind player name
			long unsigned int playerNameLength;
			sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[0].buffer_length = 32;
			sqlParam[0].buffer = (char *)_playerName;
			sqlParam[0].is_null = 0;
			sqlParam[0].length = &playerNameLength;

			// bind gear
			long unsigned int gearLength;
			sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[1].buffer_length = 2048;
			sqlParam[1].buffer = (char *)_gear;
			sqlParam[1].is_null = 0;
			sqlParam[1].length = &gearLength;

			// bind licenses
			long unsigned int licensesLength;
			sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[2].buffer_length = 2048;
			sqlParam[2].buffer = (char *)_licenses;
			sqlParam[2].is_null = 0;
			sqlParam[2].length = &licensesLength;

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
				playerNameLength = strlen(_playerName);
				gearLength = strlen(_gear);
				licensesLength = strlen(_licenses);

				// Request meta data information
				MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

				// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
				my_bool aBool = 1;
				mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute(), 1 failed\n" << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
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
void HiveLib::setPlayerReb(__int64 _steamId, int _cash, int _bank, const char *_gear, const char *_licenses, bool _arrested, const char *_playerName) {
	MYSQL_STMT *sqlStatement;
	MYSQL_BIND sqlParam[3];

	std::stringstream sqlQuery;
	sqlQuery << "UPDATE players SET ";
	sqlQuery << "name = ?, ";
	sqlQuery << "cash = " << _cash << ", ";
	sqlQuery << "bankacc = " << _bank << ", ";
	sqlQuery << "reb_gear = ?, ";
	sqlQuery << "civ_licenses = ? ";
	sqlQuery << "arrested = " << (_arrested ? 1 : 0) << " ";
	sqlQuery << "WHERE playerid = " << _steamId;
	if (this->debugLogQuery) {
		this->log(sqlQuery.str().c_str(), __FUNCTION__);
	}

	sqlStatement = mysql_stmt_init(this->MySQLStack[HIVELIB_MYSQL_CONNECTION_PLAYERUPDATE]);
	if (sqlStatement != NULL) {
		if (mysql_stmt_prepare(sqlStatement, sqlQuery.str().c_str(), sqlQuery.str().size()) == 0) {
			memset(sqlParam, 0, sizeof(sqlParam));

			// bind player name
			long unsigned int playerNameLength;
			sqlParam[0].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[0].buffer_length = 32;
			sqlParam[0].buffer = (char *)_playerName;
			sqlParam[0].is_null = 0;
			sqlParam[0].length = &playerNameLength;

			// bind gear
			long unsigned int gearLength;
			sqlParam[1].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[1].buffer_length = 2048;
			sqlParam[1].buffer = (char *)_gear;
			sqlParam[1].is_null = 0;
			sqlParam[1].length = &gearLength;

			// bind licenses
			long unsigned int licensesLength;
			sqlParam[2].buffer_type = MYSQL_TYPE_STRING;
			sqlParam[2].buffer_length = 2048;
			sqlParam[2].buffer = (char *)_licenses;
			sqlParam[2].is_null = 0;
			sqlParam[2].length = &licensesLength;

			// bind to statement
			if (mysql_stmt_bind_param(sqlStatement, sqlParam)) {
				std::stringstream errorMsg;
				errorMsg << "mysql_stmt_bind_param() failed: " << mysql_stmt_error(sqlStatement);
				this->log(errorMsg.str().c_str(), __FUNCTION__);
			}
			else {
				playerNameLength = strlen(_playerName);
				gearLength = strlen(_gear);
				licensesLength = strlen(_licenses);

				// Request meta data information
				MYSQL_RES *sqlResult = mysql_stmt_result_metadata(sqlStatement);

				// Set STMT_ATTR_UPDATE_MAX_LENGTH attribute
				my_bool aBool = 1;
				mysql_stmt_attr_set(sqlStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &aBool);

				if (mysql_stmt_execute(sqlStatement)) {
					std::stringstream errorMsg;
					errorMsg << "mysql_stmt_execute(), 1 failed\n" << mysql_stmt_error(sqlStatement);
					this->log(errorMsg.str().c_str(), __FUNCTION__);
				}
				else {
					// success :)
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

void HiveLib::log(const char *_logMessage) {
	std::ofstream logFile;
	logFile.open("HiveLib.log", std::ios::out | std::ios::app);
	logFile << _logMessage << std::endl;
	logFile.close();
}

void HiveLib::log(const char *_logMessage, const char *_functionName) {
	std::stringstream logMessage;
	logMessage << _functionName << ": " << _logMessage;
	this->log(logMessage.str().c_str());
}