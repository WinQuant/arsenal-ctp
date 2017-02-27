# -*- coding: utf-8 -*-

'''Utility functions for bullet CTP driver.
'''

'''
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
'''

# built-in modules

# third-party modules

# customized modules
import ctp.ctpOrder as ctpOrder

def getCtpInstId( secId ):
    '''Get CTP instrument ID of the given securities.

Parameters
----------
secId : str
    Securities identifier.

Returns
-------
ctpInstId : str
    instrument identifier of the given securities identifier.
    '''
    instId, exch = secId.split( '.' )

    if exch.startswith( 'XZCE' ) or exch.startswith( 'CCFX' ):
        # The CTP protocol use upper name for instruments in
        # Zhengzhou Commodity Exchange and China Finance Exchange
        instId = instId.upper()
    else:
        instId = instId.lower()

    return instId


def getCtpExch( secId ):
    '''Get the CTP exchange name of the given securities identifier.

Parameters
----------
secId : str
    Securities identifier.

Returns
-------
ctpExch : str
    exchange identifier of the given securities.
    '''
    secIdCmp = secId.split( '.' )[ 1 ]
    ctpExch  = secIdCmp[ 1 ] if len( secIdCmp ) > 1 else ''

    return ctpExch


def convertToCtpOrder( order ):
    '''Convert the given bullet order to a CTP order.

Parameters
----------
order : execution.order.Order
    Bullet order;

Returns
-------
ctpOrder : ctpOrder.CTPOrder
    CTP order.
    '''
    instId = getCtpInstId( order.secId )
    exch   = getCtpExch( order.secId )
    side   = ctpOrder.CTPOrder.DIRECTION_BUY if order.side == order.BUY \
                    else ctpOrder.CTPOrder.DIRECTION_SELL
    volume = order.volume
    price  = order.price
    offset = ctpOrder.CTPOrder.OFFSET_OPEN if order.offset is not None and order.offset == order.OPEN \
                    else ctpOrder.CTPOrder.OFFSET_CLOSE
    priceType = ctpOrder.CTPOrder.PRICE_LIMIT_PRICE

    return ctpOrder.CTPOrder( instId, exch, side, volume, priceType, price,
            offset )
