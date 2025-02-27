#include "mytcpsocket.h"
#include<QDebug>
#include<stdio.h>
#include"mytcpserver.h"
#include<QDir>
#include <QFileInfoList>


MyTcpSocket::MyTcpSocket(QObject *parent)
    : QTcpSocket{parent}
{
    m_bUpload=false;
    m_pTimer=new QTimer;

    connect(this,SIGNAL(readyRead()),this,SLOT(recvMsg()));
    connect(this,SIGNAL(disconnected()),this,SLOT(clientOffline()));


    connect(m_pTimer,SIGNAL(timeout()),this,SLOT(sendFileToClient()));
}

QString MyTcpSocket::getname()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);

    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();

    QString srcTmp;
    QString destTmp;
    for(int i =0;i<fileInfoList.size();i++)
    {
        qDebug()<<"fileName"<<fileInfoList[i].fileName();
        if(fileInfoList[i].isFile())
        {
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            QFile::copy(srcTmp,destTmp);
        }
        else if(fileInfoList[i].isDir())
        {
            if (QString(".")==fileInfoList[i].fileName()
                ||QString("..")==fileInfoList[i].fileName())
            {
                continue;
            }
            srcTmp=strSrcDir+'/'+fileInfoList[i].fileName();
            destTmp=strDestDir+'/'+fileInfoList[i].fileName();
            copyDir(srcTmp,destTmp);
        }
    }
}

void MyTcpSocket::recvMsg()
{
    if(!m_bUpload)
    {
        qDebug()<<this->bytesAvailable();
        uint uiPDULen=0;
        this->read((char*)&uiPDULen,sizeof(uint));
        uint uiMsgLen = uiPDULen-sizeof(PDU);
        //qDebug()<<uiMsgLen;
        PDU *pdu=mkPDU(uiMsgLen);
        this ->read((char*)pdu+sizeof(uint),uiPDULen-sizeof(uint));
        //qDebug()<<pdu->uiMsgType;
        //qDebug()<<(char*)(pdu->caMsg);
        //qDebug()<<*(pdu->caMsg);
        //qDebug()<<pdu->caData;

        char caName[32]={'\0'};
        char caPwd[32]={'\0'};
        strncpy(caName, pdu->caData,32);
        strncpy(caPwd, pdu->caData+32,32);

        switch(pdu->uiMsgType){
        case ENUM_MSG_TYPE_REGIST_REQUEST:
        {
            bool ret=OpeDB ::getInstance().handleRegist(caName, caPwd);
            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_REGIST_RESPOND;
            if(ret){
                strcpy(respdu->caData,REGIST_OK);
                QDir dir;
                dir.mkdir(QString("./%1").arg(caName));
            }
            else
            {
                strcpy(respdu->caData,REGIST_FAILED);
            }
            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_LOGIN_REQUEST:
        {
            bool ret=OpeDB ::getInstance().handleLogin(caName, caPwd);
            qDebug()<<ret;
            PDU* respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_LOGIN_RESPOND;
            if(ret){
                strcpy(respdu->caData,LOGIN_OK);
                m_strName=caName;
            }
            else
            {
                strcpy(respdu->caData,LOGIN_FAILED);
            }
            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
        {
            QStringList ret=OpeDB::getInstance().handleAllOnline();
            uint uiMsgLen=ret.size()*32;
            PDU *respdu=mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for(int i =0;i<ret.size();i++)
            {
                memcpy((char*)(respdu->caMsg)+i*32
                       , ret.at(i).toStdString().c_str()
                       ,ret.at(i).size());
            }
            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
        {
            int ret = OpeDB::getInstance().handleSearchUsr(pdu->caData);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if(ret==-1)
            {
                strcpy(respdu->caData,SEARCH_USR_NO);
            }
            else if(ret==1)
            {
                strcpy(respdu->caData,SEARCH_USR_ONLINE);
            }
            else if(ret==0)
            {
                strcpy(respdu->caData,SEARCH_USR_OFFLINE);
            }
            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32]={'\0'};
            char caName[32]={'\0'};
            strncpy(caPerName, pdu->caData,32);
            strncpy(caName, pdu->caData+32,32);
            int ret =OpeDB::getInstance().handleAddFriend(caPerName,caName);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
            if(ret==-1)
            {
                strcpy(respdu->caData,UNKONW_ERROR);
                this->write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if(ret==0)
            {
                // qDebug()<<EXISTED_FRIEND;
                strcpy(respdu->caData,EXISTED_FRIEND);
                this->write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if(ret==1)
            {
                MyTcpServer::getInstance().resend(caPerName,pdu);


            }
            else if(ret==2)
            {
                strcpy(respdu->caData,ADD_FRIEND_OFFLINE);
                this->write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if(ret==3)
            {
                strcpy(respdu->caData,ADD_FRIEND_NO_EXIST);
                this->write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }

            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_AGREE:
        {
            char addedName[32] = {'\0'};
            char sourceName[32] = {'\0'};
            // 拷贝读取的信息
            strncpy(addedName, pdu -> caData, 32);
            strncpy(sourceName, pdu -> caData + 32, 32);

            // 将新的好友关系信息写入数据库
            OpeDB::getInstance().handleAddFriendAgree(addedName, sourceName);

            // 服务器需要转发给发送好友请求方其被同意的消息
            MyTcpServer::getInstance().resend(sourceName, pdu);
            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char sourceName[32] = {'\0'};
            // 拷贝读取的信息
            strncpy(sourceName, pdu -> caData + 32, 32);
            // 服务器需要转发给发送好友请求方其被拒绝的消息
            MyTcpServer::getInstance().resend(sourceName, pdu);
        }
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        {
            char caName[32]={'\0'};
            strncpy(caName,pdu->caData,32);
            QStringList ret=OpeDB::getInstance().handleFlushFriend(caName);
            uint uiMsgLen=ret.size()*32;
            PDU*respdu=mkPDU(uiMsgLen);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for (int i=0;i<ret.size();i++)
            {
                memcpy((char*)(respdu->caMsg)+i*32
                       ,ret.at(i).toStdString().c_str(),ret.at(i).size());
            }

            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caSelName[32]={'\0'};
            char caFriendName[32]={'\0'};
            strncpy(caSelName, pdu->caData,32);
            strncpy(caFriendName, pdu->caData+32,32);
            OpeDB::getInstance().handleDelFriend(caSelName,caFriendName);

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData,DEL_FRIEND_OK);
            this->write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            MyTcpServer::getInstance().resend(caFriendName, pdu);

            break;
        }
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            char caPerName[32]={'\0'};
            memcpy(caPerName,pdu->caData+32,32);
            MyTcpServer::getInstance().resend(caPerName,pdu);
            break;
        }
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            char caName[32]={'\0'};
            strncpy(caName,pdu->caData,32);
            QStringList onlineFriend=OpeDB::getInstance().handleFlushFriend(caName);
            QString tmp;
            for (int i=0;i<onlineFriend.size();i++)
            {
                tmp=onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmp.toStdString().c_str(),pdu);
            }
            break;
        }
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
        {
            QDir dir;
            QString strCurPath =QString("%1").arg((char*)(pdu->caMsg));
            bool ret = dir.exists(strCurPath);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType= ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
            if(ret)  //当前目录存在
            {
                char caNewDir[32]={'\0'};
                memcpy(caNewDir,pdu->caData+32,32);
                QString strNewPath =strCurPath+"/"+caNewDir;
                qDebug()<<strNewPath;
                ret =dir.exists(strNewPath);
                if(ret) //创建的文件名已存在
                {
                    strcpy(respdu->caData,FILE_NAME_EXIST);
                }
                else //创建的文件名不存在
                {
                    dir.mkdir(strNewPath);

                    strcpy(respdu->caData,CREATE_DIR_OK);
                }
            }
            else  //当前目录不存在
            {

                strcpy(respdu->caData,DIR_NO_EXIST);
            }
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        {
            char *pCurPath =new char[pdu->uiMsgLen];
            memcpy(pCurPath,pdu->caMsg,pdu->uiMsgLen);
            QDir dir(pCurPath);
            QFileInfoList fileInfoList = dir.entryInfoList();
            int iFileCount = fileInfoList.size();
            PDU *respdu =mkPDU(sizeof(FileInfo)*iFileCount);
            respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
            FileInfo *pFileInfo = NULL;
            QString  strFileName;
            for(int i =0;i<fileInfoList.size();i++)
            {

                pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                strFileName=fileInfoList[i].fileName();

                memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());
                if(fileInfoList[i].isDir())
                {
                    pFileInfo->iFileType=0;
                }
                else if(fileInfoList[i].isFile())
                {
                    pFileInfo->iFileType=1;
                }

            }
            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_DEL_DIR_REQUEST:
        {
            char caName[32]={'\0'};
            strcpy(caName,pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath=QString("%1/%2").arg(pPath).arg(caName);
            qDebug()<<strPath;

            QFileInfo FileInfo(strPath);
            bool ret =true;
            if(FileInfo.isDir())
            {
                QDir dir;
                dir.setPath(strPath);
                dir.removeRecursively();
            }
            else if(FileInfo.isFile()) //常规文件
            {
                ret=false;
            }
            PDU *respdu=NULL;
            if(ret)
            {
                respdu=mkPDU(strlen(DEL_DIR_OK)+1);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_OK,strlen(DEL_DIR_OK));
            }
            else
            {
                respdu=mkPDU(strlen(DEL_DIR_FALSE)+1);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                memcpy(respdu->caData,DEL_DIR_FALSE,strlen(DEL_DIR_FALSE));
            }


            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
        {
            char caOldName[32] ={'\0'};
            char caNewName[32] ={'\0'};
            strncpy(caOldName,pdu->caData,32);
            strncpy(caNewName,pdu->caData+32,32);

            char *pPath= new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strOldPath=QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath=QString("%1/%2").arg(pPath).arg(caNewName);

            QDir dir;
            bool ret=dir.rename(strOldPath,strNewPath);
            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if(ret)
            {
                strcpy(respdu->caData,RENAME_FILE_OK);
            }
            else
            {
                strcpy(respdu->caData,RENAME_FILE_FAILURED);
            }

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
        {
            char caEnterName[32] ={'\0'};
            strncpy(caEnterName,pdu->caData,32);

            char *pPath= new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);

            QString strPath=QString("%1/%2").arg(pPath).arg(caEnterName);

            QFileInfo fileInfo(strPath);
            PDU *respdu=NULL;
            if(fileInfo.isDir())
            {
                QDir dir(strPath);
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount = fileInfoList.size();
                respdu =mkPDU(sizeof(FileInfo)*iFileCount);
                respdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;
                FileInfo *pFileInfo = NULL;
                QString  strFileName;
                for(int i =0;i<fileInfoList.size();i++)
                {

                    pFileInfo = (FileInfo*)(respdu->caMsg)+i;
                    strFileName=fileInfoList[i].fileName();

                    memcpy(pFileInfo->caFileName,strFileName.toStdString().c_str(),strFileName.size());
                    if(fileInfoList[i].isDir())
                    {
                        pFileInfo->iFileType=0;
                    }
                    else if(fileInfoList[i].isFile())
                    {
                        pFileInfo->iFileType=1;
                    }

                }
                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }
            else if(fileInfo.isFile())
            {
                respdu=mkPDU(0);
                respdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strcpy(respdu->caData,ENTER_DIR_FAILURED);

                write((char*)respdu,respdu->uiPDULen);
                free(respdu);
                respdu=NULL;
            }



            break;
        }
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        {
            char caFileName[32] ={'\0'};
            qint64 fileSize=0;
            sscanf(pdu->caData,"%s %lld",caFileName,&fileSize);

            char *pPath= new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);

            QString strPath=QString("%1/%2").arg(pPath).arg(caFileName);
            // qDebug()<<strPath;
            delete []pPath;
            pPath=NULL;

            m_file.setFileName(strPath);
            //以只写的方式打开文件，若文件不存在，则会自动创建文件
            if(m_file.open(QIODevice::WriteOnly))
            {
                m_bUpload=true;
                m_iTotal=fileSize;
                m_iRecved=0;
            }
            break;
        }
        case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
        {
            char caName[32]={'\0'};
            strcpy(caName,pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);
            QString strPath=QString("%1/%2").arg(pPath).arg(caName);
            qDebug()<<strPath;

            QFileInfo FileInfo(strPath);
            bool ret = false;
            if(FileInfo.isDir())
            {
                ret=false;
            }
            else if(FileInfo.isFile()) //常规文件
            {
                QDir dir;
                ret=dir.remove(strPath);
            }
            PDU *respdu=NULL;
            if(ret)
            {
                respdu=mkPDU(strlen(DEL_FILE_OK)+1);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData,DEL_FILE_OK,strlen(DEL_FILE_OK));
            }
            else
            {
                respdu=mkPDU(strlen(DEL_FILE_FALSE)+1);
                respdu->uiMsgType=ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                memcpy(respdu->caData,DEL_FILE_FALSE,strlen(DEL_FILE_FALSE));
            }


            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
            break;
        }
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        {
            char caFileName[32] ={'\0'};
            strcpy(caFileName,pdu->caData);

            char *pPath= new char[pdu->uiMsgLen];
            memcpy(pPath,pdu->caMsg,pdu->uiMsgLen);

            QString strPath=QString("%1/%2").arg(pPath).arg(caFileName);
            // qDebug()<<strPath;
            delete []pPath;
            pPath=NULL;

            QFileInfo fileInfo(strPath);
            qint64 fileSize=fileInfo.size();

            PDU *respdu=mkPDU(0);
            respdu->uiMsgType=ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            sprintf(respdu->caData,"%s %lld",caFileName,fileSize);

            qDebug()<<respdu->caData;


            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;

            m_file.setFileName(strPath);
            m_file.open(QIODevice::ReadOnly);
            m_pTimer->start(1000);

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        {
            char caSendName[32]={'\0'};
            int num =0;
            sscanf(pdu->caData,"%s%d",caSendName,&num);
            int size = num*32;
            PDU *respdu = mkPDU(pdu->uiPDULen-size);
            respdu->uiMsgType=ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strcpy(respdu->caData,caSendName);
            memcpy(respdu->caMsg,(char*)(pdu->caMsg)+size,pdu->uiPDULen-size);

            char caRecvName[32]={'\0'};
            for (int i = 0;i<num;i++)
            {
                memcpy(caRecvName,(char*)(pdu->caMsg)+i*32,32);
                MyTcpServer::getInstance().resend(caRecvName,respdu);
            }
            free(respdu);
            respdu=NULL;

            respdu = mkPDU(0);
            respdu->uiMsgType =ENUM_MSG_TYPE_SHARE_FILE_RESPOND;
            strcpy(respdu->caData,"share file ok");
            write((char*)respdu,respdu->uiPDULen);

            free(respdu);
            respdu=NULL;

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND:
        {
            QString strRecvPath = QString("./%1").arg(pdu->caData);
            QString strShareFilePath = QString("%1").arg((char*)(pdu->caMsg));
            int index =strShareFilePath.lastIndexOf('/');
            QString strFileName=strShareFilePath.right(strShareFilePath.size()-index-1);
            strRecvPath=strRecvPath+'/'+strFileName;
            QFileInfo fileInfo(strShareFilePath);
            if(fileInfo.isFile())
            {
                QFile::copy(strShareFilePath,strRecvPath);
            }
            else if(fileInfo.isDir())
            {

                copyDir(strShareFilePath,strRecvPath);
            }
            break;
        }

        default:
            break;
        }


        free(pdu);
        pdu=NULL;
    }
    else
    {
        QByteArray buff= readAll();
        m_file.write(buff);
        m_iRecved+=buff.size();
        PDU *respdu=NULL;
        respdu=mkPDU(0);
        respdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;
        if(m_iTotal==m_iRecved)
        {
            m_file.close();
            m_bUpload=false;
            strcpy(respdu->caData,UPLOAD_FILE_OK);

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
        }
        else if(m_iTotal< m_iRecved)
        {
            m_file.close();
            m_bUpload=false;

            strcpy(respdu->caData,UPLOAD_FILE_FAILURED);

            write((char*)respdu,respdu->uiPDULen);
            free(respdu);
            respdu=NULL;
        }

    }
    //qDebug()<<caName<<caPwd<<pdu->uiMsgType;

}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offline(this);
}

void MyTcpSocket::sendFileToClient()
{
    m_pTimer->stop();
    char *pData = new char[4096];
    qint64 ret =0;
    while(true)
    {
        ret=m_file.read(pData,4096);
        if(ret > 0 && ret <= 4096)
        {
            this->write(pData,ret);
        }
        else if(ret==0)
        {
            m_file.close();

            break;
        }
        else if(ret<0)
        {
            qDebug()<<"发送文件给客户端过程中失败";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData=NULL;
}
