/* CTP python trader driver

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

#include "bulletTraderSpi.h"

/* macro */
// pyObj : Py_Object *
// cstr  : const char **
#define PYOBJ_STRING_VALUE( pyObj, cstr ) { \
    if ( PyUnicode_Check( pyObj ) ) { \
        PyObject *tmpBytes = PyUnicode_AsEncodedString( pyObj, "ASCII", "strict" ); \
        if ( tmpBytes != NULL ) { \
            const char *tmpStr = PyBytes_AS_STRING( tmpBytes ); \
            *cstr = strdup( tmpStr ); \
            Py_DECREF( tmpBytes ); \
        } else { \
            std::cerr << "Failed to decode a string object." << std::endl; \
        } \
    } else { \
        std::cerr << "The passed in object is not of type str." << std::endl; \
    } \
}


// static PyObject *ctpError;

CThostFtdcTraderApi * pUserApi;
TThostFtdcFrontIDType    frontId;
TThostFtdcSessionIDType  sessionId;

static TThostFtdcBrokerIDType   brokerId;
static TThostFtdcInvestorIDType investorId;
static TThostFtdcOrderRefType   orderRef;

static int requestId = 0;


// signature
static void constructCtpOrder( PyObject *pyOrder, CThostFtdcInputOrderField *order );

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
    char *frontIp  = NULL;
    char *broker   = NULL;
    char *investor = NULL;
    char *password = NULL;

    PyObject *onRspLogin    = NULL;
    PyObject *onOrderSubmitted   = NULL;
    PyObject *onOrderActionTaken = NULL;
    PyObject *onOrderReturn = NULL;
    PyObject *onTradeReturn = NULL;

    if ( !PyArg_ParseTuple( args, "ssssOOOOO", &frontIp, &broker, &investor,
            &password, &onRspLogin, &onOrderSubmitted, &onOrderActionTaken,
            &onOrderReturn, &onTradeReturn ) ) {
        return NULL;
    }

    // copy broker, investor to global variables
    strcpy( brokerId,   broker );
    strcpy( investorId, investor );

    // initialize the object
    pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
    CThostFtdcTraderSpi *pUserSpi = new BulletTraderSpi( brokerId, investorId,
            password, onRspLogin, onOrderSubmitted, onOrderActionTaken,
            onOrderReturn, onTradeReturn );
    pUserApi->RegisterSpi( pUserSpi );
    pUserApi->RegisterFront( frontIp );

    PyObject *result = PyLong_FromLong( 0 );

    return result;
}

static PyObject *connect( PyObject *self, PyObject *args ) {
    /* bring the trader thread online */
    std::cerr << "Connect to the server " << std::endl;

    pUserApi->Init();

    // enable external thread to run
    Py_BEGIN_ALLOW_THREADS
    pUserApi->Join();
    Py_END_ALLOW_THREADS

    // code should not be executed here
    assert( 0 );

    return PyLong_FromLong( 0 );
}

static PyObject *placeOrder( PyObject *self, PyObject *args ) {
    /* return value */
    PyObject *pResult  = NULL;
    PyObject *pOrder   = NULL;
    PyObject *pOrderId = NULL;

    if ( !PyArg_ParseTuple( args, "OO", &pOrder, &pOrderId ) ) {
        pResult = NULL;
    } else {
        CThostFtdcInputOrderField order;
        memset( &order, 0, sizeof( order ) );
        constructCtpOrder( pOrder, &order );
        std::cerr << "--->>> in place order: " << order.InstrumentID << std::endl;

        /* send request */
        int iResult = pUserApi->ReqOrderInsert( &order, ++requestId );
        std::cerr << "--->>> Order input request: " << ( ( iResult == 0 ) ? "Succeeded" : "Failed" ) << std::endl;

        pResult = PyLong_FromLong( 0 );
    }

    return pResult;
}


static PyObject *cancelOrder( PyObject *self, PyObject *args ) {
    PyObject *pResult  = NULL;
    PyObject *pOrderId = NULL;

    if ( !PyArg_ParseTuple( args, "O", &pOrderId ) ) {
        pResult = NULL;
    } else {
        CThostFtdcInputOrderActionField cancelReq;

        memset( &cancelReq, 0, sizeof( cancelReq ) );

        strcpy( cancelReq.BrokerID,   brokerId );
        strcpy( cancelReq.InvestorID, investorId );

        /* order reference ID */
        PyObject *pyOrderId = PyObject_GetAttrString( pOrderId, "orderId" );
        const char *orderId = NULL;
        PYOBJ_STRING_VALUE( pyOrderId, &orderId );
        strcpy( cancelReq.OrderRef, orderId );

        cancelReq.FrontID    = frontId;
        cancelReq.SessionID  = sessionId;
        cancelReq.ActionFlag = THOST_FTDC_AF_Delete;

        /* send request */
        int iResult = pUserApi->ReqOrderAction( &cancelReq, ++requestId );
        std::cerr << "--->>> Order cancel action: " << ( ( iResult == 0 ) ? "Succeeded" : "Failed" ) << std::endl;

        /* release memory */
        Py_DECREF( pyOrderId );

        free( ( void * )orderId );

        pResult = PyLong_FromLong( 0 );
    }

    return pResult;
}


static PyObject *queryOrder( PyObject *self, PyObject *args ) {
    PyObject *pResult  = NULL;
    PyObject *pOrderId = NULL;

    if ( !PyArg_ParseTuple( args, "O", &pOrderId ) ) {
        pResult = NULL;
    } else {
        CThostFtdcQryOrderField qryReq;

        strcpy( qryReq.BrokerID,   brokerId );
        strcpy( qryReq.InvestorID, investorId );

        PyObject *pyInstId = PyObject_GetAttrString( pOrderId, "instId" );
        const char *instId = NULL;
        PYOBJ_STRING_VALUE( pyInstId, &instId );
        strcpy( qryReq.InstrumentID, instId );

        PyObject *pyOrderId = PyObject_GetAttrString( pOrderId, "orderId" );
        const char *orderId = NULL;
        PYOBJ_STRING_VALUE( pyOrderId, &orderId );
        strcpy( qryReq.OrderSysID, orderId );

        /*
        PyObject *pyExch  = PyObject_GetAttrString( pOrderId, "exchange" );
        qryReq.ExchangeID = ( char )PyLong_AsLong( pyExch );
         */

        /* send request */
        int iResult = pUserApi->ReqQryOrder( &qryReq, ++requestId );
        std::cerr << "--->>> Order query: " << ( ( iResult == 0 ) ? "Succeeded" : "Failed" ) << std::endl;

        /* release memory */
        Py_DECREF( pyInstId );
        Py_DECREF( pyOrderId );

        free( ( void * )orderId );

        pResult = PyLong_FromLong( 0 );
    }

    return pResult;
}


static PyObject *updateOrder( PyObject *self, PyObject *args ) {
    PyObject *pResult   = NULL;
    PyObject *pOrderId  = NULL;
    PyObject *pNewOrder = NULL;

    if ( !PyArg_ParseTuple( args, "OO", &pOrderId, &pNewOrder ) ) {
        pResult = NULL;
    } else {
        CThostFtdcInputOrderActionField updateReq;

        /* copy the session varialbes */
        strcpy( updateReq.BrokerID,   brokerId );
        strcpy( updateReq.InvestorID, investorId );
        strcpy( updateReq.OrderRef,   orderRef );

        updateReq.FrontID   = frontId;
        updateReq.SessionID = sessionId;

        /*
        PyObject *pyExch = PyObject_GetAttrString( pOrderId, "exchange" );
        updateReq.ExchangeID = ( char )PyLong_AsLong( pyExch );
         */

        PyObject *pyOrderId = PyObject_GetAttrString( pOrderId, "orderId" );
        const char *orderId = NULL;
        PYOBJ_STRING_VALUE( pyOrderId, &orderId );
        strcpy( updateReq.OrderSysID, orderId );

        PyObject *pyInstId  = PyObject_GetAttrString( pOrderId, "instId" );
        const char *instId  = NULL;
        PYOBJ_STRING_VALUE( pyInstId, &instId );
        strcpy( updateReq.InstrumentID, instId );

        /* updated limit price */
        PyObject *pyPrice    = PyObject_GetAttrString( pNewOrder, "price" );
        updateReq.LimitPrice = PyFloat_AsDouble( pyPrice );

        /* updated volumes */
        PyObject *pyVolume     = PyObject_GetAttrString( pNewOrder, "volume" );
        updateReq.VolumeChange = PyLong_AsLong( pyVolume );

        /* order update flag */
        updateReq.ActionFlag   = THOST_FTDC_AF_Modify;

        /* send request */
        int iResult = pUserApi->ReqOrderAction( &updateReq, ++requestId );
        std::cerr << "--->>> Update order: " << ( ( iResult == 0 ) ? "Succeeded" : "Failed" ) << std::endl;

        /* release memory */
        Py_DECREF( pyInstId );
        Py_DECREF( pyOrderId );

        free( ( void * )orderId );
        free( ( void * )instId );

        pResult = PyLong_FromLong( 0 );
    }

    return pResult;
}


static void constructCtpOrder( PyObject * pyOrder, CThostFtdcInputOrderField *order ) {
    /* Dump Python order to a CTP order struct. This function should be protected by GIL.
     * ensure GIL before entering. */
    assert( pyOrder && order );              /* NULL objects are not allowed */

    /* instrument ID */
    PyObject *pyInstId = PyObject_GetAttrString( pyOrder, "secId" );
    /* order direction */
    PyObject *pyDirect = PyObject_GetAttrString( pyOrder, "side" );
    /* offset */
    PyObject *pyOffset = PyObject_GetAttrString( pyOrder, "offsetFlag" );
    /* price type */
    PyObject *pyType   = PyObject_GetAttrString( pyOrder, "priceType" );
    /* price */
    PyObject *pyPrice  = PyObject_GetAttrString( pyOrder, "price" );
    /* volume */
    PyObject *pyVolume = PyObject_GetAttrString( pyOrder, "volume" );

    /* covert Python order field to a CTP recongnizable one. */
    /* populate the instrument ID */
    const char *instId = NULL;
    PYOBJ_STRING_VALUE( pyInstId, &instId );
    long   direction = PyLong_AsLong( pyDirect );
    long   offset    = PyLong_AsLong( pyOffset );
    long   priceType = PyLong_AsLong( pyType );
    double price     = PyFloat_AsDouble( pyPrice );
    long   volume    = PyLong_AsLong( pyVolume );

    std::cerr << instId << direction << offset << priceType << price << volume << std::endl;
    strcpy( order->InstrumentID, instId );
    order->OrderPriceType = ( TThostFtdcOrderPriceTypeType )priceType;
    order->Direction      = ( TThostFtdcDirectionType )direction;
    order->LimitPrice     = price;
    order->VolumeTotalOriginal = volume;
    order->CombOffsetFlag[ 0 ] = ( char )offset;

    /* fill-in system configure */
    strcpy( order->BrokerID,   brokerId );
    strcpy( order->InvestorID, investorId );
    strcpy( order->OrderRef,   orderRef );

    /* fixed field */
    order->CombHedgeFlag[ 0 ]  = THOST_FTDC_HF_Speculation;  /* speculation order */
    order->TimeCondition       = THOST_FTDC_TC_GFD;          /* order valid til EOD */
    order->VolumeCondition     = THOST_FTDC_VC_AV;           /* any volume */
    order->MinVolume = 1;   /* minimum filled order size */
    order->ContingentCondition = THOST_FTDC_CC_Immediately;
    order->UserForceClose      = 0;
    order->ForceCloseReason    = THOST_FTDC_FCC_NotForceClose;
    order->IsAutoSuspend       = 0;

    /* release Python objects */
    Py_DECREF( pyInstId );
    Py_DECREF( pyDirect );
    Py_DECREF( pyOffset );
    Py_DECREF( pyType );
    Py_DECREF( pyPrice );
    Py_DECREF( pyVolume );

    /* release memory */
    free( ( void * )instId );
}


/* module method table and initalization function */
static PyMethodDef ctpTraderMethods[] = {
    { "login", login, METH_VARARGS, "Login trader service." },
    { "connect", connect, METH_VARARGS, "Bring the trader service online." },
    { "placeOrder",  placeOrder, METH_VARARGS, "Place a CTP order." },
    { "cancelOrder", cancelOrder, METH_VARARGS, "Cancel a placed order." },
    { "queryOrder",  queryOrder, METH_VARARGS, "Query status of the order." },
    { "updateOrder", updateOrder, METH_VARARGS, "Update a place order with new parameters." },
    { NULL, NULL, 0, NULL }
};

static struct PyModuleDef ctpTraderModule = {
    PyModuleDef_HEAD_INIT,
    "ctptrader",
    "CTP trader interface for bullet.",
    -1,
    ctpTraderMethods
};

PyMODINIT_FUNC PyInit_ctptrader( void ) {
    return PyModule_Create( &ctpTraderModule );
}
