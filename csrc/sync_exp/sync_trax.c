/**
 * @file csrc/sync_trax.c
 *
 * Part of CCNx Sync.
 *
 * Copyright (C) 2012 Palo Alto Research Center, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details. You should have received
 * a copy of the GNU Lesser General Public License along with this library;
 * if not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* The following line for MacOS is a simple custom build of this file
 
 gcc -c -I.. -I../.. -I../../include sync_trax.c
 
 */

/* The following line for MacOS is custom to avoid conflicts with libccn and libsync.
 * It is derived from ccnx/csrc/lib/dir.mk
 
 gcc -g -o sync_trax -I. -I.. -I../.. -I../../include sync_trax.c sync_diff.c ccn_sync.c IndexSorter.c SyncBase.c SyncHashCache.c SyncNode.c SyncRoot.c SyncTreeWorker.c SyncUtil.c  ../lib/{ccn_client,ccn_charbuf,ccn_indexbuf,ccn_coding,ccn_dtag_table,ccn_schedule,ccn_extend_dict,ccn_buf_decoder,ccn_uri,ccn_buf_encoder,ccn_bloom,ccn_name_util,ccn_face_mgmt,ccn_reg_mgmt,ccn_digest,ccn_interest,ccn_keystore,ccn_seqwriter,ccn_signing,ccn_sockcreate,ccn_traverse,ccn_match,hashtb,ccn_merkle_path_asn1,ccn_sockaddrutil,ccn_setup_sockaddr_un,ccn_bulkdata,ccn_versioning,ccn_header,ccn_fetch,ccn_btree,ccn_btree_content,ccn_btree_store}.o -lcrypto
 
 */

#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include <ccn/ccn.h>
#include <ccn/ccn_private.h>
#include <ccn/charbuf.h>
#include <ccn/uri.h>

#include <sync/SyncMacros.h>
#include <sync/SyncBase.h>
#include <sync/SyncNode.h>
#include <sync/SyncPrivate.h>
#include <sync/SyncUtil.h>
#include <sync/sync.h>
#include <sync/sync_depends.h>
#include <sync/sync_diff.h>

// types

struct parms {
    struct ccn_charbuf *topo;
    struct ccn_charbuf *prefix;
    struct SyncHashCacheEntry *last_ce;
    struct SyncHashCacheEntry *next_ce;
    struct SyncNameAccum *excl;
    struct SyncNameAccum *namesToAdd;
    struct SyncHashInfoList *hashSeen;
    int debug;
    struct ccn *ccn;
    int skipToHash;
    struct ccn_scheduled_event *ev;
    struct sync_diff_fetch_data *fd;
    struct sync_diff_data *sdd;
    struct sync_update_data *ud;
    int scope;
    int fetchLifetime;
    int needUpdate;
    int64_t add_accum;
    int64_t startTime;
    int64_t timeLimit;
};


static int
noteErr2(const char *why, const char *msg) {
    fprintf(stderr, "** ERROR: %s, %s\n", why, msg);
    fflush(stderr);
    return -1;
}

int my_callback(struct ccns_handle *ccns,
                struct ccn_charbuf *lhash,
                struct ccn_charbuf *rhash,
                struct ccn_charbuf *pname) {
    return 0;
}

int
doTest(struct parms *p) {
    char *here = "sync_track.doTest";
    int res = 0;
    p->startTime = SyncCurrentTime();
    if (ccn_connect(p->ccn, NULL) == -1) {
        return noteErr2(here, "could not connect to ccnd");
    }
    
    struct ccns_slice *slice = ccns_slice_create();
    ccns_slice_set_topo_prefix(slice, p->topo, p->prefix);
    
    
    struct ccns_handle *ch = ccns_open(p->ccn, slice, my_callback, NULL, NULL);
    
    ccns_close(&ch, NULL, NULL);
    ccns_slice_destroy(&slice);
    
    return res;
}

int
main(int argc, char **argv) {
    int i = 1;
    int res = 0;
    int seen = 0;
    struct parms ps;
    struct parms *p = &ps;
    
    memset(p, 0, sizeof(*p));
    
    p->topo = ccn_charbuf_create();
    p->prefix = ccn_charbuf_create();
    p->debug = 0;
    p->timeLimit = 60*1000000; // default is one minute (kinda arbitrary)
    p->scope = -1;
    p->fetchLifetime = 4;
    
    while (i < argc && res >= 0) {
        char * sw = argv[i];
        i++;
        char *arg1 = NULL;
        char *arg2 = NULL;
        if (i < argc) arg1 = argv[i];
        if (i+1 < argc) arg2 = argv[i+1];
        if (strcasecmp(sw, "-debug") == 0 || strcasecmp(sw, "-d") == 0) {
            i++;
            p->debug = atoi(arg1);
        } else if (strcasecmp(sw, "-topo") == 0) {
            if (arg1 != NULL) {
                p->topo->length = 0;
                ccn_name_from_uri(p->topo, arg1);
                i++;
                seen++;
            }
        } else if (strcasecmp(sw, "-prefix") == 0) {
            if (arg1 != NULL) {
                p->prefix->length = 0;
                ccn_name_from_uri(p->prefix, arg1);
                i++;
                seen++;
            }
        } else if (strcasecmp(sw, "-secs") == 0) {
            if (arg1 != NULL) {
                int64_t secs = atoi(arg1);
                p->timeLimit = secs * 1000000;
                i++;
            }
        } else {
            noteErr2("invalid switch: ", sw);
            seen = 0;
            break;
        }
    }
    
    if (seen) {
        p->ccn = ccn_create();
        doTest(p);
        if (p->ccn != NULL) {
            ccn_disconnect(p->ccn);
            ccn_destroy(&p->ccn);
        }
    }
}