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

#ifndef BULLET_MDAPI_H
#define BULLET_MDAPI_H

#include <string>

#include "ThostFtdcMdApi.h"

class BulletMdSpi : public CThostFtdcMdSpi {
    /* This is an implementation of the CTP market-data API based on the usecase
     * in Tianfeng Securities.
     */
public:
    BulletMdSpi( std::string brokerId, std::string investorId,
			std::string password,
			void (*onMarketData)( CThostFtdcDepthMarketDataField * ) = NULL);
    /* connection callbacks */
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected( int nReason );

    virtual void OnHeartBeatWarning( int nTimeElapse );

    /* account management */
    virtual void OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspUserLogout( CThostFtdcUserLogoutField *pUserLogout,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );

    /* subscribe-unsubscribe */
    virtual void OnRspSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspUnSubMarketData( CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspSubForQuoteRsp( CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );
    virtual void OnRspUnSubForQuoteRsp( CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestId, bool bIsLast );

    /* market data reply */
    virtual void OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pDepthMarketData );
    virtual void OnRtnForQuoteRsp( CThostFtdcForQuoteRspField *pForQuoteRsp );

    /* error handling */
    virtual void OnRspError( CThostFtdcRspInfoField *pRspInfo,
            int nRequestId, bool bIsLast );

protected:
    ~BulletMdSpi();

private:
    void reqUserLogin();
    void subscribeMarketData();
    bool isErrorRspInfo( CThostFtdcRspInfoField *pRspInfo );

    std::string _frontAddr;            // front gateway IP address
    std::string _brokerId;             // broker identifier 
    std::string _investorId;           // investor identifier
    std::string _password;             // password

    int _reqId;                        // current request ID

    /* session sequence book-keeping fields */
    TThostFtdcFrontIDType   _frontId;
    TThostFtdcSessionIDType _sessionId;
    TThostFtdcOrderRefType  _orderRef;

    /* callbacks */
    void ( *_onMarketData )( CThostFtdcDepthMarketDataField *marketData );
};

#endif
