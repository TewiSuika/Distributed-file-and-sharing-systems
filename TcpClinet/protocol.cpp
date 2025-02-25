#include"protocol.h"
#include<cstdlib>
#include<cstring>

PDU *mkPDU(uint uiMsgLen)
{
    uint uiPDULen=sizeof(PDU)+uiMsgLen;
    PDU* pdu = (PDU*)malloc(uiPDULen);
    if(pdu==NULL)
    {
        exit(EXIT_FAILURE);
    }
    memset(pdu,0,uiPDULen); //初始化
    pdu->uiPDULen=uiPDULen;
    pdu->uiMsgLen=uiMsgLen;
    return pdu;
}
