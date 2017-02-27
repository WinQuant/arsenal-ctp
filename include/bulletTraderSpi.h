/* CTP trade API implementation.

Copyright (c) 2017, WinQuant Information and Technology Co. Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BULLET_TRADERAPI_H
#define BULLET_TRADERAPI_H

#include <string>

#include <Python.h>
#include "ThostFtdcTraderApi.h"

class BulletTraderSpi : public CThostFtdcTraderSpi {
    /* This is an implementation of the CTP trader API based on the usecase
     * in Tianfeng Securities.
     */
public:
    BulletTraderSpi( std::string brokerId, std::string investorId,
            std::string password, PyObject *onRspUserLogin,
            PyObject *onOrderSubmitted, PyObject *onOrderActionTaken,
            PyObject *onOrderReturn, PyObject *onTradeReturn );
    /* connection callbacks */
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected( int nReason );

    virtual void OnHeartBeatWarning( int nTimeElapse );

    /* account management */
    virtual void OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspUserLogout( CThostFtdcUserLogoutField *pUserLogout,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );

    /* order placement */
    virtual void OnRspOrderInsert( CThostFtdcInputOrderField *pInputOrder,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspOrderAction( CThostFtdcInputOrderActionField *pInputOrder,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRtnOrder( CThostFtdcOrderField *pOrder );
    virtual void OnRtnTrade( CThostFtdcTradeField *pTrade );

    /* order and trade query */
    virtual void OnRspQryOrder( CThostFtdcOrderField *pOrder,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspQryTrade( CThostFtdcTradeField *pTrade,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRtnInstrumentStatus( CThostFtdcInstrumentStatusField *pInstrumentStatus );

    /* order and trade execution */
    virtual void OnRspQryQuote( CThostFtdcQuoteField *pQuote,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspQryTradingNotice( CThostFtdcTradingNoticeField *pTradingNotice,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRtnTradingNotice( CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo );

    /* position query */
    virtual void OnRspQryInvestorPosition( CThostFtdcInvestorPositionField *pInvestorPosition,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspQryInvestorPositionDetail( CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspQryInvestorPositionCombineDetail( CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );

    /* Mark-to-market */
    virtual void OnRspQryTradingAccount( CThostFtdcTradingAccountField *pTradingAccount,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );

    /* error handler */
    virtual void OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnErrRtnOrderInsert( CThostFtdcInputOrderField *pInputOrder,
            CThostFtdcRspInfoField *pRspInfo );
    virtual void OnErrRtnOrderAction( CThostFtdcOrderActionField *pOrderAction,
            CThostFtdcRspInfoField *pRspInfo );

protected:
    ~BulletTraderSpi();

private:
    void reqUserLogin();
    void reqOrderInsert();
    bool isErrorRspInfo( CThostFtdcRspInfoField *pRspInfo );

    std::string _frontAddr;            // front gateway IP address
    std::string _brokerId;             // broker identifier 
    std::string _investorId;           // investor identifier
    std::string _password;             // password

    /* session sequence book-keeping fields */
    TThostFtdcFrontIDType   _frontId;
    TThostFtdcSessionIDType _sessionId;
    TThostFtdcOrderRefType  _orderRef;

    /* callbacks */
    PyObject *_onRspUserLogin;
    PyObject *_onOrderSubmitted;
    PyObject *_onOrderActionTaken;
    PyObject *_onOrderReturn;
    PyObject *_onTradeReturn;

    int _reqId;                        // current request ID
};

#endif
