#ifndef NODEPP_DSOCKET_GENERATOR
#define NODEPP_DSOCKET_GENERATOR

#include <nodepp/encoder.h>

namespace nodepp { namespace _ds_ {
    
    /*─······································································─*/

    template< class T > bool server( T& cli ) {
        auto data = cli.read(); cli.set_borrow( data ); int c=0;
        
        while(( c=cli.read_header() )>0 ); /*{ process::next(); }*/
           if(  c == -1  ){ return 0; }

        if( cli.headers["Upgrade"].to_lower_case() == "http-socket" ){

            cli.write_header( 101, header_t({
                { "Transfer-Encoding", "chunked" },
                { "Upgrade", "http-socket" },
                { "Connection", "upgrade" }
            }) );

            cli.stop();             return 1;
        }   cli.set_borrow( data ); return 0;
    }
    
    /*─······································································─*/

    template< class T > bool client( T& cli, string_t url ) {

        int c=0; header_t header ({
            { "Transfer-Encoding", "chunked" },
            { "Upgrade", "http-socket" },
            { "Connection", "upgrade" }
        });

        cli.write_header( "GET", url::path(url), "HTTP/1.0", header );

        while(( c=cli.read_header() )>0 ); /*{ process::next(); }*/ if( c!=0 ){
            _EERROR(cli.onError,"Could not connect to server");
            cli.close(); return false; 
        }

        if( cli.status != 101 || cli.headers["Upgrade"].to_lower_case() != "http-socket" ){ 
            _EERROR(cli.onError,string::format("Can't connect to Server -> status %d",cli.status)); 
            cli.close(); return false; 
        }   cli.stop();  return true;

    }

    /*─······································································─*/

    GENERATOR( read ){ 
    protected:
            ptr_t<char> fb = ptr_t<char>(4);
            string_t buffer;
            ulong sy   = 0;
            ulong sz   = 0;
            int    c   = 0;
    public: ulong data = 0;

    template<class T> coEmit( T* str, char* bf, const ulong& sx ) {
        if( str->is_closed() ){ return -1; }
    coStart; sy=0; data = 0; memset( bf, 0, sx );

        while( bf[0] != '\n' ){
        while( (c=str->__read( bf, 1 ))==-2 ){ coNext; } 
           if( c<=0 ){ data=-1; coEnd; } buffer.push(bf[0]);
        }

        /*------*/
        
        sz = encoder::hex::set<ulong>( buffer.slice(0,-2) );
        if( sz == 0 ){ data=-1; coEnd; } buffer.clear();

        /*------*/

        while( str->_read_( bf, sz, sy )== 1 ){ coNext; }
        while( str->__read( fb.get(),2 )==-2 ){ coNext; }

        /*------*/

        data = sz;

    coStop
    }};

    GENERATOR( write ){
    protected:
            string_t buffer;
            ulong sy   = 0;
            ulong sz   = 0;
    public: ulong data = 0;

    template<class T> coEmit( T* str, char* bf, const ulong& sx ) {
        if( str->is_closed() ){ return -1; }
    coStart; sy=0; data = 0;

        buffer = encoder::hex::get( sx )+"\r\n"; sz =buffer.size();
        while( str->_write_( buffer.get(), sz, sy )==1 ){ coNext; }

        /*------*/

        while( str->__write( bf, sx )==-2 ){ coNext; }

        /*------*/

        buffer = "\r\n"; sz= buffer.size(); sy = 0;
        while( str->_write_( buffer.get(), sz, sy )==1 ){ coNext; }

        /*------*/

        data = sx;

    coStop
    }};

}}

#endif