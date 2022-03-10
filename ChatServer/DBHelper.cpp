#include "DBHelper.h"
#include "bcrypt.h"
#include <time.h>
#include <chrono>

DBHelper::DBHelper(void)
	: m_IsConnected(false)
	, m_IsConnecting(false)
	, m_Connection(0)
	, m_Driver(0)
	, m_ResultSet(0)
{
}

bool DBHelper::IsConnected(void)
{
	return m_IsConnected;
}

CreateAccountWebResult DBHelper::CreateAccount(const string& email, const string& password)
{
	if (IsConnected())
	{
		sql::Statement* stmt = m_Connection->createStatement();
		sql::PreparedStatement* pStmt = m_Connection->prepareStatement("SELECT email FROM `web_auth` WHERE email = ?;");
		pStmt->setString(1, email);
		try
		{
			m_ResultSet = pStmt->executeQuery();
			//m_ResultSet = stmt->executeQuery("SELECT email FROM `web_auth` WHERE email = $1;");
		}
		catch (SQLException e)
		{
			printf("Failed to retrieved web_auth data!\n");
			return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
		}

		if (m_ResultSet->rowsCount() > 0)
		{
			printf("Account already exists with email!\n");
			return CreateAccountWebResult::ACCOUNT_ALREADY_EXISTS;;
		}

		//Can extract this into a helper function
		sql::Statement* stmt1 = m_Connection->createStatement();
		try
		{
			//m_ResultSet = stmt1->executeQuery("SELECT LAST_INSERT_ID();");
			m_ResultSet = stmt1->executeQuery("SELECT MAX(userID) AS largestID FROM web_auth;");
		}
		catch (SQLException e)
		{
			printf("Failed to retrieve last insert id!\n");
			return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
		}
		int lastId;
		if (m_ResultSet->next())
		{
			lastId = m_ResultSet->getInt(1);
		}

		printf("Successfully retrieved web_auth data!  Last ID was: %d\n", lastId);
		lastId++;

		// Insert new account into database
		//sql::Statement* stmt = m_Connection->createStatement();
		try
		{
			srand(time(NULL));
			// The insides of generateRandomSalt, using the function and putting the result into salt wouldn't send to the server.  doing
			// it here like this works tho... :(
			static const char alphanum[] =
				"0123456789"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz";
			std::string salt;
			salt.reserve(10);
			for (int i = 0; i < 10; i++)
			{
				salt += alphanum[rand() % sizeof(alphanum) - 1];
			}
			//std::string salt = "salt";
			std::string saltPass = salt + password;

			std::string hash = bcrypt::generateHash(saltPass);

			g_InsertWebAuth->setString(1, email);
			g_InsertWebAuth->setString(2, salt);
			g_InsertWebAuth->setString(3, hash);
			g_InsertWebAuth->setInt(4, lastId);

			//int result = stmt->executeUpdate(
				//"INSERT INTO web_auth(email, hashed_password, salt, userID) VALUES (?, ?, ?, ?");
			int result = g_InsertWebAuth->executeUpdate();
			if (result == 0)
			{
				printf("Failed to insert row..\n");
			}
			else
			{
				printf("Successfully inserted a row.\n");
			}
		}
		catch (SQLException e)
		{
			printf("Failed to insert account into web_auth!\n");
			return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
		}

		// TODO: Insert into the user table as well
		
		try
		{
			sql::PreparedStatement* stmt2 = m_Connection->prepareStatement("INSERT INTO user(last_login, creation_date) VALUES (now(), now());");
			stmt2->executeUpdate();

			//g_InsertUser->executeUpdate();
		}
		catch (SQLException e)
		{
			printf("Failed to insert user into user!\n");
			return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
		}


		return CreateAccountWebResult::SUCCESS;
	}
	printf("Not connected to the database rn!\n");
	return CreateAccountWebResult::NO_CONNECTION;
} //end of create account


CreateAccountWebResult DBHelper::AuthenticateAccount(const string& email, const string& password)
{
	if (IsConnected())
	{
		sql::Statement* stmt = m_Connection->createStatement();
		sql::PreparedStatement* pStmt = m_Connection->prepareStatement("SELECT email FROM `web_auth` WHERE email = ?;");
		SQLString hashedPasswordSQL;
		SQLString saltSQL;
		std::string hashedPassword;
		std::string salt;
		pStmt->setString(1, email);
		try
		{
			m_ResultSet = pStmt->executeQuery();
			//m_ResultSet = stmt->executeQuery("SELECT email FROM `web_auth` WHERE email = $1;");
		}
		catch (SQLException e)
		{
			printf("Failed to retrieved web_auth data!\n");
			return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
		}

		if (m_ResultSet->rowsCount() > 0)
		{
			//printf("Account already exists with email!\n");
			// Good, we want the account to exist

			// now we need to authenticate the account, by checking the password
			sql::PreparedStatement* pStmt2 = m_Connection->prepareStatement("SELECT hashed_password FROM `web_auth` WHERE email = ?;");
			pStmt2->setString(1, email);
			try
			{
				m_ResultSet = pStmt2->executeQuery();
			}
			catch (SQLException e)
			{
				printf("Failed to retrieve the hashed_password from web_auth!\n");
				return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
			}

			try
			{
				m_ResultSet->next();
				// Something with the salt as well
				hashedPasswordSQL = m_ResultSet->getString("hashed_password");
				hashedPassword = hashedPasswordSQL;
				// TODO: Remove this check, it's before un-hashing the password
				//if (password == hashedPassword)
				//{
				//	printf("Yo!  The password is good!\n");
				//	//return CreateAccountWebResult::VALID_PASSWORD;
				//}
				//else
				//{
				//	printf("password not good\n");
				//	//return CreateAccountWebResult::INVALID_PASSWORD;
				//}
			}
			catch (SQLException e)
			{
				printf("Failed to retrieve the hashed password.\n");
				return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
			}

			sql::PreparedStatement* pStmt3 = m_Connection->prepareStatement("SELECT salt FROM `web_auth` WHERE email = ?;");
			pStmt3->setString(1, email);
			try
			{
				m_ResultSet = pStmt3->executeQuery();
			}
			catch (SQLException e)
			{
				printf("Failed to retrieve the salt.\n");
				return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
			}

			try
			{
				// Get the salt and check the password using it
				m_ResultSet->next();
				saltSQL = m_ResultSet->getString("salt");
				salt = saltSQL;
				std::string saltPass = salt + password;
				if (bcrypt::validatePassword(saltPass, hashedPassword))
				{
					// and update the last login time
					try
					{
						// Need to get the id from web_auth first
						sql::PreparedStatement* pStmt4 = m_Connection->prepareStatement("SELECT id FROM web_auth WHERE email = ?;");
						pStmt4->setString(1, email);

						m_ResultSet = pStmt4->executeQuery();
						
					}
					catch (SQLException e)
					{
						printf("Unable to get id from web_auth.\n");
						return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
					}

					try
					{
						sql::PreparedStatement* pStmt5 = m_Connection->prepareStatement("UPDATE user SET last_login = now() WHERE id = ?;");
						m_ResultSet->next();
						int id = m_ResultSet->getInt("id");
						pStmt5->setInt(1, id);
						int result = pStmt5->executeUpdate();
						if (result > 0)
						{
							printf("Updated last_login to now()\n");
						}
					}
					catch (SQLException e)
					{
						printf("Unable to update last_login.\n");
						return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
					}

					printf("Yo!  The password is good!\n");
					return CreateAccountWebResult::VALID_PASSWORD;
				}
				else
				{
					printf("password not good\n");
					return CreateAccountWebResult::INVALID_PASSWORD;
				}
			}
			catch (SQLException e)
			{
				printf("Failed to retrieve the salt.\n");
				return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
			}
		}
		else
		{
			printf("Account did not exist.\n");
			return CreateAccountWebResult::ACCOUNT_DOES_NOT_EXIST;
		}
	}
	return CreateAccountWebResult::NO_CONNECTION;
} //end of authenticate account

void DBHelper::GeneratePreparedStatements(void)
{
	g_GetEmails = m_Connection->prepareStatement(
		"SELECT email FROM `web_auth` WHERE email = ?;");
	g_InsertWebAuth = m_Connection->prepareStatement(
		"INSERT INTO web_auth(email, salt, hashed_password, userID) VALUES (?, ?, ?, ?);");

}

void DBHelper::Connect(const string& hostname, const string& username, const string& password)
{
	if (m_IsConnecting)
		return;

	m_IsConnecting = true;
	try
	{
		m_Driver = mysql::get_driver_instance();
		m_Connection = m_Driver->connect(hostname, username, password);
		m_Connection->setSchema("authenticationservice");
	}
	catch (SQLException e)
	{
		printf("Failed to connect to database with error: %s\n", e.what());
		m_IsConnecting = false;
		return;
	}
	m_IsConnected = true;
	m_IsConnecting = false;

	GeneratePreparedStatements();

	printf("Successfully connected to database!\n");
}

std::string DBHelper::GetAccountCreationDate(const string& email)
{
	sql::PreparedStatement* pStmt = m_Connection->prepareStatement("SELECT id FROM web_auth WHERE email = ?;");
	pStmt->setString(1, email);

	try
	{
		m_ResultSet = pStmt->executeQuery();
	}
	catch (SQLException e)
	{
		printf("Unable to get id from web_auth, account.\n");
		return "";
	}
	
	try
	{
		m_ResultSet->next();
		int id = m_ResultSet->getInt("id");
		sql::PreparedStatement* pStmt1 = m_Connection->prepareStatement("SELECT creation_date FROM user WHERE id = ?;");
		pStmt1->setInt(1, id);
		m_ResultSet = pStmt1->executeQuery();		//whoops
	}
	catch (SQLException e)
	{
		printf("Unable to select creation_date from user where this id.\n");
		return "";
	}

	try
	{
		m_ResultSet->next();
		SQLString dateSQL = m_ResultSet->getString("creation_date");
		std::string date = dateSQL;
		return date;
	}
	catch (SQLException e)
	{
		printf("Unable to get creation date from result set,\n");
		return "";
	}
	return "";
}
