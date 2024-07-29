/*
 * Copyright 2023 The Nodepp Project Authors. All Rights Reserved.
 *
 * Licensed under the MIT (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://github.com/NodeppOficial/nodepp/blob/main/LICENSE
 */

/*────────────────────────────────────────────────────────────────────────────*/

#ifndef NODEPP_ARGON_2
#define NODEPP_ARGON_2

/*────────────────────────────────────────────────────────────────────────────*/

#include <nodepp/nodepp.h>
#include <nodepp/encoder.h>
#include <argon2.h>

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { class argon2_t {
protected:

    struct NODE {
        ptr_t<argon2_context> ctx;
        ptr_t<uint32_t> conf;
        ptr_t<uchar>    hash;
        bool state = 0;
    };  ptr_t<NODE> obj = new NODE();

public:

    /*.........................................................................*/

    argon2_t( uint32_t t_cost, uint32_t m_cost, uint32_t parallelism ) : obj( new NODE() ) {
        obj->state = 1; obj->conf = ptr_t<uint32_t>({ t_cost, m_cost, parallelism });
    }

    argon2_t() : obj( new NODE() ) {
        obj->state = 1; obj->conf = ptr_t<uint32_t>({ 2, 64, 1 });
    }

    /*.........................................................................*/

    int hash( ptr_t<uchar>& hash, const string_t& pass, const string_t& salt, const string_t& secr, const string_t& addr ) const { 
        if( obj->state != 1 ){ return -1; } 

        obj->ctx = new argon2_context({ 
                    hash.get(), (uint32_t)hash.size(), 
            (uchar*)pass.get(), (uint32_t)pass.size(),
            (uchar*)salt.get(), (uint32_t)salt.size(), 
            (uchar*)secr.get(), (uint32_t)secr.size(), 
            (uchar*)addr.get(), (uint32_t)addr.size(), 
            obj->conf[0], obj->conf[1], 
            obj->conf[2], obj->conf[2], 
            ARGON2_VERSION_13, NULL, NULL, ARGON2_DEFAULT_FLAGS
        }); 
        
        int rc = argon2i_ctx( &obj->ctx ); 
        
        if( ARGON2_OK != rc ) {
            process::error( argon2_error_message(rc) );
        }   return rc;
    }

    int hash( ptr_t<uchar>& hash, const string_t& pass ) const { 
        if( obj->state != 1 ){ return -1; } 
        ptr_t<char> salt ( 16, '\0' );

        obj->ctx = new argon2_context({ 
                    hash.get(), (uint32_t)hash.size(), 
            (uchar*)pass.get(), (uint32_t)pass.size(),
            (uchar*)salt.get(), (uint32_t)salt.size(), 
            NULL,0, NULL, 0, 
            obj->conf[0], obj->conf[1], 
            obj->conf[2], obj->conf[2], 
            ARGON2_VERSION_13, NULL, NULL, ARGON2_DEFAULT_FLAGS
        });
        
        int rc = argon2i_ctx( &obj->ctx ); 
        
        if( ARGON2_OK != rc ) {
            process::error( argon2_error_message(rc) );
        }   return rc;
    }

    /*.........................................................................*/

    int verify( const ptr_t<uchar>& hash ){
        auto hex = encoder::hex::get( hash );
        return argon2i_verify_ctx( &obj->ctx, hex.get() );
    }

};}

/*────────────────────────────────────────────────────────────────────────────*/

namespace nodepp { namespace argon2 { namespace hash {

    string_t get( const string_t& pass, const string_t& salt, const string_t& secr, const string_t& addr ) {
        auto hash = ptr_t<uchar>( 32, '\0' ); argon2_t argon; argon.hash( hash, pass, salt, secr, addr );
        return encoder::hex::get( hash );
    }

    string_t get( const string_t& pass ) {
        auto hash = ptr_t<uchar>( 32, '\0' ); argon2_t argon; argon.hash( hash,pass );
        return encoder::hex::get( hash );
    }

}}}

/*────────────────────────────────────────────────────────────────────────────*/

#endif