#include "opedb.h"
#include<QMessageBox>
#include<QDebug>

OpeDB::OpeDB(QObject *parent)
    : QObject{parent}
{
    m_db=QSqlDatabase::addDatabase("QSQLITE");

}

OpeDB &OpeDB::getInstance()
{
    static OpeDB instance;
    return instance;
}

void OpeDB::init()
{
    m_db.setHostName("locahost");
    m_db.setDatabaseName("E:\\QT\\NetworkDisk-main\\NetworkDisk\\TcpServer\\cloud.db");
    if(m_db.open())
    {
        QSqlQuery query;
        query.exec("select * from usrInfo");
        while(query.next())
        {
            QString data = QString ("%1,%2,%3,%4")
                               .arg(query.value(0).toString())
                               .arg(query.value(1).toString())
                               .arg(query.value(2).toString())
                               .arg(query.value(3).toString());
            //ui->textEdit->append(data);
            qDebug()<< data;

        }
    }
    else
    {
        QMessageBox::critical(NULL,"打开数据库","打开数据库失败");
    }
}



bool OpeDB::handleRegist(const char *name, const char *pwd)
{
    if (name==NULL||pwd==NULL)
    {
        //qDebug()<<"name | pwd is NULL";
        return false;
    }
    QString data=QString("insert into usrInfo(name, pwd) values(\'%1\',\'%2\')").arg(name).arg(pwd);
    //qDebug()<<data;
    QSqlQuery query;
    return query.exec(data);
}

bool OpeDB::handleLogin(const char *name, const char *pwd)
{
    if (name==NULL||pwd==NULL)
    {
        qDebug()<<"name | pwd is NULL";
        return false;
    }
    QString data=QString("select * from usrInfo where name=\'%1\' and pwd= \'%2\' and online=0").arg(name).arg(pwd);
    //qDebug()<<name;
    //qDebug()<<pwd;
    qDebug()<<data;
    QSqlQuery query;
    query.exec(data);
    if(query.next())
    {
        QString data=QString("update usrInfo set online =1 where name=\'%1\' and pwd= \'%2\'").arg(name).arg(pwd);
        //qDebug()<<data;
        QSqlQuery query;
        query.exec(data);
        return true;
    }
    else
    {
        return false;
    }
}

void OpeDB::handleOffline(const char *name)
{
    if (name==NULL)
    {
        qDebug()<<"name is NULL";
        return;
    }
    QString data=QString("update usrInfo set online=0 where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
}

QStringList OpeDB::handleAllOnline()
{
    QString data=QString("select name from usrInfo where online=1");
    QSqlQuery query;
    query.exec(data);
    QStringList result;
    result.clear();

    while(query.next())
    {
        result.append(query.value(0).toString());
    }
    return result;
}

int OpeDB::handleSearchUsr(const char *name)
{
    if(NULL==name){
        return -1;
    }
    QString data=QString("select online from usrInfo where name=\'%1\'").arg(name);
    QSqlQuery query;
    query.exec(data);
    if(query.next())
    {
        int ret = query.value(0).toInt();
        return ret;
        // if(ret ==1)
        // {
        //     return 1;
        // }
        // else
        // {
        //     return 0;
        // }
    }
    else
    {
        return -1;
    }
}

int OpeDB::handleAddFriend(const char *pername, const char *name)
{
    if(NULL==pername || NULL==name)
    {
        return -1; //操作无效
    }
    QString data=QString("select * from friend  where(id=(select id from usrInfo where name=\'%1\')and friendId=(select id from usrInfo where name=\'%2\'))"
                           "or(id=(select id from usrInfo where name=\'%3\')and friendId=(select id from usrInfo where name=\'%4\'))")
                       .arg(pername).arg(name).arg(name).arg(pername);
    // qDebug()<< data;
    QSqlQuery query;
    query.exec(data);
    // qDebug()<<query.next();
    if(query.next())
    {
        return 0;  //双方已是好友
    }
    else
    {
        data=QString("select online from usrInfo where name=\'%1\'").arg(pername);
        QSqlQuery query;
        query.exec(data);
        if(query.next())
        {
            int ret = query.value(0).toInt();
            return ret;
            if (1==ret)
            {
                return 1; //在线
            }
            else if(0==ret)
                return 2; // 不在线

        }
        else
        {
            return 3; // 不存在
        }
    }
}

bool OpeDB::handleAddFriendAgree(const char *addedName, const char *sourceName)
{
    if(NULL == addedName || NULL == sourceName)
    {
        qDebug() << "handleAddFriendAgree: name is NULL";
        return false;
    }

    int sourceUserId = -1, addedUserId = -1;
    // 查找用户对应id
    addedUserId = getIdByUserName(addedName);
    sourceUserId = getIdByUserName(sourceName);

    QString strQuery = QString("insert into friend values(%1, %2) ").arg(sourceUserId).arg(addedUserId);
    QSqlQuery query;

    qDebug() << "handleAddFriendAgree " << strQuery;

    return query.exec(strQuery);
}

int OpeDB::getIdByUserName(const char *name)
{
    if(NULL == name)
    {
        return -1;
    }
    QString strQuery = QString("select id from usrInfo where name = \'%1\' ").arg(name);
    QSqlQuery query;

    query.exec(strQuery);
    if(query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        return -1; // 不存在该用户
    }
}

QStringList OpeDB::handleFlushFriend(const char *name)
{
    QStringList strFriendList;
    strFriendList.clear();
    // qDebug()<<"名字为："<<name;
    if(name==NULL){
        return strFriendList;
    }

    // 获取请求方name对应的id
    QString strQuery = QString("select id from usrInfo where name = \'%1\' and online = 1 ").arg(name);
    QSqlQuery query;
    int iId = -1; // 请求方name对应的id
    query.exec(strQuery);
    if (query.next())
    {
        iId = query.value(0).toInt();
    }
    // qDebug()<<"ID为："<<iId;


    // QString data=QString("select name from usrInfo where online=1 and id=(select id from friend where friendId = (select id from usrInfo where name=\'%1\'))").arg(name);
    QString data = QString("select name from usrInfo "
                       "where id in "
                       "(select friendId from friend "
                       "where id = %1 "
                       "union "
                       "select id from friend "
                       "where friendId = %2)").arg(iId).arg(iId);

    query.exec(data);
    while(query.next())
    {
        strFriendList.append(query.value(0).toString());
    }


    // data=QString("select name from usrInfo where online=1 and id in(select friendId from friend where id in (select id from usrInfo where name=\'%1\'))").arg(name);
    // query.exec(data);
    // while(query.next())
    // {
    //     strFriendList.append(query.value(0).toString());
    // }

    // qDebug()<<strFriendList;

    return strFriendList;
}

bool OpeDB::handleDelFriend(const char *name, const char *friendName)
{
    if(NULL==name||NULL==friendName)
    {
        return false;
    }
    QString data=QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId =(select id from usrInfo where name=\'%2\')").arg(name).arg(friendName);
    QSqlQuery query;
    query.exec(data);

    data=QString("delete from friend where id=(select id from usrInfo where name=\'%1\') and friendId =(select id from usrInfo where name=\'%2\')").arg(friendName).arg(name);
    query.exec(data);

    return true;
}



