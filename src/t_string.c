/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"
#include "dict.h"
#ifdef USE_PMEM
#include "libpmemobj.h"
#endif
#include <math.h> /* isnan(), isinf() */
#include <libpmemobj/base.h>

double times[200000];
double latency_times[10000001];
int times_count = 0;
int set_count = 0;
clock_t set_start;
/*-----------------------------------------------------------------------------
 * String Commands
 *----------------------------------------------------------------------------*/

static int checkStringLength(client *c, long long size) {
    if (size > 512*1024*1024) {
        addReplyError(c,"string exceeds maximum allowed size (512MB)");
        return C_ERR;
    }
    return C_OK;
}
/* The setGenericCommand() function implements the SET operation with different
 * options and variants. This function is called in order to implement the
 * following commands: SET, SETEX, PSETEX, SETNX.
 *
 * 'flags' changes the behavior of the command (NX or XX, see belove).
 *
 * 'expire' represents an expire to set in form of a Redis object as passed
 * by the user. It is interpreted according to the specified 'unit'.
 *
 * 'ok_reply' and 'abort_reply' is what the function will reply to the client
 * if the operation is performed, or when it is not because of NX or
 * XX flags.
 *
 * If ok_reply is NULL "+OK" is used.
 * If abort_reply is NULL, "$-1" is used. */

#define OBJ_SET_NO_FLAGS 0
#define OBJ_SET_NX (1<<0)     /* Set if key not exists. */
#define OBJ_SET_XX (1<<1)     /* Set if key exists. */
#define OBJ_SET_EX (1<<2)     /* Set if time in seconds is given */
#define OBJ_SET_PX (1<<3)     /* Set if time in ms in given */

void setGenericCommand(client *c, int flags, robj *key, robj *val, robj *expire, int unit, robj *ok_reply, robj *abort_reply) {
	FILE *out_file;
	if(set_count == 0){
		set_start = clock();
	}
	else if (set_count == 9999999){
		out_file = fopen("throughput.txt", "w+");
	}
	clock_t start = clock();
    //clock_t start = clock();
    long long milliseconds = 0; /* initialized to avoid any harmness warning */
#ifdef USE_PMEM
    robj* newVal = 0;
    robj* newKey = 0;
#endif
    if (expire) {
        if (getLongLongFromObjectOrReply(c, expire, &milliseconds, NULL) != C_OK)
            return;
        if (milliseconds <= 0) {
            addReplyErrorFormat(c,"invalid expire time in %s",c->cmd->name);
            return;
        }
        if (unit == UNIT_SECONDS) milliseconds *= 1000;
    }

    if ((flags & OBJ_SET_NX && lookupKeyWrite(c->db,key) != NULL) ||
        (flags & OBJ_SET_XX && lookupKeyWrite(c->db,key) == NULL))
    {
        addReply(c, abort_reply ? abort_reply : shared.null[c->resp]);
        return;
    }
#ifdef USE_PMEM
        /* Copy value from RAM to PM - create RedisObject and sds(value) */
        /* TODO: Remove bookKeeper */
            struct bookKeeper *book = zmalloc(sizeof(struct bookKeeper));
        //clock_t end = clock();
        //double time_here = (double)(end - set_start) / CLOCKS_PER_SEC;
        //printf("Beginning of setKeypm %f\n", time_here);
        //start = clock();
        TX_BEGIN (server.pm_pool) {

            book->dict_offset = -1;
        /* TODO: Remove the last argument from dupStringObjectPM */
            newVal = dupStringObjectPM(val, book, false, &c->db->dict->ht[0]);
	    //char temp[3] = "ab";
            newKey = dupStringObjectPM(key, book, true, &c->db->dict->ht[0]);
            //end = clock();
            //time_here = (double)(end - set_start) / CLOCKS_PER_SEC;
            //printf("Dup String ObjectPM %f\n", time_here);
            //start = clock();
		//memcpy(newVal->ptr, "ab", 2);
		//printf("%s\n", newVal->ptr);
            /* Set key in PM - create DictEntry and sds(key) linked to RedisObject with value
             * Don't increment value "ref counter" as in normal process. */
            //printf("%d\n", newVal->type);
            //pmemobj_tx_add_range_direct(newVal, (sizeof(robj)));
            //pmemobj_tx_add_range_direct(newKey, (sizeof(robj)));
            //newKey->type = OBJ_LIST;
            //newVal->type = OBJ_LIST;
            setKeyPM(c->db, newKey,newVal, book);
            //end = clock();
            //time_here = (double)(end - set_start) / CLOCKS_PER_SEC;
            //printf("setKeyPM %f\n", time_here);
            //start = clock();
            if(book->dict_offset == -1) {
                // This means that this was a value replacement command
                sdsfreePM(newKey->ptr);
                // free r_object
                PMEMoid oid;
                oid.off = book->sds_robj_offset;
                oid.pool_uuid_lo = server.pool_uuid;
                pmemobj_tx_free(oid);
		set_count++;
            } else {
                char* actual= book->val_offset + (uint64_t)server.pm_pool;
		set_count++;
            }
        } TX_ONABORT {
            printf("\n\n\n\n\n\ +++*********** Set command aborted, good luck finding that out \n\n\n\n\n\n\n");
        } TX_ONCOMMIT {
	       clock_t end = clock();

	    if(set_count % 100 == 0){
	    	times[times_count] = (double)(end - set_start) / CLOCKS_PER_SEC;
	    	times_count++;
	    }
		if(set_count == 10000000){
                printf("BEGIN\n");
                double total = 0;
                for(int ind = 1; ind < times_count; ind++){
                       	 	fprintf(out_file, "%f\n", 100/(times[ind] - (times[ind - 1])));
				total += 100/(times[ind] - (times[ind - 1]));
                	}
                	fclose(out_file);
			printf("END\n");
			printf("total avg is %f\n", total / times_count);
                	//fclose(out_file);
          	}
		/*double time_here = (double)(end - start)/CLOCKS_PER_SEC;
		latency_times[set_count - 1] = time_here;
	 	if(set_count == 1000000){
			for(int ind = 0; ind < 1000000; ind++){
				fprintf(out_file, "%f\n", latency_times[ind]*1000000);
			}
			fclose(out_file);
			//double total = 0;
                        //double avg_latency = 0;
                        //for(int ind = 0; ind < 50000000; ind++){
                        //        total += latency_times[ind];
                                //fprintf(out_file, "%f\n", latency_times[ind]);
                        //}
                        //avg_latency = total/50000000;
                        //printf("average latency %.10lf\n", avg_latency);
		}*/
            //if(set_count >= 50000000){
	//	for(int ind = 0; ind < times_count; ind++){
	//	fprintf(out_file, "%d,%f\n", ind * 100, times[ind]); 
	  //   }
	 //   }
        } TX_FINALLY {
            zfree(book);
        } TX_END
        TX_BEGIN(server.pm_pool){
            pmemobj_tx_add_range_direct(newVal, (sizeof(robj)));
            //pmemobj_tx_add_range_direct(newKey, (sizeof(robj)));
        }TX_END
        //end = clock();
            //time_here = (double)(end - set_start) / CLOCKS_PER_SEC;
            //printf("end of transaction %f\n", time_here);
            //start = clock();
#else
    setKey(c->db,key,val);
#endif
#ifdef USE_PMEM
    server.dirty++;
    if (expire) setExpire(c,c->db,newKey,mstime()+milliseconds);
    // QUESTION IS DO I NEED TO CHANGE THIS BACK TO KEY
    notifyKeyspaceEvent(NOTIFY_STRING,"set",newKey,c->db->id);
    if (expire) notifyKeyspaceEvent(NOTIFY_GENERIC,
        "expire",newKey,c->db->id);
#else
    server.dirty++;
    if (expire) setExpire(c,c->db,key,mstime()+milliseconds);
    notifyKeyspaceEvent(NOTIFY_STRING,"set",key,c->db->id);
    if (expire) notifyKeyspaceEvent(NOTIFY_GENERIC,
        "expire",key,c->db->id);
#endif
    addReply(c, ok_reply ? ok_reply : shared.ok);
}

/* SET key value [NX] [XX] [EX <seconds>] [PX <milliseconds>] */
void setCommand(client *c) {
    int j;
    robj *expire = NULL;
    int unit = UNIT_SECONDS;
    int flags = OBJ_SET_NO_FLAGS;

    for (j = 3; j < c->argc; j++) {
        char *a = c->argv[j]->ptr;
        robj *next = (j == c->argc-1) ? NULL : c->argv[j+1];

        if ((a[0] == 'n' || a[0] == 'N') &&
            (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
            !(flags & OBJ_SET_XX))
        {
            flags |= OBJ_SET_NX;
        } else if ((a[0] == 'x' || a[0] == 'X') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_NX))
        {
            flags |= OBJ_SET_XX;
        } else if ((a[0] == 'e' || a[0] == 'E') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_PX) && next)
        {
            flags |= OBJ_SET_EX;
            unit = UNIT_SECONDS;
            expire = next;
            j++;
        } else if ((a[0] == 'p' || a[0] == 'P') &&
                   (a[1] == 'x' || a[1] == 'X') && a[2] == '\0' &&
                   !(flags & OBJ_SET_EX) && next)
        {
            flags |= OBJ_SET_PX;
            unit = UNIT_MILLISECONDS;
            expire = next;
            j++;
        } else {
            addReply(c,shared.syntaxerr);
            return;
        }
    }

    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setGenericCommand(c,flags,c->argv[1],c->argv[2],expire,unit,NULL,NULL);
}

void setnxCommand(client *c) {
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setGenericCommand(c,OBJ_SET_NX,c->argv[1],c->argv[2],NULL,0,shared.cone,shared.czero);
}

void setexCommand(client *c) {
    c->argv[3] = tryObjectEncoding(c->argv[3]);
    setGenericCommand(c,OBJ_SET_NO_FLAGS,c->argv[1],c->argv[3],c->argv[2],UNIT_SECONDS,NULL,NULL);
}

void psetexCommand(client *c) {
    c->argv[3] = tryObjectEncoding(c->argv[3]);
    setGenericCommand(c,OBJ_SET_NO_FLAGS,c->argv[1],c->argv[3],c->argv[2],UNIT_MILLISECONDS,NULL,NULL);
}

int getGenericCommand(client *c) {
    robj *o;
	FILE *out_file;
    //if(set_count == 199999)
//		out_file = fopen("throughput.txt", "w+");


    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.null[c->resp])) == NULL){
    //   set_count++;
    //clock_t end = clock();
      //      if(set_count % 100 == 0){
        //        times[times_count] = (double)(end - set_start) / CLOCKS_PER_SEC;
         //       times_count++;
          //  }
 //if(set_count >= 200000){
   //             for(int ind = 0; ind <= times_count; ind++){
     //           fprintf(out_file, "%d,%f\n", ind * 100, times[ind]); 
       //      }
          //  }  
 
      return C_OK;
    }
    if (o->type != OBJ_STRING) {
        addReply(c,shared.wrongtypeerr);
    //set_count++;
    //clock_t end = clock();
      //      if(set_count % 100 == 0){
       //         times[times_count] = (double)(end - set_start) / CLOCKS_PER_SEC;
       //         times_count++;
       //     }
	// if(set_count >= 200000){
          //      for(int ind = 0; ind <= times_count; ind++){
          //      fprintf(out_file, "%d,%f\n", ind * 100, times[ind]); 
          //   }
            //}  

        return C_ERR;
    } else {
        addReplyBulk(c,o);
    /*set_count++;
    clock_t end = clock();
            if(set_count % 100 == 0){
                times[times_count] = (double)(end - set_start) / CLOCKS_PER_SEC;
                times_count++;
            }
 if(set_count >= 200000){
                for(int ind = 0; ind <= times_count; ind++){
                fprintf(out_file, "%d,%f\n", ind * 100, times[ind]); 
             }
            }  */


        return C_OK;
    }
}

void getCommand(client *c) {
    getGenericCommand(c);
}

void getsetCommand(client *c) {
    if (getGenericCommand(c) == C_ERR) return;
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setKey(c->db,c->argv[1],c->argv[2]);
    notifyKeyspaceEvent(NOTIFY_STRING,"set",c->argv[1],c->db->id);
    server.dirty++;
}

void setrangeCommand(client *c) {
    //printf("RANGE \n");
    robj *o;
    long offset;
    sds value = c->argv[3]->ptr;

    if (getLongFromObjectOrReply(c,c->argv[2],&offset,NULL) != C_OK)
        return;

    if (offset < 0) {
        addReplyError(c,"offset is out of range");
        return;
    }

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o == NULL) {
        /* Return 0 when setting nothing on a non-existing string */
        if (sdslen(value) == 0) {
            addReply(c,shared.czero);
            return;
        }

        /* Return when the resulting string exceeds allowed size */
        if (checkStringLength(c,offset+sdslen(value)) != C_OK)
            return;

        o = createObject(OBJ_STRING,sdsnewlen(NULL, offset+sdslen(value)));
        dbAdd(c->db,c->argv[1],o);
    } else {
        size_t olen;

        /* Key exists, check type */
        if (checkType(c,o,OBJ_STRING))
            return;

        /* Return existing string length when setting nothing */
        olen = stringObjectLen(o);
        if (sdslen(value) == 0) {
            addReplyLongLong(c,olen);
            return;
        }

        /* Return when the resulting string exceeds allowed size */
        if (checkStringLength(c,offset+sdslen(value)) != C_OK)
            return;

        /* Create a copy when the object is shared or encoded. */
        o = dbUnshareStringValue(c->db,c->argv[1],o);
    }

    if (sdslen(value) > 0) {
        o->ptr = sdsgrowzero(o->ptr,offset+sdslen(value));
        memcpy((char*)o->ptr+offset,value,sdslen(value));
        signalModifiedKey(c->db,c->argv[1]);
        notifyKeyspaceEvent(NOTIFY_STRING,
            "setrange",c->argv[1],c->db->id);
        server.dirty++;
    }
    addReplyLongLong(c,sdslen(o->ptr));
}

void getrangeCommand(client *c) {
    robj *o;
    long long start, end;
    char *str, llbuf[32];
    size_t strlen;

    if (getLongLongFromObjectOrReply(c,c->argv[2],&start,NULL) != C_OK)
        return;
    if (getLongLongFromObjectOrReply(c,c->argv[3],&end,NULL) != C_OK)
        return;
    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.emptybulk)) == NULL ||
        checkType(c,o,OBJ_STRING)) return;

    if (o->encoding == OBJ_ENCODING_INT) {
        str = llbuf;
        strlen = ll2string(llbuf,sizeof(llbuf),(long)o->ptr);
    } else {
        str = o->ptr;
        strlen = sdslen(str);
    }

    /* Convert negative indexes */
    if (start < 0 && end < 0 && start > end) {
        addReply(c,shared.emptybulk);
        return;
    }
    if (start < 0) start = strlen+start;
    if (end < 0) end = strlen+end;
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if ((unsigned long long)end >= strlen) end = strlen-1;

    /* Precondition: end >= 0 && end < strlen, so the only condition where
     * nothing can be returned is: start > end. */
    if (start > end || strlen == 0) {
        addReply(c,shared.emptybulk);
    } else {
        addReplyBulkCBuffer(c,(char*)str+start,end-start+1);
    }
}

void mgetCommand(client *c) {
    int j;

    addReplyArrayLen(c,c->argc-1);
    for (j = 1; j < c->argc; j++) {
        robj *o = lookupKeyRead(c->db,c->argv[j]);
        if (o == NULL) {
            addReplyNull(c);
        } else {
            if (o->type != OBJ_STRING) {
                addReplyNull(c);
            } else {
                addReplyBulk(c,o);
            }
        }
    }
}

void msetGenericCommand(client *c, int nx) {
    clock_t start = clock();
    int j;

    if ((c->argc % 2) == 0) {
        addReplyError(c,"wrong number of arguments for MSET");
        return;
    }

    /* Handle the NX flag. The MSETNX semantic is to return zero and don't
     * set anything if at least one key alerady exists. */
    if (nx) {
        for (j = 1; j < c->argc; j += 2) {
            if (lookupKeyWrite(c->db,c->argv[j]) != NULL) {
                addReply(c, shared.czero);
                return;
            }
        }
    }

    for (j = 1; j < c->argc; j += 2) {
        c->argv[j+1] = tryObjectEncoding(c->argv[j+1]);


#ifdef USE_PMEM
        robj* newVal = 0;
        robj* newKey = 0;
        struct bookKeeper *book = zmalloc(sizeof(struct bookKeeper));
        TX_BEGIN (server.pm_pool) {

            book->dict_offset = -1;
            newVal = dupStringObjectPM(c->argv[j+1], book, false, &c->db->dict->ht[0]);
            newKey = dupStringObjectPM(c->argv[j]  , book, true, &c->db->dict->ht[0]);
            /* Set key in PM - create DictEntry and sds(key) linked to RedisObject with value
             * Don't increment value "ref counter" as in normal process. */
            //printf("Two pointers are %p %p \n", book->sds_robj_offset, book->val_robj_offset );
            setKeyPM(c->db, newKey,newVal, book);

            if(book->dict_offset == -1) {
                // This means that this was a value replacement command
                // free sds string
                sdsfreePM(newKey->ptr);
                // free r_object
                PMEMoid oid;
                oid.off = book->sds_robj_offset;
                oid.pool_uuid_lo = server.pool_uuid;
                pmemobj_tx_free(oid);
            } else {
                char* actual= book->val_offset + (uint64_t)server.pm_pool;
            }

        } TX_ONABORT {
            printf("Set command aborted, good luck finding that out \n");
        } TX_ONCOMMIT {
	  //printf("Set command successfully committed\n");
        } TX_FINALLY {
            zfree(book);
        } TX_END
        #else
        setKey(c->db,c->argv[j],c->argv[j+1]);
        #endif
        notifyKeyspaceEvent(NOTIFY_STRING,"set",c->argv[j],c->db->id);
    }
    server.dirty += (c->argc-1)/2; 
    clock_t end = clock();
    fprintf(stderr, "Time spent: %lf second\n", (double)(end - start) / CLOCKS_PER_SEC );
    fprintf(stderr, "Rehash Time was : %lf second \n", total_time);
    addReply(c, nx ? shared.cone : shared.ok);
}

void msetCommand(client *c) {
    msetGenericCommand(c,0);
}

void msetnxCommand(client *c) {
    msetGenericCommand(c,1);
}

void incrDecrCommand(client *c, long long incr) {
    long long value, oldvalue;
    robj *o, *new;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o != NULL && checkType(c,o,OBJ_STRING)) return;
    if (getLongLongFromObjectOrReply(c,o,&value,NULL) != C_OK) return;

    oldvalue = value;
    if ((incr < 0 && oldvalue < 0 && incr < (LLONG_MIN-oldvalue)) ||
        (incr > 0 && oldvalue > 0 && incr > (LLONG_MAX-oldvalue))) {
        addReplyError(c,"increment or decrement would overflow");
        return;
    }
    value += incr;

    if (o && o->refcount == 1 && o->encoding == OBJ_ENCODING_INT &&
        (value < 0 || value >= OBJ_SHARED_INTEGERS) &&
        value >= LONG_MIN && value <= LONG_MAX)
    {
        new = o;
        o->ptr = (void*)((long)value);
    } else {
        new = createStringObjectFromLongLongForValue(value);
        if (o) {
            dbOverwrite(c->db,c->argv[1],new);
        } else {
            dbAdd(c->db,c->argv[1],new);
        }
    }
    signalModifiedKey(c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_STRING,"incrby",c->argv[1],c->db->id);
    server.dirty++;
    addReply(c,shared.colon);
    addReply(c,new);
    addReply(c,shared.crlf);
}

void incrCommand(client *c) {
    incrDecrCommand(c,1);
}

void decrCommand(client *c) {
    incrDecrCommand(c,-1);
}

void incrbyCommand(client *c) {
    long long incr;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &incr, NULL) != C_OK) return;
    incrDecrCommand(c,incr);
}

void decrbyCommand(client *c) {
    long long incr;

    if (getLongLongFromObjectOrReply(c, c->argv[2], &incr, NULL) != C_OK) return;
    incrDecrCommand(c,-incr);
}

void incrbyfloatCommand(client *c) {
    long double incr, value;
    robj *o, *new, *aux;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o != NULL && checkType(c,o,OBJ_STRING)) return;
    if (getLongDoubleFromObjectOrReply(c,o,&value,NULL) != C_OK ||
        getLongDoubleFromObjectOrReply(c,c->argv[2],&incr,NULL) != C_OK)
        return;

    value += incr;
    if (isnan(value) || isinf(value)) {
        addReplyError(c,"increment would produce NaN or Infinity");
        return;
    }
    new = createStringObjectFromLongDouble(value,1);
    if (o)
        dbOverwrite(c->db,c->argv[1],new);
    else
        dbAdd(c->db,c->argv[1],new);
    signalModifiedKey(c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_STRING,"incrbyfloat",c->argv[1],c->db->id);
    server.dirty++;
    addReplyBulk(c,new);

    /* Always replicate INCRBYFLOAT as a SET command with the final value
     * in order to make sure that differences in float precision or formatting
     * will not create differences in replicas or after an AOF restart. */
    aux = createStringObject("SET",3);
    rewriteClientCommandArgument(c,0,aux);
    decrRefCount(aux);
    rewriteClientCommandArgument(c,2,new);
}

void appendCommand(client *c) {
    size_t totlen;
    robj *o, *append;

    o = lookupKeyWrite(c->db,c->argv[1]);
    if (o == NULL) {
        /* Create the key */
        c->argv[2] = tryObjectEncoding(c->argv[2]);
        dbAdd(c->db,c->argv[1],c->argv[2]);
        incrRefCount(c->argv[2]);
        totlen = stringObjectLen(c->argv[2]);
    } else {
        /* Key exists, check type */
        if (checkType(c,o,OBJ_STRING))
            return;

        /* "append" is an argument, so always an sds */
        append = c->argv[2];
        totlen = stringObjectLen(o)+sdslen(append->ptr);
        if (checkStringLength(c,totlen) != C_OK)
            return;

        /* Append the value */
        o = dbUnshareStringValue(c->db,c->argv[1],o);
        o->ptr = sdscatlen(o->ptr,append->ptr,sdslen(append->ptr));
        totlen = sdslen(o->ptr);
    }
    signalModifiedKey(c->db,c->argv[1]);
    notifyKeyspaceEvent(NOTIFY_STRING,"append",c->argv[1],c->db->id);
    server.dirty++;
    addReplyLongLong(c,totlen);
}

void strlenCommand(client *c) {
    robj *o;
    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.czero)) == NULL ||
        checkType(c,o,OBJ_STRING)) return;
    addReplyLongLong(c,stringObjectLen(o));
}
