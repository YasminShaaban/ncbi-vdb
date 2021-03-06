/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include <kfg/config.h> /* KConfigRelease */
#include <kns/kns-mgr-priv.h> /* KNSManagerMakeConfig */
#include <kns/manager.h> /* KNSManagerRelease */
#include <ktst/unit_test_suite.hpp> // TestCase

#include "test-proxy.h" // KNSManagerMakeKEndPointArgsIterator

#define RELEASE( type, obj ) do { rc_t rc2 = type##Release ( obj ); \
    if (rc2 != 0 && rc == 0) { rc = rc2; } obj = NULL; } while ( false )

struct C { // Configuration Helper
    C ( const std::string & aPath, const std::string & aValue )
        : path ( aPath )
        , value ( aValue )
        , next ( NULL)
    {}

    ~ C ( void )
    {
        delete next;
        next = NULL;
    }

    void add ( const std::string & path, const std::string & value )
    {
        C * p = this;
        while ( p -> next != NULL) {
            p = p -> next;
        }

        p -> next = new C ( path, value );
    }

    bool contains ( const std::string & name, const std::string & aValue ) const
    {
        const C * c = this;
        while ( c ) {
            if ( name == c -> path && aValue == c -> value ) {
                return true;
            }
            c = c -> next;
        }

        return false;
    }

    const std::string path;
    const std::string value;

    C * next;
};

struct E { // Expected Proxy Values
    E ( const std::string & aPath, uint16_t aPort )
        : path ( aPath )
        , port ( aPort )
        , next ( NULL )
    {}

    ~ E ( void )
    {
        delete next;
        next = NULL;
    }

    void add ( const std::string & path, uint16_t port )
    {
        E * p = this;
        while ( p -> next != NULL) {
            p = p -> next;
        }

        p -> next = new E ( path, port );
    }

    const std::string path;
    const uint16_t port;

    E * next;
};

class CKConfig {
    KConfig * _self;

public:
    CKConfig ( const C * c = NULL ) : _self (NULL) {
        rc_t rc = KConfigMake ( & _self, NULL );
        if ( rc != 0 ) {
            throw rc;
        }

        while ( c ) {
            rc_t rc = KConfigWriteString
                ( _self, c -> path . c_str (), c -> value . c_str () );
            if ( rc != 0 ) {
                throw rc;
            }

            c = c -> next;
        }
    }

    ~CKConfig ( void ) {
        KConfigRelease ( _self );
        _self = NULL;
    }

    KConfig * get ( void ) const {
        return _self;
    }
};

class TestProxy : private ncbi::NK::TestCase {
    static bool _StringEqual ( const String * a, const String * b ) {
        assert ( a && b );
        return ( ( a ) -> len == ( b ) -> len &&
            memcmp ( ( a ) -> addr, ( b ) -> addr, ( a ) -> len ) == 0 );
    }
    TestCase * _dad;
    CKConfig _kfg;
    void testProxies ( const KNSManager * mgr, const E * e ) {
        const HttpProxy * p = KNSManagerGetHttpProxy ( mgr );
        if ( p != NULL && e == NULL) {
            REQUIRE ( ! p );
        }
        while ( e ) {
            const String * http_proxy = NULL;
            uint16_t http_proxy_port = 0;
            HttpProxyGet ( p, & http_proxy, & http_proxy_port );
            REQUIRE ( http_proxy );
            std::string proxy ( http_proxy -> addr, http_proxy -> len );
            REQUIRE_EQ ( e -> path, proxy );
            REQUIRE_EQ ( e -> port, http_proxy_port );
            e = e -> next;
            p = HttpProxyGetNextHttpProxy ( p );
            REQUIRE ( ( e && p ) || ( ! e && ! p ) ); 
        }
    }
    void testEndPoints ( const KNSManager * mgr, const E * e, const C * c ) {
        String aHost;
        CONST_STRING ( & aHost, "a.host.domain" );
        uint16_t aPort = 8976;
        struct KEndPointArgsIterator * i
            = KNSManagerMakeKEndPointArgsIterator ( mgr, & aHost, aPort );
        REQUIRE ( i );
        const String * hostname = NULL;
        uint16_t port = ~ 0;
        bool proxy_default_port = true;
        bool proxy_ep = false;
        while ( e ) {
            REQUIRE ( KEndPointArgsIterator_Next ( i,
                 & hostname, & port, & proxy_default_port, & proxy_ep ) );
            REQUIRE ( proxy_ep );
            String host;
            StringInit ( & host,
                e -> path. c_str (), e -> path. size (), e -> path. size () );
            REQUIRE ( StringEqual ( hostname, & host ) );
            if ( e -> port != 0 ) {
                REQUIRE_EQ ( port, e -> port );
                REQUIRE ( ! proxy_default_port );
            } else {
                REQUIRE_EQ ( static_cast < int> ( port), 3128 );
                REQUIRE ( proxy_default_port );
                REQUIRE ( KEndPointArgsIterator_Next ( i, & hostname,
                             & port, & proxy_default_port, & proxy_ep ) );
                REQUIRE ( proxy_ep );
                REQUIRE ( StringEqual ( hostname, & host ) );
                REQUIRE_EQ ( static_cast < int> ( port ), 8080 );
                REQUIRE ( proxy_default_port );
            }
            e = e -> next;
        }
        if ( c == NULL || ! c -> contains ( "/http/proxy/only", "true" ) ) {
            REQUIRE (     KEndPointArgsIterator_Next ( i, & hostname,
                             & port, & proxy_default_port, & proxy_ep ) );
            REQUIRE ( ! proxy_ep );
            REQUIRE ( _StringEqual ( hostname, & aHost ) );
            REQUIRE_EQ ( port, aPort );
            REQUIRE ( ! proxy_default_port );
        }
        REQUIRE ( !   KEndPointArgsIterator_Next ( i,
                 & hostname, & port, & proxy_default_port, & proxy_ep ) );
    }
public:
    TestProxy
            ( TestCase * dad, const C * c = NULL, const E * e = NULL )
        : TestCase ( dad -> GetName () ), _dad ( dad ), _kfg ( c )
    {
        rc_t rc = 0;
        KNSManager * mgr = NULL;
        REQUIRE_RC ( KNSManagerMakeConfig ( & mgr, _kfg . get () ) );
        testProxies   ( mgr, e );
        testEndPoints ( mgr, e, c );
        RELEASE ( KNSManager, mgr );
        REQUIRE_RC ( rc );
    }
    ~ TestProxy ( void )
    {   assert ( _dad ); _dad -> ErrorCounterAdd ( GetErrorCounter () ); }
};

////////////////////////////////////////////////////////////////////////////////
