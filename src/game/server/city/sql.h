/* SQL Class by Sushi */

#include <engine/shared/protocol.h>

#include <mysql_connection.h>
	
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <game/server/city/accdata.h>

class CSQL
{
public:
	CSQL(class CGameContext *pGameServer);

	sql::Driver *driver;
	sql::Connection *connection;
	sql::Statement *statement;
	sql::ResultSet *results;
	
	bool connect();
	void disconnect();
	
	void Register(const char* name, const char* pass, int client_id, const char *pNick);
	void Login(const char* name, const char* pass, int client_id);
	void Update(int cid, const char *pTable, const char *pVar, const char *pWhere);
};

struct CSqlData
{
	CSQL *m_SqlData;
	char name[32];
	char pass[32];
	char m_Nick[32];
	int m_ClientID;
	SAccData m_AccData;

	char m_aTable[64];
	char m_aVar[64];
	char m_aWhere[128];
};