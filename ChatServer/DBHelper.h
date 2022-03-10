#pragma once

#include <mysql/jdbc.h>

#include <string>

using namespace sql;

using std::string;

enum CreateAccountWebResult
{
	SUCCESS,
	ACCOUNT_ALREADY_EXISTS,
	INVALID_PASSWORD,
	INTERNAL_SERVER_ERROR,
	VALID_PASSWORD,
	NO_CONNECTION,
	ACCOUNT_DOES_NOT_EXIST
};

class DBHelper
{
public:
	DBHelper(void);

	void Connect(const string& hostname, const string& username, const string& password);
	bool IsConnected(void);

	// SELECT = sql::Statement::executeQuery()
	// UPDATE = and sql::Statement::executeUpdate()
	// INSERT = sql::Statement::execute()

	CreateAccountWebResult CreateAccount(const string& email, const string& password);
	CreateAccountWebResult AuthenticateAccount(const string& email, const string& password);
	std::string GetAccountCreationDate(const string& email);

private:
	void GeneratePreparedStatements(void);

	sql::PreparedStatement* g_GetEmails;
	sql::PreparedStatement* g_InsertWebAuth;
	mysql::MySQL_Driver* m_Driver;
	Connection* m_Connection;
	ResultSet* m_ResultSet;
	bool m_IsConnected;
	bool m_IsConnecting;
};