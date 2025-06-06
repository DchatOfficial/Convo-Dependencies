/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_NGINX_HTTPS
#define NODEPP_NGINX_HTTPS

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <express/https.h>
#include <nodepp/https.h>
#include <nodepp/path.h>
#include <nodepp/json.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class nginx_https_t : public express_tls_t {
protected:

    void file( express_https_t& cli, string_t cmd, string_t path, object_t args ) const noexcept {

        auto pth = regex::replace( cli.path, path, "/" );
             pth = regex::replace_all( pth, "\\.[.]+/", "" );

        auto bsd =!args["path"].has_value() ? "./" :
                   args["path"].as<string_t>() ;

        auto dir = pth.empty() ? path::join( bsd, "" ) :
                                 path::join( bsd,pth ) ;

        if ( dir.empty() ){ dir = path::join( bsd, "index.html" ); }
        if ( dir[dir.last()] == '/' ){ dir += "index.html"; }

        if( fs::exists_file(dir+".html") == true ){ dir += ".html"; }
        if( fs::exists_file(dir) == false || dir == bsd ){
        if( !path::extname(dir).empty() ){ cli.status(404).send("not_found"); return; }
        if( fs::exists_file( path::join( bsd, "404.html" ) )){
            dir = path::join( bsd, "404.html" ); cli.status(404);
        } else {
            cli.status(404).send("Oops 404 Error");
            return;
        }}

        if( !cli.headers.has("Range") ){

            if( regex::test(path::mimetype(dir),"audio|video",true) ){ cli.send(); return; }
            if( regex::test(path::mimetype(dir),"html",true) ){ cli.render(dir); } else {
                cli.header( "Cache-Control", "public, max-age=604800" );
			    cli.sendFile( dir );
            }

        } else { auto str = fs::readable( dir );

            array_t<string_t> range = regex::match_all(cli.headers["Range"],"\\d+",true);
             ulong rang[3]; rang[0] = string::to_ulong( range[0] );
                   rang[1] =min(rang[0]+CHUNK_MB(10),str.size()-1);
                   rang[2] =min(rang[0]+CHUNK_MB(10),str.size()  );

            cli.header( "Content-Range", string::format("bytes %lu-%lu/%lu",rang[0],rang[1],str.size()) );
            cli.header( "Content-Type",  path::mimetype(dir) ); cli.header( "Accept-Range", "bytes" );
            cli.header( "Cache-Control", "public, max-age=604800" );

            str.set_range( rang[0], rang[2] );
            cli.status(206).sendStream( str );

        }

    }

    /*.........................................................................*/

    void pipe( express_https_t& cli, string_t cmd, string_t path, object_t args ) const noexcept {
        if( !args["href"].has_value() ){ cli.status(503).send("url not found"); return; }

        auto uri = url::parse( args["href"].as<string_t>() );
        auto pth = regex::replace( cli.path, path, "/" );
             pth = path::join( uri.path, pth );
             pth+= cli.search;
        auto slf = type::bind( cli );
        auto hdr = cli.headers;

        hdr["Params"] = query::format( cli.params );
        hdr["Real-Ip"]= cli.get_peername();
        hdr["Host"]   = uri.hostname;

        if( uri.protocol.to_lower_case() == "https" ){

            ssl_t ssl; tls_t tmp ([=]( https_t dpx ){
                dpx.write_header( slf->method, pth, slf->get_version(), hdr );
                dpx .set_timeout( args["timeout"].as<uint>() );
                slf->set_timeout( args["timeout"].as<uint>() );
                stream::duplex( *slf,dpx );
            }, &ssl );

            tmp.onError([=]( except_t err ){
                slf->status(503).send( err.what() );
            });

            tmp.connect( uri.hostname, uri.port ); slf->done();

        } else {

            tcp_t tmp ([=]( http_t dpx ){
                dpx.write_header( slf->method, pth, slf->get_version(), hdr );
                dpx .set_timeout( args["timeout"].as<uint>() );
                slf->set_timeout( args["timeout"].as<uint>() );
                stream::duplex( *slf,dpx );
            });

            tmp.onError([=]( except_t err ){
                slf->status(503).send( err.what() );
            });

            tmp.connect( uri.hostname, uri.port ); slf->done();

        }

    }

    /*.........................................................................*/

    void append( string_t cmd, string_t path, object_t* args ) const noexcept {
        auto n = args==nullptr ? object_t() : *args; auto self = type::bind( this );
        this->ALL( path, [=]( express_https_t& cli ){

            if(!n["timeout"].has_value() ){ n["timeout"] = 0; }
            if( n["timeout"].has_value() ){ cli.set_timeout(n["timeout"].as<uint>()); }
            if( n["method"] .has_value() && !regex::test( cli.method, n["method"].as<string_t>() ) )
              { return; }

              if( cmd.to_lower_case() == "file" ){ self->file( cli, cmd, path, n ); }
            elif( cmd.to_lower_case() == "pipe" ){ self->pipe( cli, cmd, path, n ); }
            elif( cmd.to_lower_case() == "move" ){
                auto href =!n["href"].has_value() ? "./" :
                            n["href"].as<string_t>();
                cli.redirect( href );
            }

        });
    }

public:

    template< class... T >
    nginx_https_t( const T&... args ) noexcept : express_tls_t( args... ) {}

    void add( string_t cmd, string_t path, object_t args ) const noexcept {
        append( cmd, path, &args );
    }

    void add( string_t cmd, string_t path ) const noexcept {
        append( cmd, path, nullptr );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace nginx { namespace https {

    template< class... T > nginx_https_t add( T... args ) {
        return nginx_https_t( args... );
    }

}}}

/*────────────────────────────────────────────────────────────────────────────*/

#endif
