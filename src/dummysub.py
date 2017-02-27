# -*- coding: utf-8 -*-
'''Dummy subscriber for CTP testing.'''

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
import zmq

# customized modules
import datafeed.subpub  as dsp
import ctpDataPublisher as cdd

class DummySubscriber( dsp.Subscriber ):
    '''A dummy subscriber print out all the data received.
    '''

    def __init__( self, instrument ):
        '''Initialize a dummy subscriber.

Parameters
----------
instrument : str
    name of the instrument to subscribe.
        '''
        self.instrument = instrument


    def onData( self, data ):
        '''Responsor to the arrived data.

Parameters
----------
data : object
    data from up-stream feed.
        '''
        print( data )


    def getSubscribedTopics( self ):
        '''Topics to subscribe.

Returns
-------
subscribes : list of str
    list of topics to subscribe.
        '''
        return [ self.instrument, 'cu1607' ]

def main():
    '''Test driver
    '''
    pub = cdd.CTPDataPublisher( 'config.yaml' )
    sub = DummySubscriber( 'cu1609' )
    pub.addSubscriber( sub )
    pub.connect().join()
    print( 'hello' )


def recv( addr, topic ):
    '''Receive published information.
    '''
    ctx  = zmq.Context()
    sock = ctx.socket( zmq.SUB )
    sock.setsockopt_string( zmq.SUBSCRIBE, topic )
    sock.connect( addr )
    while True:
        print( sock.recv() )


if __name__ == '__main__':
    main()
