

/*
 * Copyright (C) Weibin Yao
 * Email: yaoweibin@gmail.com
 * Version: $Id$
 *
 * This module is modified from Nginx's upstream_ip_hash module and 
 * Evan Miller's upstream_hash module. Thanks for them.
 *
 * This module can be distributed under the same terms as Nginx itself.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    /* the round robin data must be first */
    ngx_http_upstream_rr_peer_data_t   rrp;

    ngx_str_t                          cookie;

    struct sockaddr                   *sockaddr;
    socklen_t                          socklen;

    u_char                             tries;

    ngx_event_get_peer_pt              get_rr_peer;
} ngx_http_upstream_jvm_route_peer_data_t;

static ngx_int_t ngx_http_upstream_init_jvm_route_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);
static ngx_int_t ngx_http_upstream_get_jvm_route_peer(ngx_peer_connection_t *pc,
    void *data);
static char *ngx_http_upstream_jvm_route(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);


static ngx_command_t  ngx_http_upstream_jvm_route_commands[] = {

    { ngx_string("jvm_route"),
      NGX_HTTP_UPS_CONF|NGX_CONF_TAKE12,
      ngx_http_upstream_jvm_route,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_upstream_jvm_route_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_upstream_jvm_route_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_jvm_route_module_ctx, /* module context */
    ngx_http_upstream_jvm_route_commands,    /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


/* string1 compares with the string2 in reverse order.*/
static ngx_int_t
ngx_strncmp_r(u_char *s1, u_char *s2, size_t len1, size_t len2)
{
    if (len2 == 0 || len1 == 0) {
        return -1;
    }

    while (s1[--len1] == s2[--len2]) {
        if (len2 == 0 || len1 == 0) {
            return 0;
        }
    }

    return s1[len1] - s2[len2];
}

static ngx_int_t
ngx_http_upstream_init_jvm_route(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    us->peer.init = ngx_http_upstream_init_jvm_route_peer;

    return NGX_OK;
}

static ngx_int_t
ngx_http_upstream_jvm_route_get_socket(ngx_http_request_t *r,
        ngx_http_upstream_jvm_route_peer_data_t *jrp,
        ngx_http_upstream_srv_conf_t *us)
{
    ngx_uint_t                     i = 0;
    ngx_http_upstream_server_t    *server;

    if (us->servers) {
        server = us->servers->elts;

        for (i = 0; i < us->servers->nelts; i++) {
            /* TODO: Include the backup server and the servers with more than 
               one IP.*/
            if (server[i].backup) {
                continue;
            }

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "jvm_route# srun_id: %V", &server[i].srun_id);

            if (us->reverse) {
                if (ngx_strncmp_r(jrp->cookie.data, server[i].srun_id.data,
                            jrp->cookie.len, server[i].srun_id.len) == 0){
                    break;
                }
            }
            else {
                if (ngx_strncmp(jrp->cookie.data, server[i].srun_id.data,
                            server[i].srun_id.len) == 0){
                    break;
                }
            }
        }
        
        if (i < us->servers->nelts) {
            jrp->sockaddr = server[i].addrs[0].sockaddr;
            jrp->socklen = server[i].addrs[0].socklen;
        }
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_upstream_init_jvm_route_peer(ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_str_t                                 val;
    ngx_http_upstream_jvm_route_peer_data_t  *jrp;

    if (ngx_http_script_run(r, &val, us->lengths, 0, us->values) == NULL) {
        return NGX_ERROR;
    }

    jrp = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_jvm_route_peer_data_t));
    if (jrp == NULL) {
        return NGX_ERROR;
    }

    r->upstream->peer.data = &jrp->rrp;

    if (ngx_http_upstream_init_round_robin_peer(r, us) != NGX_OK) {
        return NGX_ERROR;
    }

    r->upstream->peer.get = ngx_http_upstream_get_jvm_route_peer;

    jrp->cookie = val;
    jrp->tries = 0;
    jrp->get_rr_peer = ngx_http_upstream_get_round_robin_peer;

    if (jrp->cookie.len > 0) {
        ngx_http_upstream_jvm_route_get_socket(r, jrp, us);
    }


    return NGX_OK;
}


static ngx_int_t
ngx_http_upstream_get_jvm_route_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_upstream_jvm_route_peer_data_t  *jrp = data;

    time_t                        now;
    uintptr_t                     m = 0;
    ngx_uint_t                    n = 0, p = 0;
    ngx_str_t                     cookie;
    ngx_http_upstream_rr_peer_t  *peer= NULL;

#if (NGX_DEBUG)
    u_char                        addr[NGX_INET_ADDRSTRLEN] = {0};

    if (jrp->socklen > 0) {
        ngx_sock_ntop(jrp->sockaddr, addr, NGX_INET_ADDRSTRLEN, 0);
    }
    
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                   "get jvm_route peer, try: %ui, cookie: \"%V\", addr: %s",
                   pc->tries, &jrp->cookie, addr);
#endif

    /* TODO: cached */

    if (jrp->cookie.len == 0 || jrp->socklen == 0 
            || jrp->tries > 20 || jrp->rrp.peers->single) {
        return jrp->get_rr_peer(pc, &jrp->rrp);
    }

    now = ngx_time();

    pc->cached = 0;
    pc->connection = NULL;

    cookie = jrp->cookie;

    for (p = 0; p < jrp->rrp.peers->number; p++ ) {
        peer = &jrp->rrp.peers->peer[p];

        if (jrp->sockaddr != peer->sockaddr) {
            continue;
        }

        n = p / (8 * sizeof(uintptr_t));
        m = (uintptr_t) 1 << p % (8 * sizeof(uintptr_t));

        if (!(jrp->rrp.tried[n] & m)) {
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                    "get jvm_route peer, index: %ui %04XA", p, m);

            /* ngx_lock_mutex(jrp->rrp.peers->mutex); */

            if (!peer->down) {

                if (peer->max_fails == 0 || peer->fails < peer->max_fails) {
                    break;
                }

                if (now - peer->accessed > peer->fail_timeout) {
                    peer->fails = 0;
                    break;
                }
            }

            jrp->rrp.tried[n] |= m;

            /* ngx_unlock_mutex(jrp->rrp.peers->mutex); */

            pc->tries--;
        }

        return jrp->get_rr_peer(pc, &jrp->rrp);
    }

    if (peer == NULL) {
        return jrp->get_rr_peer(pc, &jrp->rrp);
    } 

    jrp->rrp.current = p;

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    /* ngx_unlock_mutex(jrp->rrp.peers->mutex); */

    jrp->rrp.tried[n] |= m;

    return NGX_OK;
}


static char *
ngx_http_upstream_jvm_route(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_upstream_srv_conf_t        *uscf;
    ngx_http_script_compile_t            sc;
    ngx_str_t                           *value;
    ngx_array_t                         *vars_lengths, *vars_values;

    value = cf->args->elts;

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));

    vars_lengths = NULL;
    vars_values = NULL;

    sc.cf = cf;
    sc.source = &value[1];
    sc.lengths = &vars_lengths;
    sc.values = &vars_values;
    sc.complete_lengths = 1;
    sc.complete_values = 1;

    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts > 2) {
        if (ngx_strncmp(value[2].data, "reverse", 7) == 0 ) {
            uscf->reverse = 1;
        }
    }

    uscf->values = vars_values->elts;
    uscf->lengths = vars_lengths->elts;


    uscf->peer.init_upstream = ngx_http_upstream_init_jvm_route;

    uscf->flags = NGX_HTTP_UPSTREAM_CREATE 
        | NGX_HTTP_UPSTREAM_MAX_FAILS
        | NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
        | NGX_HTTP_UPSTREAM_SRUN_ID
        | NGX_HTTP_UPSTREAM_DOWN;

    return NGX_CONF_OK;
}
