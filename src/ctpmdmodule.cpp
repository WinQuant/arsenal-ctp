/* CTP python driver

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

#include <Python.h>
#include <zmq.h>

#include "bulletMarketDataSpi.h"

#define BUF_SIZE 1024

// macro
#define PARSE_STRING_ARRAY_AND_APPLY( args, func ) { \
    PyObject *pResult = NULL;                        \
    PyObject *pList   = NULL;                        \
    /* ensure GIL */                                 \
    PyGILState_STATE gState = PyGILState_Ensure();   \
    if ( !PyArg_ParseTuple( args, "O", &pList ) ) {  \
        pResult = NULL;                              \
    } else {                                         \
        int iListLen = ( int )PyList_Size( pList );  \
        char **ppInstrumentList = ( char ** )malloc( iListLen * sizeof( char * ) ); \
        if ( ppInstrumentList != NULL ) {            \
            PyObject *iterator = PyObject_GetIter( pList );                         \
            PyObject *item = NULL;                   \
            int i = 0;                               \
            while ( item = PyIter_Next( iterator ) ) {                              \
                /* decode the string object */       \
                if ( PyUnicode_Check( item ) ) {     \
                    PyObject *tempBytes = PyUnicode_AsEncodedString( item, "ASCII", "strict" ); \
                    if ( tempBytes != NULL ) {       \
                        const char *tempStr = PyBytes_AS_STRING( tempBytes );       \
                        ppInstrumentList[ i++ ] = strdup( tempStr );                \
                        /* release reference when done */                           \
                        Py_DECREF( tempBytes );      \
                        Py_DECREF( item );           \
                    } else {                         \
                        std::cerr << "Failed to decode a string object." << std::endl; \
                    }                                \
                } else {                             \
                    std::cerr << "The passed in object is not of type str." << std::endl; \
                }                                    \
            }                                        \
            Py_DECREF( iterator );                   \
            int iResult = func( ppInstrumentList, iListLen );                             \
            /* free memory once used */              \
            for ( int i = 0; i < iListLen; i++ ) {   \
                free( ppInstrumentList[ i ] );       \
            }                                        \
            free( ppInstrumentList );                \
            pResult = PyLong_FromLong( iResult );    \
        } else {                                     \
            /* Error occur, no enough memory */      \
            PyErr_SetString( ctpError, "No enough memory available for furthur processing." ); \
            pResult = NULL;                          \
        }                                            \
    }                                                \
    /* release GIL */                                \
    PyGILState_Release( gState );                    \
    return pResult;                                  \
}

/* Zero-MQ data structure */
static void *pCtx  = NULL;
static void *pSock = NULL;

static PyObject *ctpError = NULL;
CThostFtdcMdApi *pUserApi = NULL;

static void onMarketData( CThostFtdcDepthMarketDataField * );

static PyObject *login( PyObject *self, PyObject *args ) {
    // 4-argument required
    //     frontIp : str
    //         Gateway IP address;
    //     brokderId : str
    //         broker ID;
    //     investorId : str
    //         investor account;
    //     password : str
    //         password to authenticate.
    // initialize the Api object
    // extract the pass-in argument
    char *frontIp    = NULL;
    char *brokerId   = NULL;
    char *investorId = NULL;
    char *password   = NULL;

    if ( !PyArg_ParseTuple( args, "ssss", &frontIp, &brokerId, &investorId,
            &password ) ) {
        return NULL;
    }

    // initialize the object
    pUserApi = CThostFtdcMdApi::CreateFtdcMdApi();
    CThostFtdcMdSpi *pUserSpi = new BulletMdSpi(
            brokerId, investorId, password, onMarketData );
    pUserApi->RegisterSpi( pUserSpi );
    pUserApi->RegisterFront( frontIp );

    PyObject *result = PyLong_FromLong( 0 );

    return result;
}

static PyObject *connect( PyObject *self, PyObject *args ) {
    // return value
    PyObject *pResult = NULL;
    // address to publish the message
    const char *pAddr    = NULL;
    PyObject *pStartDate = NULL;
    PyObject *pEndDate   = NULL;

    if ( !PyArg_ParseTuple( args, "s|OO", &pAddr, &pStartDate, &pEndDate) ) {
        pResult = NULL;
    } else {
        // initialize Zero MQ socket
        if ( ( pCtx = zmq_ctx_new() ) == NULL ) {
            PyErr_SetString( ctpError, "Fail to create message queue." );
            std::cerr << "Fail to create context." << std::endl;
            pResult = NULL;
        } else if ( ( pSock = zmq_socket( pCtx, ZMQ_PUB ) ) == NULL ) {
            zmq_ctx_destroy( pCtx );
            PyErr_SetString( ctpError, "Fail to create message queue." );
            std::cerr << "Fail to create socket." << std::endl;
            pResult = NULL;
        } else if ( zmq_bind( pSock, pAddr ) < 0 ) {
            zmq_close( pSock );
            zmq_ctx_destroy( pCtx );
            PyErr_SetString( ctpError, "Fail to create message queue." );
            std::cerr << "Fail to bind socket." << std::endl;
            pResult = NULL;
        } else {
            // Establish connection to the data source
            pUserApi->Init();
            std::cerr << "Everything is ready" << std::endl;

            pResult = PyLong_FromLong( 0 );
        }
    }

    return pResult;
}

/* subscribe market data */
static PyObject *subscribeMarketData( PyObject *self, PyObject *args )   PARSE_STRING_ARRAY_AND_APPLY( args, pUserApi->SubscribeMarketData )
static PyObject *unsubscribeMarketData( PyObject *self, PyObject *args ) PARSE_STRING_ARRAY_AND_APPLY( args, pUserApi->UnSubscribeMarketData )

/* subscribe quote data */
static PyObject *subscribeForQuoteRsp( PyObject *self, PyObject *args )   PARSE_STRING_ARRAY_AND_APPLY( args, pUserApi->SubscribeForQuoteRsp )
static PyObject *unsubscribeQuoteForRsp( PyObject *self, PyObject *args ) PARSE_STRING_ARRAY_AND_APPLY( args, pUserApi->UnSubscribeForQuoteRsp )

static void onMarketData( CThostFtdcDepthMarketDataField *pData ) {
    // send message to the message queue once message arrives
    // including date, instrument id, update time, and last price
    // TODO need to send more data
    char buf[ BUF_SIZE ] = { 0 };
    int iMsgLen = snprintf( buf, BUF_SIZE, "%s,%s %s,%f", pData->InstrumentID,
            pData->TradingDay, pData->UpdateTime, pData->LastPrice );

    if ( zmq_send( pSock, buf, iMsgLen, 0 ) < 0 ) {
        // if error occurs, simply drop the error.
        std::cerr << "Error sending messages through ZeroMQ." << std::endl;
    }
}

// module method table and initialization function
static PyMethodDef ctpMdMethods[] = {
    { "login", login, METH_VARARGS, "Login market data service." },
    { "connect", connect, METH_VARARGS,
      "Initialize the 0MQ instance and put the market data publisher online." },
    { "subscribeMarketData", subscribeMarketData, METH_VARARGS,
      "Subscribe market data." },
    { "unsubscribeMarketData", unsubscribeMarketData, METH_VARARGS,
      "Unsubscribe market data." },
    { "subscribeForQuoteRsp",   subscribeForQuoteRsp,  METH_VARARGS,
      "Subscribe quote messages." },
    { "unsubscribeForQuoteRsp", unsubscribeMarketData, METH_VARARGS,
      "Unsubscribe quote messages." },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef ctpMdModule = {
    PyModuleDef_HEAD_INIT,
    "ctpmd",
    "CTP Market data interface for bullet.",
    -1,
    ctpMdMethods
};

PyMODINIT_FUNC PyInit_ctpmd( void ) {
    return PyModule_Create( &ctpMdModule );
}
