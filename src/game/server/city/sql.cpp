/* SQL class by Sushi */
#include "../gamecontext.h"

#include <engine/shared/config.h>

static LOCK sql_lock = 0;
class CGameContext *m_pGameServer;
CGameContext *GameServer() { return m_pGameServer; }

CSQL::CSQL(class CGameContext *pGameServer)
{
	if(sql_lock == 0)
		sql_lock = lock_create();

	m_pGameServer = pGameServer;
}

bool CSQL::connect()
{
	try 
	{
		// Create connection
		driver = get_driver_instance();
		char buf[256];
		str_format(buf, sizeof(buf), "tcp://%s:%d", g_Config.m_SvSqlIp, g_Config.m_SvSqlPort);
		connection = driver->connect(buf, g_Config.m_SvSqlUser, g_Config.m_SvSqlPw);
		
		// Create Statement
		statement = connection->createStatement();
		
		// Connect to specific database
		connection->setSchema(g_Config.m_SvSqlDatabase);
		dbg_msg("SQL", "SQL connection established");
		return true;
	} 
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: SQL connection failed");
		return false;
	}
}

void CSQL::disconnect()
{
	try
	{
		delete connection;
		dbg_msg("SQL", "SQL connection disconnected");
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: No SQL connection");
	}
}

// create Account
static void RegisterThread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		if(GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_UserID)
		{
			GameServer()->SendChatTarget(Data->m_ClientID, "You're already logged in!");
			delete Data;
			lock_unlock(sql_lock);
			return;
		}
		// Connect to database
		if(Data->m_SqlData->connect())
		{
			try
			{
				bool CanRegister = true;
				// check if already exists
				char buf[512];
				str_format(buf, sizeof(buf), "SELECT * FROM tw_Account WHERE Username='%s';", Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					GameServer()->SendChatTarget(Data->m_ClientID, "This username is already in use!");
					CanRegister = false;
				}
				str_format(buf, sizeof(buf), "SELECT * FROM tw_Account WHERE Nick='%s';", Data->m_Nick);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

				if(Data->m_SqlData->results->next())
				{
					GameServer()->SendChatTarget(Data->m_ClientID, "This nickname is already in use!");
					CanRegister = false;
				}
				if(CanRegister)
				{
					// create Account \o/
					str_format(buf, sizeof(buf), "INSERT INTO tw_Account(Username, Password, Nick, SPoint) VALUES ('%s', '%s', '%s', 300);", 
					Data->name, Data->pass, Data->m_Nick);
					
					Data->m_SqlData->statement->execute(buf);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "Account was created successfully.");
					GameServer()->SendChatTarget(Data->m_ClientID, "You may login now. (/login <user> <pass>)");
				}
				
				// delete statement
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not create Account (%s)", e.what());
			}
			
			// disconnect from database
			Data->m_SqlData->disconnect();
		}
	}
	
	delete Data;
	
	lock_unlock(sql_lock);
}

void CSQL::Register(const char* name, const char* pass, int m_ClientID, const char *pNick)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	str_copy(tmp->m_Nick, pNick, sizeof(tmp->m_Nick));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;
	
	void *register_thread = thread_init(RegisterThread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)register_thread);
#endif
}

// login stuff
static void LoginThread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		if(GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_UserID)
		{
			GameServer()->SendChatTarget(Data->m_ClientID, "You're already logged in!");
			delete Data;
			lock_unlock(sql_lock);
			return;
		}
		// Connect to database
		if(Data->m_SqlData->connect())
		{
			try
			{		
				// check if Account exists
				char buf[1024];
				str_format(buf, sizeof(buf), "SELECT * FROM tw_Account WHERE Username='%s';", Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					// check for right pw and get data
					str_format(buf, sizeof(buf), "SELECT * "
					"FROM tw_Account WHERE Username='%s' AND Password='%s';", Data->name, Data->pass);
					
					// create results
					Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
					
					// if match jump to it
					if(Data->m_SqlData->results->next())
					{
						// never use player directly!
						// finally save the result to SAccData \o/

						// check if Account allready is logged in
						for(int i = 0; i < MAX_CLIENTS; i++)
						{
							if(!GameServer()->m_apPlayers[i])
								continue;

							if(GameServer()->m_apPlayers[i]->m_AccData.m_UserID == Data->m_SqlData->results->getInt("UserID"))
							{
								dbg_msg("SQL", "Account '%s' already is logged in", Data->name);
								
								GameServer()->SendChatTarget(Data->m_ClientID, "This Account is already logged in.");
								
								// delete statement and results
								delete Data->m_SqlData->statement;
								delete Data->m_SqlData->results;
								
								// disconnect from database
								Data->m_SqlData->disconnect();
								
								// delete Data
								delete Data;
	
								// release lock
								lock_unlock(sql_lock);
								
								return;
							}
						}

						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_UserID = Data->m_SqlData->results->getInt("UserID");
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_SocialPoint = Data->m_SqlData->results->getInt("SPoint");
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_Level = Data->m_SqlData->results->getInt("Level");
						GameServer()->m_apPlayers[Data->m_ClientID]->m_AccData.m_Job = Data->m_SqlData->results->getInt("Job");
						
						GameServer()->SendChatTarget(Data->m_ClientID, "You are now logged in.");
					}
					else
					{	
						GameServer()->SendChatTarget(Data->m_ClientID, "The username/password you entered is wrong.");
					}
				}
				else
				{					
					GameServer()->SendChatTarget(Data->m_ClientID, "This Account does not exists.");
					GameServer()->SendChatTarget(Data->m_ClientID, "Please register first. (/register <user> <pass>)");
				}
				
				// delete statement and results
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not login Account");
			}
			
			// disconnect from database
			Data->m_SqlData->disconnect();
		}
	}
	
	delete Data;
	
	lock_unlock(sql_lock);
}

void CSQL::Login(const char* name, const char* pass, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;
	
	void *login_account_thread = thread_init(LoginThread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)login_account_thread);
#endif
}

// update stuff
static void UpdateThread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	// Connect to database
	if(Data->m_SqlData->connect())
	{
		try
		{
			char buf[1024];
			str_format(buf, sizeof(buf), "SELECT * FROM %s WHERE %s;", Data->m_aTable, Data->m_aWhere);
			Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
			if(Data->m_SqlData->results->next())
			{
				str_format(buf, sizeof(buf), 
				"UPDATE %s SET %s WHERE %s;", 
					Data->m_aTable, Data->m_aVar, 
					Data->m_aWhere);
				Data->m_SqlData->statement->execute(buf);

				// create results
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

				// jump to result
				Data->m_SqlData->results->next();
			}
			else
				dbg_msg("SQL", "Account seems to be deleted");
			
			// delete statement and results
			delete Data->m_SqlData->statement;
			delete Data->m_SqlData->results;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account");
		}
		
		// disconnect from database
		Data->m_SqlData->disconnect();
	}
	
	delete Data;
	
	lock_unlock(sql_lock);
}

void CSQL::Update(int CID, const char *pTable, const char *pVar, const char *pWhere)
{
	CSqlData *tmp = new CSqlData();
	tmp->m_ClientID = CID;
	tmp->m_AccData = GameServer()->m_apPlayers[CID]->m_AccData;
	str_copy(tmp->m_aTable, pTable, sizeof(tmp->m_aTable));
	str_copy(tmp->m_aVar, pTable, sizeof(tmp->m_aVar));
	str_copy(tmp->m_aWhere, pTable, sizeof(tmp->m_aWhere));
	tmp->m_SqlData = this;
	
	void *update_account_thread = thread_init(UpdateThread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)update_account_thread);
#endif
}