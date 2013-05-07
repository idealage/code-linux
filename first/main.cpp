#include <iostream>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <iconv.h>
#include <occi.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QPluginLoader>
#include <QtGui/QApplication>
#include <QtSql/qsqlerror.h>
#include <QtSql/QSqlDatabase>
#include <QtSql/qsqlquery.h>

#include <IceUtil/StringUtil.h>
#include "TiAutoArrayPtr.h"

using namespace std;
using namespace boost;
using namespace oracle;


void print()
{
	for (int i = 0; i < 10; i++)
	{
        cout << i << endl;
    }
}

std::string Utf8ToGbk(const char* szUtf8)
{
	//	先将UTF8转化为UNICODE
	int nLen = strlen(szUtf8) * 6;
	auto_arrayptr<wchar_t> wszU16(new wchar_t[nLen + 1]);
	setlocale(LC_ALL, "zh_CN.UTF-8");
	mbstowcs(wszU16.get(), szUtf8, nLen);

	//	再将UNICODE转化为GBK
	nLen = wcslen(wszU16.get()) * 3;
	auto_arrayptr<char> szGbk(new char[nLen + 1]);
	setlocale(LC_ALL, "zh_CN.GBK");
	wcstombs(szGbk.get(), wszU16.get(), nLen);
	return szGbk.get();
}

std::string GbkToUtf8(const char* szGbk)
{
	//	先将GBK转化为UNICODE
	int nLen = strlen(szGbk) * 6;
	auto_arrayptr<wchar_t> wszU16(new wchar_t[nLen + 1]);
	setlocale(LC_ALL, "zh_CN.GBK");
	mbstowcs(wszU16.get(), szGbk, nLen);

	//	再将UNICODE转化为UTF8
	nLen = wcslen(wszU16.get()) * 3;
	auto_arrayptr<char> szUtf8(new char[nLen + 1]);
	setlocale(LC_ALL, "zh_CN.UTF-8");
	wcstombs(szUtf8.get(), wszU16.get(), nLen);
	return szUtf8.get();
}

bool ODBC_CreateConnection(QSqlDatabase &db, const char *szName)
{
	string strDSN("KMJYDB"); // 直接使用DSN的方式，此DSN在 /etc/odbc.ini 中配置
	strDSN = "DRIVER={SQLServer};SERVER=192.168.0.197;DATABASE=kmjy;Port=1433;";

	db = QSqlDatabase::addDatabase("QODBC", szName);
	db.setDatabaseName(strDSN.c_str());
	db.setUserName("test");		// 即使dsn中已设置UID和PASSWD，仍需要执行setUserName和setPassword
	db.setPassword("123456");	// 即使dsn中已设置UID和PASSWD，仍需要执行setUserName和setPassword
	return db.open();
}

bool CheckODBC()
{
	// 有问题时可以使用以下方法试试
//  export ODBCINI=/etc/odbc.ini
//  export ODBCSYSINI=/etc
//  QCoreApplication::addLibraryPath("/sdk/qt-4.8.4/plugins/"); // 加载程序目录下的"sqldrivers"内的QT插件
	cout << "QT Dababase Available drivers:" << endl;
	QStringList drivers = QSqlDatabase::drivers();
	foreach(QString driver, drivers)
		cout << "\t" << driver.toStdString() << endl;

	QSqlDatabase db;
	if(ODBC_CreateConnection(db, "KMJYDB"))
	{
		QStringList tables = db.tables();
		cout << "ODBC query table size: " << tables.size() << endl;
        foreach(QString table, tables)
            cout << "\t" << table.toStdString() << endl;
	}
	else
	{
		QString qs1 = db.lastError().text();
		string s1(qs1.toStdString());
		cout << "connect ODBC database error: " << GbkToUtf8(s1.c_str()) << endl;
	}

	return db.isValid();
}

int main()
{
	// 编码转换测试
	string s1("123>编码转换测试=ABC");
	string s2 = Utf8ToGbk (s1.c_str());
	string s3 = GbkToUtf8 (s2.c_str());
	cout << "GBK:" << s1 << ", UTF8:" << s2 << ", GBK:" << s3 << endl;

    // boost测试
    string strError;
    shared_ptr<int> intPtr(new int(100));
    cout << "Hello world!, " << *intPtr << endl;

 	boost::thread thr1(boost::bind(&print));

	// ICE普通测试
	string s5 = IceUtilInternal::toLower("AbCdEf");
    cout << "s1:" << s5 << endl;

    // Qt ODBC 测试
    CheckODBC();

    // oracle测试
	try {
        string connstr = "192.168.0.198:1521/orcl";
        occi::Environment *env = occi::Environment::createEnvironment("UTF8", "UTF8");
        occi::Connection *conn = env->createConnection("acct", "123456", connstr);

        if(conn == NULL)
        {
            cout << "access oracle failed..." << endl;
            return 0;
        }

        occi::Statement *st = conn->createStatement();
        occi::ResultSet *pResultSet = st->executeQuery("select * from N_NOTIFY_PRODUCT order by product_id");

        if (pResultSet != NULL)
        {
            while (pResultSet->next() != occi::ResultSet::END_OF_FETCH)
            {
                int nProductId = pResultSet->getInt(2);
                string strName = pResultSet->getString(3);
                cout << "数据ID:" << nProductId << ", 名称:" << strName << endl;
            }
        }

        env->terminateConnection(conn);
        occi::Environment::terminateEnvironment(env);
	}
	catch(occi::SQLException &ex) {
		strError = ex.getMessage();
	}

    // end
 	char key;
 	cin >> key;
    return 0;
}
