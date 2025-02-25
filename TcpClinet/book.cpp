#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "opewidget.h"
#include "sharefile.h"

Book::Book(QWidget *parent)
    : QWidget{parent}
{
    m_pTimer=new QTimer;
    m_bDownload=false;

    m_pBookListW = new QListWidget;
    m_pReturnPB = new QPushButton("返回");
    m_pCreateDirPB = new QPushButton("创建文件夹");
    m_pDelDirPB = new QPushButton("删除文件夹");
    m_pRenamePB = new QPushButton("重命名文件夹");
    m_pFlushFilePB = new QPushButton("刷新文件");

    QVBoxLayout *pDirVBL =new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);


    m_pUploadPB = new QPushButton("上传文件");
    m_pDownLoadPB = new QPushButton("下载文件");
    m_pDelFilePB = new QPushButton("删除文件");
    m_pShareFilePB = new QPushButton("共享文件");
    //m_pMoveFilePB =new QPushButton("移动文件");

    QVBoxLayout *pFileVBL =new QVBoxLayout;
    pFileVBL->addWidget(m_pUploadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);

    QHBoxLayout *pMain =new QHBoxLayout;
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(pDirVBL);
    pMain->addLayout(pFileVBL);

    setLayout(pMain);

    connect(m_pCreateDirPB,SIGNAL(clicked(bool)),this,SLOT(createDir()));
    connect(m_pFlushFilePB,SIGNAL(clicked(bool)),this,SLOT(flushFile()));
    connect(m_pDelDirPB,SIGNAL(clicked(bool)),this,SLOT(delDir()));
    connect(m_pRenamePB,SIGNAL(clicked(bool)),this,SLOT(renameDir()));
    connect(m_pBookListW,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(enterDir(QModelIndex)));
    connect(m_pReturnPB,SIGNAL(clicked(bool)),this,SLOT(returnPre()));
    connect(m_pUploadPB,SIGNAL(clicked(bool)),this,SLOT(uploadFile()));
    connect(m_pTimer,SIGNAL(timeout()),this,SLOT(uploadFileData()));
    connect(m_pDelFilePB,SIGNAL(clicked(bool)),this,SLOT(delFile()));
    connect(m_pDownLoadPB,SIGNAL(clicked(bool)),this,SLOT(downloadFile()));
    connect(m_pShareFilePB,SIGNAL(clicked(bool)),this,SLOT(sharFile()));
}



void Book::updateFileList(const PDU *pdu)
{
    if(NULL==pdu)
    {
        return;
    }

    m_pBookListW->clear();

    FileInfo *pFileInfo = NULL;
    int iCount = pdu->uiMsgLen/sizeof(FileInfo);
    for(int i= 0; i<iCount;i++)
    {
        pFileInfo = (FileInfo*)(pdu->caMsg)+i;
        // qDebug()<<pFileInfo->caFileName<<pFileInfo->iFileType;
        QListWidgetItem *pItem = new QListWidgetItem;
        if (0==pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/icon/dir.png")));
        }
        else if(1==pFileInfo->iFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":/icon/reg.jpg")));
        }
        pItem->setText(pFileInfo->caFileName);
        m_pBookListW->addItem(pItem);
    }
}

void Book::clearEnterDir()
{
    m_strEnterDir.clear();
}

QString Book::enterDir()
{
    return m_strEnterDir;
}

void Book::setDownloadStatus(bool status)
{
    m_bDownload=status;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}

void Book::createDir()
{
    QString strNewDir = QInputDialog::getText(this,"新建文件夹","新文件夹名字");
    if(!strNewDir.isEmpty())
    {
        if (strNewDir.size()>32)
        {
            QMessageBox::warning(this,"新建文件夹","新文件夹名字不能超过32字节");
        }
        else
        {
            QString strName = TcpClient::getInstance().loginName();
            QString strCurPath = TcpClient::getInstance().curPath();
            PDU *pdu = mkPDU(strCurPath.size()+1);
            pdu->uiMsgType=ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData,strName.toStdString().c_str(),strName.size());
            strncpy(pdu->caData+32,strNewDir.toStdString().c_str(),strNewDir.size());
            memcpy(pdu->caMsg,strCurPath.toStdString().c_str(),strCurPath.size());

            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu=NULL;
        }
    }
    else
    {
        QMessageBox::warning(this,"新建文件夹","新文件夹名字不能为空");
    }

}

void Book::flushFile()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    PDU *pdu=mkPDU(strCruPath.size()+1);
    pdu->uiMsgType=ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    strncpy((char*)(pdu->caMsg),strCruPath.toStdString().c_str(),strCruPath.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void Book::delDir()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL==pItem)
    {
        QMessageBox::warning(this,"删除文件","请选择要删除的文件");
    }
    else
    {
        QString strDelName = pItem->text();
        PDU *pdu=mkPDU(strCruPath.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_DEL_DIR_REQUEST;
        strncpy(pdu->caData,strDelName.toStdString().c_str(),strDelName.size());
        memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;
    }
}

void Book::renameDir()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL==pItem)
    {
        QMessageBox::warning(this,"重命名文件","请选择要重命名的文件");
    }
    else
    {
        QString strOldName=pItem->text();
        QString strNewName=QInputDialog::getText(this,"重命名文件","请输入新的文件名");
        if(!strNewName.isEmpty())
        {
            PDU *pdu=mkPDU(strCruPath.size()+1);
            pdu->uiMsgType=ENUM_MSG_TYPE_RENAME_FILE_REQUEST;
            strncpy(pdu->caData,strOldName.toStdString().c_str(),strOldName.size());
            strncpy(pdu->caData+32,strNewName.toStdString().c_str(),strNewName.size());
            memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());
            TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
            free(pdu);
            pdu=NULL;
        }
        else
        {
            QMessageBox::warning(this,"重命名文件","新文件名不能为空");
        }


    }
}

void Book::enterDir(const QModelIndex &index)
{
    QString strDirName=index.data().toString();
    m_strEnterDir = strDirName;
    QString strCruPath =TcpClient::getInstance().curPath();
    PDU *pdu=mkPDU(strCruPath.size()+1);
    pdu->uiMsgType=ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData,strDirName.toStdString().c_str(),strDirName.size());
    memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());

    TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
    free(pdu);
    pdu=NULL;
}

void Book::returnPre()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QString strRootPath ="./"+TcpClient::getInstance().loginName();
    if (strCruPath==strRootPath)
    {
        QMessageBox::warning(this,"返回","返回失败：已经在根目录");

    }
    else
    {
        //"./aa/bb/cc" --> "./aa/bb"
        int index = strCruPath.lastIndexOf('/');
        strCruPath.remove(index,strCruPath.size()-index);
        TcpClient::getInstance().setCurPath(strCruPath);


        clearEnterDir();

        flushFile();
    }
}

void Book::uploadFile()
{

    m_strUploadFilePath=QFileDialog::getOpenFileName();
    qDebug()<<m_strUploadFilePath;
    if(! m_strUploadFilePath.isEmpty())
    {
        int index=m_strUploadFilePath.lastIndexOf('/');
        QString strFileName=m_strUploadFilePath.right(m_strUploadFilePath.size()-index-1);
        QFile file(m_strUploadFilePath);
        qint64 fileSize=file.size();

        QString strCruPath =TcpClient::getInstance().curPath();
        PDU *pdu=mkPDU(strCruPath.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());
        sprintf(pdu->caData,"%s %lld", strFileName.toStdString().c_str(),fileSize);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;

        m_pTimer->start(1000);

    }
    else
    {
        QMessageBox::warning(this,"上传文件","上传文件不能为空");
    }
}

void Book::uploadFileData()
{
    m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,"上传文件","打开文件失败");
        return;
    }
    char *pBuffer = new char[4096];
    qint64 ret =0;
    while(true)
    {
        ret=file.read(pBuffer,4096);
        if(ret > 0 && ret <= 4096)
        {
            TcpClient::getInstance().getTcpSocket().write(pBuffer,ret);
        }
        else if(ret==0)
        {
            break;
        }
        else
        {
            QMessageBox::warning(this,"上传文件","上传文件失败：读文件失败");
            break;
        }

    }
    file.close();
    delete []pBuffer;
    pBuffer=NULL;
}

void Book::delFile()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL==pItem)
    {
        QMessageBox::warning(this,"删除文件","请选择要删除的文件");
    }
    else
    {
        QString strDelName=pItem->text();
        PDU *pdu =mkPDU(strCruPath.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_DEL_FILE_REQUEST;

        strncpy(pdu->caData,strDelName.toStdString().c_str(),strDelName.size());
        memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;

    }
}

void Book::downloadFile()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL==pItem)
    {
        QMessageBox::warning(this,"下载文件","请选择要下载的文件");
    }
    else
    {
        QString strSaveFilePath =QFileDialog::getSaveFileName();
        if(strSaveFilePath.isEmpty())
        {
            QMessageBox::warning(this,"下载文件","请指定要保存的位置");
            m_strSaveFilePath.clear();
        }
        else
        {
            m_strSaveFilePath=strSaveFilePath;
            // m_bDownload=true;
        }

        QString strFileName=pItem->text();
        PDU *pdu =mkPDU(strCruPath.size()+1);
        pdu->uiMsgType=ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;

        strncpy(pdu->caData,strFileName.toStdString().c_str(),strFileName.size());
        memcpy(pdu->caMsg,strCruPath.toStdString().c_str(),strCruPath.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu,pdu->uiPDULen);
        free(pdu);
        pdu=NULL;


    }
}

void Book::sharFile()
{
    QString strCruPath =TcpClient::getInstance().curPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if(NULL==pItem)
    {
        QMessageBox::warning(this,"分享文件","请选择要分享的文件");
        return;
    }
    else
    {
        m_strShareFileName =pItem->text();

    }
    Friend *pFriend=OpeWidget::getInstance().getFriend();
    QListWidget *pFriendList = pFriend->getFriendList();
    ShareFile::getInstance().updateFriend(pFriendList);
    if (ShareFile::getInstance().isHidden())
    {
        ShareFile::getInstance().show();
    }
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

QString Book::getShareFileName()
{
    return m_strShareFileName;
}
