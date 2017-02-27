/* CTP market data API implementation.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bulletMarketDataSpi.h"

extern CThostFtdcMdApi *pUserApi;

char *ppInstrumentId[] = { "cu1607", "cu1609" };

// constructor
BulletMdSpi::BulletMdSpi( std::string brokerId, std::string investorId,
		std::string password, void(*onMarketData)(CThostFtdcDepthMarketDataField *) ) : CThostFtdcMdSpi(), _brokerId( brokerId ),
		_investorId( investorId ), _password( password ), _reqId( 0 ), _onMarketData( onMarketData ) {
}

// destructor
BulletMdSpi::~BulletMdSpi() {
}

void BulletMdSpi::OnFrontConnected() {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    // request user login
    reqUserLogin();
}

void BulletMdSpi::OnFrontDisconnected( int nReason ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    std::cerr << "--->>> Reason = " << nReason << std::endl;
}

void BulletMdSpi::OnHeartBeatWarning( int nTimeElapse ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    std::cerr << "--->>> nTimeElapse = " << nTimeElapse << std::endl;
}

void BulletMdSpi::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    if ( bIsLast && !isErrorRspInfo( pRspInfo ) ) {
        _frontId   = pRspUserLogin->FrontID;
        _sessionId = pRspUserLogin->SessionID;
        int orderRef = atoi( pRspUserLogin->MaxOrderRef );
        sprintf( _orderRef, "%d", orderRef + 1 );
        std::cerr << "--->>> Current trading time = " << pUserApi->GetTradingDay() << std::endl;
        // subscribeMarketData();
    }
}

void BulletMdSpi::OnRspUserLogout( CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void BulletMdSpi::OnRspSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void BulletMdSpi::OnRspUnSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void BulletMdSpi::OnRspSubForQuoteRsp( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void BulletMdSpi::OnRspUnSubForQuoteRsp( CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void BulletMdSpi::OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pDepthMarketData ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    std::cerr << pDepthMarketData->InstrumentID << " " << pDepthMarketData->UpdateTime << std::endl;
    if ( _onMarketData != NULL ) {
        std::cerr << "Send message" << std::endl;
        ( *_onMarketData )( pDepthMarketData );
    }
}

void BulletMdSpi::OnRtnForQuoteRsp( CThostFtdcForQuoteRspField *pForQuoteRsp ) {
	/* Quote is used for IOI query, currently, do not care too much about it. */
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

/* Error handling */
void BulletMdSpi::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast ) {
    std::cerr << "--->>> " << __FUNCTION__ << std::endl;
    isErrorRspInfo( pRspInfo );
}


void BulletMdSpi::reqUserLogin() {
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

void BulletMdSpi::subscribeMarketData() {
	int iResult = pUserApi->SubscribeMarketData( ppInstrumentId, 2 );
    std::cerr << "--->>> Subscribe: " << ( ( iResult == 0 ) ? "succeeded" : "failed" ) << std::endl;
}

bool BulletMdSpi::isErrorRspInfo( CThostFtdcRspInfoField *pRspInfo ) {
    bool bResult = ( pRspInfo && ( pRspInfo->ErrorID != 0 ) );
    if ( bResult ) {
        std::cerr << "--->>> ErrorID = " << pRspInfo->ErrorID << ", ErrorMsg = " << pRspInfo->ErrorMsg << std::endl;
    }

    return bResult;
}
