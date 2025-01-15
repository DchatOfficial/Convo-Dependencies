/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_DSOCKET
#define NODEPP_DSOCKET

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/http.h>
#include "generator.h"

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class ds_t : public socket_t {
protected:

    struct NODE {
        _ds_::write write;
        _ds_::read  read ;
    };  ptr_t<NODE> ds;

public:

    template< class... T > 
    ds_t( const T&... args ) noexcept : socket_t( args... ), ds( new NODE() ){}

    virtual int _write( char* bf, const ulong& sx ) const noexcept {
        if( is_closed() ){ return -1; } if( sx==0 ){ return 0; }
        while( ds->write( this, bf, sx )==1 ){ return -2; }
        return ds->write.data==0 ? -2 : ds->write.data;
    }

    virtual int _read( char* bf, const ulong& sx ) const noexcept {
        if( is_closed() ){ return -1; } if( sx==0 ){ return 0; }
        while( ds->read( this, bf, sx )==1 ){ return -2; }
        return ds->read.data==0 ? -2 : ds->read.data;
    }

public:

    bool _write_( char* bf, const ulong& sx, ulong& sy ) const noexcept {
        if( sx==0 || is_closed() ){ return 1; } while( sy < sx ) {
            int c = __write( bf+sy, sx-sy );
            if( c <= 0 && c != -2 )          { return 0; }
            if( c >  0 ){ sy += c; continue; } return 1;
        }   return 0;
    }

    bool _read_( char* bf, const ulong& sx, ulong& sy ) const noexcept {
        if( sx==0 || is_closed() ){ return 1; } while( sy < sx ) {
            int c = __read( bf+sy, sx-sy );
            if( c <= 0 && c != -2 )          { return 0; }
            if( c >  0 ){ sy += c; continue; } return 1;
        }   return 0;
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace ds {

    tcp_t server( const tcp_t& srv ){ srv.onSocket([=]( socket_t cli ){
        auto hrv = type::cast<http_t>(cli);
        if ( !_ds_::server( hrv ) ){ return; }

        cli.onDrain.once([=](){ cli.free(); cli.onData.clear(); });
        ptr_t<_file_::read> _read = new _file_::read;
        cli.set_timeout(0);

        srv.onConnect.once([=]( ds_t ctx ){ process::poll::add([=](){
            if(!cli.is_available() )    { cli.close(); return -1; }
            if((*_read)(&ctx)==1 )      { return 1; }
            if(  _read->state<=0 )      { return 1; }
            ctx.onData.emit(_read->data); return 1;
        }); });

        process::task::add([=](){
            cli.resume(); srv.onConnect.emit(cli); return -1;
        });
        
    }); return srv; }

    /*─······································································─*/

    tcp_t server( agent_t* opt=nullptr ){
        auto server = http::server( [=]( http_t /*unused*/ ){}, opt );
                        ds::server( server ); return server; 
    }

    /*─······································································─*/

    tcp_t client( const string_t& uri, agent_t* opt=nullptr ){ 
    tcp_t srv ( [=]( socket_t /*unused*/ ){}, opt ); 
        srv.connect( url::hostname(uri), url::port(uri) );
        srv.onSocket.once([=]( socket_t cli ){
            auto hrv = type::cast<http_t>(cli);
            if ( !_ds_::client( hrv, uri ) ){ return; }

            cli.onDrain.once([=](){ cli.free(); cli.onData.clear(); }); 
            ptr_t<_file_::read> _read = new _file_::read;
            cli.set_timeout(0);

            srv.onConnect.once([=]( ds_t ctx ){ process::poll::add([=](){
                if(!cli.is_available() )    { cli.close(); return -1; }
                if((*_read)(&ctx)==1 )      { return 1; }
                if(  _read->state<=0 )      { return 1; }
                ctx.onData.emit(_read->data); return 1;
            }); });

            process::task::add([=](){
                cli.resume(); srv.onConnect.emit(cli); return -1;
            });
        
        });
    
    return srv; }

}}

#endif