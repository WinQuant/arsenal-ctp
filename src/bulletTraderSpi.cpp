/* CTP trader API implementation.

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

#include <iostream>
#include <string>

#include <Python.h>

#include "bulletTraderSpi.h"

extern CThostFtdcTraderApi *pUserApi;

// constructor
BulletTraderSpi::BulletTraderSpi( std::string brokerId, std::string investorId,
        std::string password, PyObject *rspUserLogin, PyObject *orderSubmitted,
        PyObject *orderActionTaken, PyObject *orderReturn,
        PyObject *tradeReturn ) : CThostFtdcTraderSpi(), _brokerId( brokerId ),
        _investorId( investorId ), _password( password ), _onRspUserLogin( rspUserLogin ),
        _onOrderSubmitted( orderSubmitted ), _onOrderActionTaken( orderActionTaken ),
        _onOrderReturn( orderReturn ), _onTradeReturn( tradeReturn ), _reqId( 0 ) {
    /* Increase reference counter */
    Py_XINCREF( _onRspUserLogin );
    Py_XINCREF( _onOrderSubmitted );
    Py_XINCREF( _onOrderActionTaken );
    Py_XINCREF( _onOrderReturn );
    Py_XINCREF( _onTradeReturn );
}


// destructor
BulletTraderSpi::~BulletTraderSpi() {
    /* Decrease reference counter */
    Py_XDECREF( _onRspUserLogin );
    Py_XDECREF( _onOrderSubmitted );
    Py_XDECREF( _onOrderActionTaken );
    Py_XDECREF( _onOrderReturn );
    Py_XDECREF( _onTradeReturn );
}


void BulletTraderSpi::OnFrontConnected() {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    // request user login
    reqUserLogin();
}


void BulletTraderSpi::OnFrontDisconnected( int nReason ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    std::cerr << "--->>> Reason = " << nReason << std::endl;
}

void BulletTraderSpi::OnHeartBeatWarning( int nTimeElapse ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    std::cerr << "--->>> nTimeElapse = " << nTimeElapse << std::endl;
}


void BulletTraderSpi::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    if ( bIsLast && !isErrorRspInfo( pRspInfo ) ) {
        _frontId   = pRspUserLogin->FrontID;
        _sessionId = pRspUserLogin->SessionID;
        int orderRef = atoi( pRspUserLogin->MaxOrderRef );
        sprintf( _orderRef, "%d", orderRef + 1 );
        std::cerr << "--->>> Current trading time = " << pUserApi->GetTradingDay() << std::endl;

        /* ensure GIL */
        PyGILState_STATE gState = PyGILState_Ensure();

        // reqOrderInsert();
        if ( !( ( this->_onRspUserLogin == NULL ) || ( this->_onRspUserLogin == Py_None ) ) && \
                PyCallable_Check( this->_onRspUserLogin ) ) {
            /* construct Python objects to invoke the Python methods */
            std::cerr << "--->>> I'm calling" << std::endl;
            PyObject *result  = PyObject_CallFunction( this->_onRspUserLogin, NULL );
            /* release temporary arguments */
            if ( result != NULL )
                Py_DECREF( result );
        }

        /* release GIL */
        PyGILState_Release( gState );
    }
}


void BulletTraderSpi::OnRspUserLogout( CThostFtdcUserLogoutField *pUserLogout,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::reqUserLogin() {
    // Construct the struct for user login
    CThostFtdcReqUserLoginField req;
    memset( &req, 0, sizeof( req ) );
    // fill-in the broker ID
    strncpy( req.BrokerID, this->_brokerId.c_str(), sizeof( req.BrokerID ) );
    // fill-in the investro ID
    strncpy( req.UserID, this->_investorId.c_str(), sizeof( req.UserID ) );
    // fill-in the password
    strncpy( req.Password, this->_password.c_str(), sizeof( req.Password ) );

    int iResult = pUserApi->ReqUserLogin( &req, ++_reqId );
    std::cerr << "--->>> Request login: " << ( ( iResult == 0 ) ? "succeeded" : "failed" ) << std::endl;
}

/* order placement */
void BulletTraderSpi::OnRspOrderInsert( CThostFtdcInputOrderField *pRspOrderInsert,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    TThostFtdcRequestIDType orderId = pRspOrderInsert->RequestID;
    std::cerr << "--->>> " << "old request ID: " << nRequestId << " new request ID: " << orderId << std::endl;

    /* ensure GIL */
    PyGILState_STATE gState = PyGILState_Ensure();

    if ( !( ( this->_onOrderSubmitted == NULL ) || ( this->_onOrderSubmitted == Py_None ) ) && \
            PyCallable_Check( this->_onOrderSubmitted ) ) {
        /* construct Python objects to invoke the Python methods */
        PyObject *result  = PyObject_CallFunction( this->_onOrderSubmitted,
                "ii", orderId, nRequestId );
        /* release temporary arguments */
        if ( result != NULL )
            Py_DECREF( result );
    }

    /* release GIL */
    PyGILState_Release( gState );
}


void BulletTraderSpi::OnRspOrderAction( CThostFtdcInputOrderActionField *pInputOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << nRequestId << bIsLast << std::endl;
    TThostFtdcOrderActionRefType orderActionRef = pInputOrderAction->OrderActionRef;
    std::cerr << "--->>> " << "old request ID: " << nRequestId << " new request ID: " << orderActionRef << std::endl;

    /* ensure GIL */
    PyGILState_STATE gState = PyGILState_Ensure();

    if ( !( ( this->_onOrderActionTaken == NULL ) || ( this->_onOrderActionTaken == Py_None ) ) && \
            PyCallable_Check( this->_onOrderActionTaken ) ) {
        /* construct Python objects to invoke the Python methods */
        PyObject *result  = PyObject_CallFunction( this->_onOrderActionTaken,
                "ii", orderActionRef, nRequestId );

        /* release temporary arguments */
        if ( result != NULL )
            Py_DECREF( result );
    }

    /* release GIL */
    PyGILState_Release( gState );
}


void BulletTraderSpi::OnRtnOrder( CThostFtdcOrderField *pOrder ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;

    /* ensure GIL */
    PyGILState_STATE gState = PyGILState_Ensure();

    if ( !( ( this->_onOrderReturn == NULL ) || ( this->_onOrderReturn == Py_None ) ) && \
            PyCallable_Check( this->_onOrderReturn ) ) {
        /* extract order information
         * 1. order reference ID;
         * 2. notify sequence;
         * 3. order status;
         * 4. volume traded;
         * 5. total volume;
         * 6. sequence number.
         */
        PyObject *result  = PyObject_CallFunction( this->_onOrderReturn,
                "siiiii", pOrder->OrderRef, pOrder->NotifySequence,
                pOrder->OrderStatus, pOrder->VolumeTraded, pOrder->VolumeTotal,
                pOrder->SequenceNo );

        /* release temporary arguments */
        if ( result != NULL )
            Py_DECREF( result );
    }

    /* release GIL */
    PyGILState_Release( gState );
}


void BulletTraderSpi::OnRtnTrade( CThostFtdcTradeField *pTrade ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;

    /* ensure GIL */
    PyGILState_STATE gState = PyGILState_Ensure();

    if ( !( ( this->_onTradeReturn == NULL ) || ( this->_onTradeReturn == Py_None ) ) && \
            PyCallable_Check( this->_onTradeReturn ) ) {
        /* extract trade information
         * 1. order reference ID;
         * 2. order system ID;
         * 3. trade ID;
         * 4. trade price;
         * 5. trade volume;
         * 6. trade date;
         * 7. trade time;
         * 8. order local ID.
         */
        PyObject *result  = PyObject_CallFunction( this->_onTradeReturn,
                "sssdisssi", pTrade->OrderRef, pTrade->OrderSysID,
                pTrade->TradeID, pTrade->Price, pTrade->Volume, pTrade->TradeDate,
                pTrade->TradeTime, pTrade->OrderLocalID, pTrade->SequenceNo );

        /* release temporary arguments */
        if ( result != NULL )
            Py_DECREF( result );
    }

    /* release GIL */
    PyGILState_Release( gState );
}


/* order and trade query */
void BulletTraderSpi::OnRspQryOrder( CThostFtdcOrderField *pOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRspQryTrade( CThostFtdcTradeField *pTrade,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRtnInstrumentStatus( CThostFtdcInstrumentStatusField *pInstrumentStatus ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


/* order and trade execution */
void BulletTraderSpi::OnRspQryQuote( CThostFtdcQuoteField *pQuote,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRspQryTradingNotice( CThostFtdcTradingNoticeField *pTradingNotice,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRtnTradingNotice( CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


/* position query */
void BulletTraderSpi::OnRspQryInvestorPosition( CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRspQryInvestorPositionDetail( CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnRspQryInvestorPositionCombineDetail( CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


/* Mark-to-market */
void BulletTraderSpi::OnRspQryTradingAccount( CThostFtdcTradingAccountField *pTradingAccount,
        CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


/* error handler */
void BulletTraderSpi::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    isErrorRspInfo( pRspInfo );
}


void BulletTraderSpi::OnErrRtnOrderInsert( CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


void BulletTraderSpi::OnErrRtnOrderAction( CThostFtdcOrderActionField *pOrderAction,
        CThostFtdcRspInfoField *pRspInfo ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}


bool BulletTraderSpi::isErrorRspInfo( CThostFtdcRspInfoField *pRspInfo ) {
    bool bResult = ( pRspInfo && ( pRspInfo->ErrorID != 0 ) );
    if ( bResult ) {
        std::cerr << "--->>> ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
    }

    return bResult;
}


void BulletTraderSpi::reqOrderInsert() {
	CThostFtdcInputOrderField req;
	memset(&req, 0, sizeof(req));
	///经纪公司代码
	strcpy(req.BrokerID, _brokerId.c_str());
	///投资者代码
	strcpy(req.InvestorID, _investorId.c_str());
	///合约代码
	strcpy(req.InstrumentID, "cu1609");
	///报单引用
	strcpy(req.OrderRef, "100");
	///用户代码
	// TThostFtdcUserIDType	UserID;
	///报单价格条件: 限价
	req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	///买卖方向:
	req.Direction = THOST_FTDC_D_Sell;
	///组合开平标志: 开仓
	req.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	///组合投机套保标志
	req.CombHedgeFlag[0]  = THOST_FTDC_HF_Speculation;
	///价格
	req.LimitPrice = 38850;
	///数量: 1
	req.VolumeTotalOriginal = 1;
	///有效期类型: 当日有效
	req.TimeCondition = THOST_FTDC_TC_GFD;
	///GTD日期
	// TThostFtdcDateType	GTDDate;
	///成交量类型: 任何数量
	req.VolumeCondition = THOST_FTDC_VC_AV;
	///最小成交量: 1
	req.MinVolume = 1;
	///触发条件: 立即
	req.ContingentCondition = THOST_FTDC_CC_Immediately;
	///止损价
	// TThostFtdcPriceType	StopPrice;
	///强平原因: 非强平
	req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	///自动挂起标志: 否
	req.IsAutoSuspend = 0;
	///业务单元
	// TThostFtdcBusinessUnitType	BusinessUnit;
	///请求编号
	// TThostFtdcRequestIDType	RequestID;
	///用户强评标志: 否
	req.UserForceClose = 0;
	int iResult = pUserApi->ReqOrderInsert(&req, ++_reqId);
	std::cerr << "--->>> 报单录入请求: " << ((iResult == 0) ? "成功" : "失败") << std::endl;
}
