/* In this file, a library for a simple ISAM (indexed sequential access method)
   file structure is defined.
   -------------------------------------------------------------------------
   Author: G.D. van Albada
   University of Amsterdam
   Faculty of Science
   Informatics Institute
   dick at science.uva.nl
   Version: 0.0
   Date: December 2001 / January 2002
   Goal: Part of an assignment on file system structure for the operating
         systems course.
         It demonstrates many of the administrative and layering structures
     that are also used in normal file systems.

   The basic assumptions are the following:
   1. The file structure should be flexible and allow for different
      key lengths (but keys have to be character strings)
      data field lengths (we allow for only one data field; the user of
         the library can put any (fixed length) data structure in here).
      key and data fields will be stored together in a record. The record
      will contain additional administrative fields:
      state: free, used, deleted
      next: index of next record in sequence
      previous: index of previous record in sequence
      padding: to make the basic record length a multiple of 8 bytes
      The initial file structure will contain Nblocks blocks, each containing
         maximum of NrecPB records, of which at most (NrecPB - 1) are
         initially filled.
         The key of the first record in each block is also used in the
         (fixed) index at the start of the file.
             The index will be a N-ary tree (N = 4), organised in records.
             It will precede the area with the data blocks and will in turn
             be preceded by the information area for the file.
             The last part of the file is the general overflow area, which can
             grow idefinitely, in principle.

      Because the record length is not known at compile time, we have to use
      an approach based on array-indexing and casting etc. To help getting
      this right, we'll define some suitable macros.

      File structure:
      File header - describes contents and field lengths
      ---------------------------------------
      Index records:  ceil(Nblocks / 4) + ceil(Nblocks / 16) + ....
      ---------------------------------------
      Sequential data block area (Nblocks blocks)
      ---------------------------------------
      General overflow area
      ---- EOF --------

      Regarding the search procedure:
      The file is provided with an incomplete index, i.e. not all keys
      that occur in the file will occur in the index. As we will
      (for the sake of simplicity) not update the index, actually no record
      need correspond to the keys in the index (the record may have been
      deleted).
      The index will return the number of the record with the largest index
      not larger than the searched key. Such a record will always be the
      first record of a "block". Now this record may have been deleted
      from the file. There is a large number of ways to handle this so
      that the organisation remains consistent. The simplest way appears
      to just mark this record as deleted, but retain it otherwise. I.e.
      the first record in a block can be deleted, but cannot be overwritten
      with a record with a different key.
      A case in point is the very first record in the file. In order to
      allow for the insertion of records with low key values, this record
      necessarily must have the minimum key value. Luckily, this is well
      defined: it is the empty string "". This record will be created and
      marked "reserved" right at the start.
      When creating the file, every Nth (N = NrecPB - 1) record's key will
      be inserted into the index.
      The index will be a tree where every node has up to four children.
      The leaf nodes point to the records. The reason for not using a more
      sophisticated tree structure are that not all keys searched for need
      appear in the tree.
      So the structure will be:
                                1 17 33 49
                1 5 9 13                        17 ..
      1 2 3 4   5 6 7 8   9 10 11 12   13 14 15 16   17 18 19 20 ....

      The depth of the tree will be determined by Nblocks
        (D = ceil(log4(Nblocks)))
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

/* Use assert to pinpoint fatal errors - should be removed later */
#include <assert.h>
#include "isam.h"
#include "index.h"

/* Just one flag value describing the state of the file for now */

#define ISAM_STATE_UPDATING     (1024)

/* An isam file will start with an information block that is described
   in the following typedef. */

typedef struct {
    unsigned long magic;         /* Make sure this is an isam file   */
    unsigned long version;       /* Allow for later versions         */
    unsigned long Nblocks;       /* Number of regular data blocks    */
    unsigned long NrecPB;        /* Number of records per data block */
    unsigned long KeyLen;        /* Maximum key field length         */
    unsigned long DataLen;       /* Data field length                */
    unsigned long Nrecords;      /* Current number of valid records  */
    unsigned long DataStart;     /* Byte offset of first data record */
    unsigned long RecordLen;     /* Total record length              */
    unsigned long CurBlocks;     /* Current number of data blocks    */
    unsigned long MaxKeyRec;     /* The location of the MaxKey rec.  */
    unsigned long FileState;     /* Flags to indicate file state     */
} fileHead;

/* Occupied record positions come in three tastes, for now */

#define ISAM_VALID        (1)
#define ISAM_DELETED      (2)
#define ISAM_SPECIAL      (4)

/* A record will start with a small header linking it sequentially
   to other records in the file. Next and previous will contain
   record numbers. Records are numbered sequentially throughout the
   file, starting at zero */

typedef struct {
    unsigned long next;
    unsigned long previous;
    unsigned long statusFlags;
} recordHead;

/* When a file is opened some additional information will be stored in
   memory, besides the file header. This includes, e.g. a cache for
   recently used blocks. */

#define CACHE_SIZE  (6)

typedef struct ISAM {
    fileHead fHead;                     /* The file header                */
    unsigned long blockSize;            /* The file datablock size        */
    int     mayWrite;                   /* Unused - opened for read/write */
    int     fileId;                     /* The file-id for the file       */
    int     errorState;                 /* Unused                         */
    int     cur_id;                     /* The cache-slot containing the current record */
    int     cur_recno;                  /* The position in the cache of the current record */
    int     last_in;                    /* For a FIFO cache               */
    int     blockInCache[CACHE_SIZE];   /* block number per cache slot    */
            index_handle index;         /* The handle for the index       */
    char    *cache[CACHE_SIZE];         /* Pointers to cache blocks       */
    char    * maxKey;                   /* The highest key in the file    */
} isam;

/* Starting a file with a magic number provides a simple validity test
   when opening the file (avoid some accidents)                          */

#define isamMagic   (0x15a8f17e)

/* The following macros should simplify the access to the various parts
   of the variable length records in blocks as stored in the cache.
   Apparently the use of key() as a macro does not interfere with the
   use of key as a variable name */

#define head(isam,id,Nrec)  ((recordHead *)((isam).cache[(id)]+\
            (Nrec)*((isam).fHead.RecordLen)))

#define cur_head(isam)      head((isam), (isam).cur_id, (isam).cur_recno)


#define key(isam,id,Nrec)   (((isam).cache[(id)])+\
            (Nrec)*((isam).fHead.RecordLen)+sizeof(recordHead))

#define cur_key(isam)       key((isam), (isam).cur_id, (isam).cur_recno)

#define data(isam,id,Nrec)  ((void *)((isam).cache[(id)]+\
            (Nrec)*((isam).fHead.RecordLen)+sizeof(recordHead)+\
            (isam).fHead.KeyLen))

#define cur_data(isam)      data((isam), (isam).cur_id, (isam).cur_recno)

enum isam_error isam_error = ISAM_NO_ERROR;

int cache_call_global = 0;
int disk_reads_global = 0;
int disk_writes_global = 0;

/* makeIsamPtr creates an isamPtr given a file header, fills in some
   data and initialises the cache */

static isamPtr  makeIsamPtr(fileHead * fHead) {
    isamPtr  ipt = (isamPtr) calloc(1, sizeof(isam));
    int      blockSize;
    int      i;

    assert(ipt != NULL);
    ipt->fHead = *fHead;
    ipt->blockSize = blockSize = fHead->NrecPB * fHead->RecordLen;
    ipt->cache[0] = calloc(CACHE_SIZE, blockSize);
    assert(ipt->cache[0] != NULL);
    ipt->blockInCache[0] = -1;
    for (i = 1; i < CACHE_SIZE; i++) {
        ipt->cache[i] = blockSize + ipt->cache[i - 1];
        ipt->blockInCache[i] = -1;
    }
    return ipt;
}

static void dumpMaxKey(isamPtr f) {
    unsigned int i;
    fprintf(stderr, "Maxkey ='");
    for (i = 0; (i < f->fHead.KeyLen) && (f->maxKey[i]); i++) {
        fprintf(stderr, "%c", f->maxKey[i]);
    }
    fprintf(stderr, "'\n");
}

/* Write the file header to disk (again) */

static int writeHead(isamPtr f) {
    #ifdef DEBUG
    fprintf(stderr,
    "writeHead: Nrecords = %lu DataStart = %lu CurBlocks = %lu FileState = %lu\n",
    f->fHead.Nrecords, f->fHead.DataStart,
    f->fHead.CurBlocks, f->fHead.FileState);
    #endif

    if(lseek(f->fileId, 0, SEEK_SET)) {
        isam_error = ISAM_SEEK_ERROR;
        return -1;
    }

    if(sizeof(fileHead) != write(f->fileId, &(f->fHead), sizeof(fileHead))) {
        isam_error = ISAM_WRITE_FAIL;
        return -1;
    }

    /* STEP 2 INF: This is a good place to record the number of header writes.
    */
    disk_writes_global++;

    return 0;
}

/* You never can predict what junk you get as a file pointer... */

static int testPtr(isamPtr f) {
    if ((!f) || (f->fHead.magic != isamMagic)) {
        isam_error = ISAM_IDENT_INVALID;
        return -1;
    }
    return 0;
}

/* In the remainder of the code, we should not need to worry about how and
   where to write a given block from the cache */
static int write_cache_block(isamPtr isam_ident, int iCache) {
    unsigned long block_no = isam_ident->blockInCache[iCache];
    int rv;

    if (lseek(isam_ident->fileId, isam_ident->fHead.DataStart +
        block_no * isam_ident->blockSize, SEEK_SET) == (off_t)-1) {
        isam_error = ISAM_SEEK_ERROR;
        return -1;
    }
    rv = write(isam_ident->fileId, isam_ident->cache[iCache], isam_ident->blockSize);

    if (rv != (int) isam_ident->blockSize) {
        isam_error = ISAM_WRITE_FAIL;
        return -1;
    }

    /* STEP 2 INF: This is a good place to record the number of block writes. */
    disk_writes_global++;

    return 0;
}

/* See if the requested block is in the cache (could be a data block, or
   a block in the overflow area). If not, load the block. We will
   assume that there are no "dirty" blocks - every block that is
   modified is written to disk right away.
   If the block lies beyond the last block in the file, there is no need
   to read from the file (which should result in an EOF error anyway);
   we just zero the cache block (corresponding to an empty record). */

static int isam_cache_block(isamPtr isam_ident, unsigned long block_no) {
    int iCache;
    int rv;
    #ifdef DEBUG
    fprintf(stderr, "isam_cache_block(..., %lu)\n", block_no);
    #endif

    /* A block beyond the current end of the file. Write it to extend
       the file right away */

    /* STEP 2: this function needs to be instrumented to record the
       number of times it has been called (this is a good place to do
       that) and the number of times it needs to read from disk (see
       read() call below).  */

    cache_call_global++;

    if (block_no >= isam_ident->fHead.CurBlocks) {
        iCache = isam_ident->last_in + 1;

        if (iCache >= CACHE_SIZE) {
            iCache = 0;
        }
        memset(isam_ident->cache[iCache], 0, isam_ident->blockSize);
        isam_ident->last_in = iCache;
        isam_ident->blockInCache[iCache] = block_no;

        if (write_cache_block(isam_ident, iCache)) {
            return -1;
        }
        isam_ident->fHead.CurBlocks = block_no + 1;

        if (writeHead(isam_ident)) {
            return -1;
        }
        return iCache;
    }
    /* A block within the current file bounds. First see if it is in the
       cache already */

    for (iCache = 0; iCache < CACHE_SIZE; iCache++) {
        if ((int) block_no == isam_ident->blockInCache[iCache]) {
            break;
        }
    }
    if (iCache >= CACHE_SIZE) {
        /* The block is not in the cache. Fill the slot after the last
        slot filled */
        iCache = isam_ident->last_in + 1;
        if (iCache >= CACHE_SIZE) {
            iCache = 0;
        }

        if (lseek(isam_ident->fileId, isam_ident->fHead.DataStart +
              block_no * isam_ident->blockSize, SEEK_SET) == (off_t)-1) {
            isam_error = ISAM_SEEK_ERROR;
            return -1;
        }

        rv = read(isam_ident->fileId, isam_ident->cache[iCache], isam_ident->blockSize);

        if (rv != (int) isam_ident->blockSize) {
            isam_error = ISAM_READ_ERROR;
            return -1;
        }

        isam_ident->last_in = iCache;
        isam_ident->blockInCache[iCache] = block_no;

        /* STEP 2: This is a good place to record the number of disk reads.  */
        disk_reads_global++;
    }
    return iCache;
}

/* We look for the first free record in a given block. We have a number
   of options to do this - by block number (in which case we have to
   make sure that the block is in the cache), or by cached block number
   (putting the responsability for the blocknumber on the calling function)
   We'll use the latter approach - it also has the advantage that we only
   need to return a single value. */
static int free_record_in_block(isamPtr isam_ident, int iCache)
{
    unsigned int iFree;
    if ((iCache < 0) || (iCache >= CACHE_SIZE))
    {
#ifdef DEBUG
    fprintf(stderr, "free_record iCache = %d\n", iCache);
#endif
    return -1;
    }
    if (isam_ident->blockInCache[iCache] < 0)
    {
#ifdef DEBUG
    fprintf(stderr, "free_record blockInCache = %d\n",
         isam_ident->blockInCache[iCache]);
#endif
    return -1;
    }
    for (iFree = 0; iFree < isam_ident->fHead.NrecPB; iFree++)
    {
    /* We cannot use a deleted record either. Records are only
       marked deleted rather than unused (free) if
       1. they are the first record of a block and have been deleted.
          (otherwise this plays havoc with the index).
       2. temporarily, while pointers in the preceding and following
       records are being updated after deletion. */
    if (!(head(*isam_ident, iCache, iFree)->statusFlags))
    {
        return iFree;
    }
    }
    return -1;
}

/* Strictly for debugging - print some information about a record */

static void debugRecord(isamPtr f, unsigned long n, char * from) {
#ifdef DEBUG
    int ib = n / f->fHead.NrecPB;
    int ic = isam_cache_block(f, ib);
    int ir = n % f->fHead.NrecPB;

    if (ic < 0) {
        return;
    }

    fprintf(stderr, "%s: key = %s\n", from,
    key(*f, ic, ir));
#endif
}

/* The following function will create an empty isam file, with the specified
   parameters. It will return an isamPtr when succesful, NULL if not.
   The empty file should initially be written sequentially (i.e. with
   increasing key numbers). Now, how long can that continue and what happens
   when you write a non-sequential entry? That is for somewhere else...
   Requirements here:
   no file with the specified name should exist yet. */

isamPtr isam_create(const char * name, unsigned long KeyLen,
    unsigned long DataLen, unsigned long NrecPB, unsigned long Nblocks)
{
    struct stat buf;
    int     i, l;
    long    rv;
    isamPtr fp;
    fileHead fHead;

    memset(&fHead, 0, sizeof(fHead));
    isam_error = ISAM_NO_ERROR;
    if ((8 > KeyLen) || (40 < KeyLen))
    {
    isam_error = ISAM_KEY_LEN;
    return NULL;
    }

    /*
     * First check if name points to an existing file. If it does, stat will
     * return 0, and our routine should return NULL
     */
    if (!stat(name, &buf))
    {
    isam_error = ISAM_FILE_EXISTS;
    return NULL;
    }

    /*
     * Now stat may fail when the pointer is an existing, but invalid
     * symbolic link. We do not want that either.
     */
    if (!lstat(name, &buf))
    {
    isam_error = ISAM_LINK_EXISTS;
    return NULL;
    }

    /*
     * Now create and initialize the file header and descriptor
     */
    fHead.magic  = isamMagic;
    fHead.KeyLen = KeyLen;
    fHead.DataLen = DataLen;
    fHead.NrecPB = NrecPB;
    fHead.Nblocks = Nblocks;
    i = KeyLen + DataLen + sizeof(recordHead);
    l = (i + 7) / 8;
    fHead.RecordLen = 8 * l;
    fp = makeIsamPtr(&fHead);

    /*
     * At last - create a file
     */
    fp->fileId = open(name, O_RDWR | O_CREAT | O_EXCL, 0660);
    if (fp->fileId < 0)
    {
    isam_error = ISAM_OPEN_FAIL;
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    /* Write an initial header */
    if (writeHead(fp))
    {
    close(fp->fileId);
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    fp->mayWrite = 1;
    /* Initialise the file index and write it to disk */
    fp->index = index_makeNew(Nblocks, KeyLen);
    /* The data blocks will start immediately after the index */
    fp->fHead.DataStart = rv = index_writeToDisk(fp->index, fp->fileId);
    if (rv < 0)
    {
    isam_error = ISAM_WRITE_FAIL;
    index_free(fp->index);
    close(fp->fileId);
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    /* Initialise the first data block with the dummy first record.
       Store in cache and write to disk */
    fp->blockInCache[0] = 0;
    fp->cur_id = 0;
    fp->cur_recno = 0;
    cur_head((*fp))->statusFlags = ISAM_SPECIAL;
    l = write(fp->fileId, fp->cache[0], fp->blockSize);
    if (l != (int) fp->blockSize)
    {
    isam_error = ISAM_WRITE_FAIL;
    index_free(fp->index);
    close(fp->fileId);
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    /* The file header can now be further updated */

    fp->fHead.CurBlocks = 1;
    if (writeHead(fp))
    {
    close(fp->fileId);
    index_free(fp->index);
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    fp->maxKey = calloc(1, KeyLen);
    assert(fp->maxKey != NULL);
    return fp;
}

isamPtr
isam_open(const char *name, int update)
{
    struct stat buf;
    isamPtr fp;
    fileHead fh;
    int     fid;
    int     block_no, rec_no;
    int     iCache;

    memset(&fh, 0, sizeof(fh));
    isam_error = ISAM_NO_ERROR;

    /*
     * First check if name points to an existing file. If it does, stat will
     * return 0, and our routine can continue
     */
    if (stat(name, &buf))
    {
    isam_error = ISAM_NO_SUCH_FILE;
    return NULL;
    }

    /*
     * At last - open a file
     */
    fid = open(name, O_RDWR);
    if (fid < 0)
    {
    isam_error = ISAM_OPEN_FAIL;
    return NULL;
    }
    /* read header and test amount of data read */

    if (sizeof(fileHead) != read(fid, &fh, sizeof(fileHead)))
    {
    isam_error = ISAM_READ_ERROR;
        close(fid);
    return NULL;
    }

    /* test if it could be an isam file */
    if (fh.magic != isamMagic)
    {
    isam_error = ISAM_BAD_MAGIC;
    close(fid);
    return NULL;
    }

    /* We can only handle version 0 */
    if (fh.version > 0)
    {
    isam_error = ISAM_BAD_VERSION;
    close(fid);
    return NULL;
    }

    /* Now create and initialise the isamPtr */
    fp = makeIsamPtr(&fh);

    isam_error = ISAM_NO_ERROR;

    if (!(fp->index = index_readFromDisk(fid)))
    {
    close(fid);
    free(fp->cache[0]);
    isam_error = ISAM_INDEX_ERROR;
    free(fp);
    return NULL;
    }
    fp->fileId = fid;
    fp->mayWrite = 1;
    fp->blockInCache[0] = 0;
    fp->cur_id = 0;
    fp->cur_recno = 0;
    if (read(fp->fileId, fp->cache[0], fp->blockSize) != (int) fp->blockSize)
    {
    index_free(fp->index);
    close(fid);
    free(fp->cache[0]);
    isam_error = ISAM_READ_ERROR;
    free(fp);
    return NULL;
    }
    fp->maxKey = calloc(1, fp->fHead.KeyLen);
    assert(fp->maxKey != NULL);
    block_no = fp->fHead.MaxKeyRec / fp->fHead.NrecPB;
    rec_no = fp->fHead.MaxKeyRec % fp->fHead.NrecPB;
    iCache = isam_cache_block(fp, block_no);
    if (iCache < 0)
    {
    index_free(fp->index);
    close(fid);
    free(fp->cache[0]);
    free(fp);
    return NULL;
    }
    memcpy(fp->maxKey, key(*fp, iCache, rec_no), fp->fHead.KeyLen);

    return fp;
}

/* Close an isam file and release the memory used */

int
isam_close(isamPtr f)
{
    int i;
    /* Before closing the file, we should actually make sure it has been
       "sync-ed" to disk. We'll make sure that any modifications are written
       immediately by the function making the modification. So we'll need not
       do any saving here */
    isam_error = ISAM_NO_ERROR;
    if (testPtr(f))
    {
    return -1;
    }
    free(f->cache[0]);
    free(f->maxKey);
    for (i = 0; i < CACHE_SIZE; i++)
    {
    f->cache[i] = NULL;
    }
    index_free(f->index);
    f->fHead.magic = 0;
    close(f->fileId);
    free(f);
    return 0;
}

/* Set the current pointer to the last valid record (if that exists) with
   a key less than the requested key. isam_readNext will then read the
   record with that key (if it exists), or the next higher key (if that
   exists) */

int isam_setKey(isamPtr isam_ident, const char *key)
{
    int block_no;
    int rec_no;
    unsigned long next, prev;
    int iCache;
    int rv;

    if (testPtr(isam_ident))
    {
    return -1;
    }
    if (key[0] == 0)
    {
    /* "rewind" the file to the dummy first record.*/
    iCache = isam_cache_block(isam_ident, 0);
    if (iCache < 0)
    {
        return -1;
    }
        isam_ident->cur_id = iCache;
        isam_ident->cur_recno = 0;
        return 0;
    }
    /* First find block number from index */
    block_no = index_keyToBlock(isam_ident->index, key);
    rec_no = 0;
    /* Now make sure the block is in cache */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
    return -1;
    }
    /* Skip all records with smaller keys */
    while ((rv = strncmp(key, key((*isam_ident),iCache,rec_no),
              isam_ident->fHead.KeyLen)) > 0)
    {
    next = head((*isam_ident),iCache,rec_no)->next;
    debugRecord(isam_ident, next, "setkey #1");
    if (!next)
    {
        /* There is no next record */
        break;
    }
    block_no = next / isam_ident->fHead.NrecPB;
    rec_no = next % isam_ident->fHead.NrecPB;
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
      }
    /* Now we have found / may have found a record with a key larger than
       or equal to the given key - but then we promised to position at
       the previous valid record - so look for that.
       There may be no record with a larger key - in that case
           (rv < 0) && (next == 0)
       A valid record is a record that has the valid flag set.*/

    while (((rv = strncmp(key, key((*isam_ident),iCache,rec_no),
             isam_ident->fHead.KeyLen)) <= 0) ||
       (!(head((*isam_ident),iCache,rec_no)->statusFlags & ISAM_VALID)))
    {
    prev = head((*isam_ident),iCache,rec_no)->previous;
    debugRecord(isam_ident, prev, "setkey #2");
    /* prev may be 0 - that is OK; we are then pointing to the dummy
       first record. However, that is not a valid record, but we
       have no choice.
       This means that a call to isam_prev must return an invalid result
    */
    block_no = prev / isam_ident->fHead.NrecPB;
    rec_no = prev % isam_ident->fHead.NrecPB;
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
    if (!prev)
    {
        /* The "current" record is not "it", and the previous is the
           dummy first record. We have gone there already.
           This is completely normal */
        isam_ident->cur_id = iCache;
        isam_ident->cur_recno = 0;
        return 0;
    }
    }
    isam_ident->cur_id = iCache;
    isam_ident->cur_recno = rec_no;
    return 0;
}

/* isam_readNext will read the next valid record (from cur_recno and cur_id),
   if such a record exists */

int isam_readNext(isamPtr isam_ident, char *key, void *data) {
    int block_no;
    int rec_no;
    int iCache;
    int next;

    if (testPtr(isam_ident)) {
    return -1;
    }
    /* First we have to look for the next valid record (that is the way we
       decided to make all this work! */
    rec_no = isam_ident->cur_recno;
    iCache = isam_ident->cur_id;
    do {
        next = head((*isam_ident),iCache,rec_no)->next;
        debugRecord(isam_ident, next, "next #1");

        if (! next) {
            isam_error = ISAM_EOF;
            return -1;
        }
        block_no = next / isam_ident->fHead.NrecPB;
        rec_no = next % isam_ident->fHead.NrecPB;
        iCache = isam_cache_block(isam_ident, block_no);

        if (iCache < 0) {
            return -1;
        }
    } while(!(head((*isam_ident),iCache,rec_no)->statusFlags & ISAM_VALID));

    isam_ident->cur_id = iCache;
    isam_ident->cur_recno = rec_no;
    memcpy(key, cur_key(*isam_ident), isam_ident->fHead.KeyLen);
    memcpy(data, cur_data(*isam_ident), isam_ident->fHead.DataLen);
    isam_error = ISAM_NO_ERROR;
    return 0;
}

/* isam_readPrev will read the current record, if it is valid, and then
   reposition the file to the preceding valid record (if that exists). */

int isam_readPrev(isamPtr isam_ident, char *key, void *data) {
    int block_no;
    int rec_no = 0;
    int iCache = 0;
    int prev;

    if (testPtr(isam_ident)) {
    return -1;
    }
    /* We should now be at the correct valid record (unless we are at the
       very first record). So we check, copy the data, and try to find
       the preceding valid record. */

    isam_error = ISAM_NO_ERROR;
    if (cur_head(*isam_ident)->statusFlags & ISAM_VALID) {
        memcpy(key, cur_key(*isam_ident), isam_ident->fHead.KeyLen);
        memcpy(data, cur_data(*isam_ident), isam_ident->fHead.DataLen);
        rec_no = isam_ident->cur_recno;
        iCache = isam_ident->cur_id;

        do {
            prev = head((*isam_ident),iCache,rec_no)->previous;
            block_no = prev / isam_ident->fHead.NrecPB;
            rec_no = prev % isam_ident->fHead.NrecPB;
            iCache = isam_cache_block(isam_ident, block_no);

            if (iCache < 0) {
                return -1;
            }
            if (! prev) {
                break;
            }
        }  while(!(head((*isam_ident),iCache,rec_no)->statusFlags&ISAM_VALID));

        isam_ident->cur_id = iCache;
        isam_ident->cur_recno = rec_no;
        return 0;
    }
    isam_error = ISAM_SOF;
    return -1;
}

/* isam_readByKey will attempt to read a record with the requested key */

int isam_readByKey(isamPtr isam_ident, const char *key, void *data) {
    /* STEP 5: This implementation is inefficient, and I have not verified
       that it really works according to its specification
       For one thing, it does not test for a valid isam_ident before use.
       It is probably better to include tempData and tempKey as part of the
       structure pointed to by isam_ident. That also saves some mallocs
       and frees */

    void * tempData = malloc(isam_ident->fHead.DataLen);
    char * tempKey = malloc(isam_ident->fHead.KeyLen);
    int rv = isam_setKey(isam_ident, key);

    assert(tempData != NULL && tempKey != NULL);

    if (rv)
    {
    free(tempKey);
    free(tempData);
    return -1;
    }
    rv = isam_readNext(isam_ident, tempKey, tempData);
    if (rv)
    {
    free(tempKey);
    free(tempData);
    return -1;
    }
    if (strncmp(key, tempKey, isam_ident->fHead.KeyLen))
    {
    isam_error = ISAM_NO_SUCH_KEY;
    free(tempKey);
    free(tempData);
    return -1;
    }
    memcpy(data, tempData, isam_ident->fHead.DataLen);
    free(tempKey);
    free(tempData);
    return 0;
}

/* isam_append implements part of the functionality of isam_writeNew.
   It is only used when the key is larger than or equal to the largest key
   so far (this can be a key in a regular record, in a deleted first record
   of a block, or in an overflow record. The latter can happen if a record was
   inserted before the then last record, after which the record was deleted).
   We must find the last record (store that in the file header) and an
   appropriate place to store the new record. The appropriate place will be
   defined as:
   1. The place of a deleted first record, if the key matches the key of
      the deleted record.
   2. The first free slot in the block returned by an index search for the
      maximum key, as long as that is not the last slot in that block.
   3. The first slot in a new block if that still fits in with the regular
      data blocks. In this case the key must be added to the index and
      the index rewritten.
   4. An overflow record position (last block in a record, or a free slot
   in an overflow block)*/

static int
isam_append(isamPtr isam_ident, const char * key, const void * data)
{
    int block_no;
    int rec_no;
    int new_block_no, new_rec_no;
    unsigned long next;
    int iCache, nCache;
    int rv;

    /* First find block number from index */
    block_no = index_keyToBlock(isam_ident->index, key);
    rec_no = 0;
    /* Now make sure the block is in cache */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
    return -1;
    }
    /* First see if this is a deleted first record.
       There are a lot of properties that we could check.
       1. The first record must have been deleted (but it may still have a
       smaller key)
       2. The first record has the same key as the requested key (and as
       maxKey).
       3. The next pointer of the first record is zero (this appears to be
       the most sensible test anyway). */
    next = head((*isam_ident),iCache,rec_no)->next;
    if (next == 0)
    {
        /* I think this must be a deleted first record. It still may have
       a smaller key than the new record.
       This actually is a bit a doubtful case for append -
       let it be for now */
    if ((rv =strncmp(key, key((*isam_ident),iCache,rec_no),
             isam_ident->fHead.KeyLen)))
    {
        assert(rv > 0);
        /* This now implies an otherwise normal append */
    } else
    {
        /* Keys are indeed equal - but, have we checked anywhere else
           if the key did not yet exist by chance? So let's check that
           here. Next check maxKey for added security */
        if (ISAM_VALID & head((*isam_ident),iCache,rec_no)->statusFlags)
        {
        isam_error = ISAM_RECORD_EXISTS;
        return -1;
        }
        assert(0 == strncmp(key, isam_ident->maxKey,
                isam_ident->fHead.KeyLen));
            /* Assert deleted state */
        assert(ISAM_DELETED ==
           head((*isam_ident),iCache,rec_no)->statusFlags);
        /* We only need to copy the data and mark the record as valid */
        memcpy(data(*isam_ident, iCache, rec_no), data,
           isam_ident->fHead.DataLen);
        head(*isam_ident, iCache, rec_no)->statusFlags = ISAM_VALID;
        isam_ident->fHead.Nrecords++;
        /* Now what do we write first? - writing the header twice is
           extra work, but at least makes it easy to identify an
           inconsistent file. Writing intentions would be even more
           secure*/
        isam_ident->fHead.FileState |= ISAM_STATE_UPDATING;
        writeHead(isam_ident);
        if (write_cache_block(isam_ident, iCache))
        {
        return -1;
        }
        isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
        return writeHead(isam_ident);
    }
    }
    /* So there is a next, follow it to the bitter end */
    while (next)
    {
    block_no = next / isam_ident->fHead.NrecPB;
    rec_no = next % isam_ident->fHead.NrecPB;
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
    next = head((*isam_ident),iCache,rec_no)->next;
    }
    /* Now we should have a record with a smaller key, equal to
       maxKey */
    rv = strncmp(key, key((*isam_ident),iCache,rec_no),
         isam_ident->fHead.KeyLen);
    if (rv <= 0)
    {
    unsigned int i;
    dumpMaxKey(isam_ident);
    fprintf(stderr, "key = '%s', found key ='", key);
    for (i = 0; i <= isam_ident->fHead.KeyLen; i++)
    {
        fprintf(stderr, "%c", key((*isam_ident),iCache,rec_no)[i]);
    }
    fprintf(stderr, "'\n");
    }
    assert(rv > 0);
    new_block_no = block_no;
    nCache = iCache;
    new_rec_no = free_record_in_block(isam_ident, iCache);
    while (new_rec_no < 0 || new_rec_no >= (int) isam_ident->fHead.NrecPB)
    {
    /* We'll leave the last slot free for inserts */
    new_block_no ++;
    if (new_block_no >= (int) isam_ident->fHead.Nblocks)
    {
        /* Insertion in an overflow block obeys slightly different
           rules - e.g. we do not add to the index, and we do not
           leave a free slot */
        break;
    }
    nCache = isam_cache_block(isam_ident, new_block_no);
    if (nCache < 0)
    {
        return -1;
    }
    new_rec_no = free_record_in_block(isam_ident, nCache);
    }
    while (new_rec_no < 0)
    {
    new_block_no ++;
    nCache = isam_cache_block(isam_ident, new_block_no);
    if (nCache < 0)
    {
        return -1;
    }
    new_rec_no = free_record_in_block(isam_ident, nCache);
    }
    memcpy(isam_ident->maxKey, key, isam_ident->fHead.KeyLen);
    isam_ident->cur_id = nCache;
    isam_ident->cur_recno = new_rec_no;
    memcpy(cur_key(*isam_ident), key, isam_ident->fHead.KeyLen);
    memcpy(cur_data(*isam_ident), data, isam_ident->fHead.DataLen);
    cur_head(*isam_ident)->statusFlags = ISAM_VALID;
    cur_head(*isam_ident)->previous = block_no * isam_ident->fHead.NrecPB +
    rec_no;
    cur_head(*isam_ident)->next = 0;
    /* Beware - the previous record may have been deleted from the
       cache, but re-reading it may remove the current record.
       We'll first update the file header */
    isam_ident->fHead.FileState |= ISAM_STATE_UPDATING;
    isam_ident->fHead.Nrecords++;
    isam_ident->fHead.MaxKeyRec = new_rec_no +
    new_block_no * isam_ident->fHead.NrecPB;
    writeHead(isam_ident);
    /* We'll now update the index - if needed and as long as the
       file pointer is correct */
    if (new_rec_no == 0 && new_block_no < (int) isam_ident->fHead.Nblocks) {
        index_addKey(isam_ident->index, key, new_block_no);
        index_writeToDisk(isam_ident->index, isam_ident->fileId);
    }
    if (new_block_no == block_no) {
        /* Update the "next" pointer here and now */
        assert(iCache == nCache);
        head(*isam_ident, iCache, rec_no)->next =
        isam_ident->fHead.MaxKeyRec;
    }
    if (write_cache_block(isam_ident, nCache)) {
        return -1;
    }
    if (new_block_no != block_no) {
        iCache = isam_cache_block(isam_ident, block_no);
        if (iCache < 0) {
            return -1;
        }
        head(*isam_ident, iCache, rec_no)->next =
        isam_ident->fHead.MaxKeyRec;

        if (write_cache_block(isam_ident, iCache)) {
            return -1;
        }
    }
    isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
    return writeHead(isam_ident);
}

int isam_writeNew(isamPtr isam_ident, const char *key, const void *data) {
    int block_no;
    int rec_no;
    int new_block_no, new_rec_no;
    int prev_block_no, prev_rec_no;
    unsigned long next, prev;
    int iCache, nCache, pCache;
    int rv;

    if (testPtr(isam_ident)) {
        return -1;
    }
    if (!key[0]) {
        isam_error = ISAM_NULL_KEY;
        return -1;
    }

    rv = strncmp(key, isam_ident->maxKey, isam_ident->fHead.KeyLen);
    if (rv >= 0) {
        return isam_append(isam_ident, key, data);
    }
    /* Now look for the record with the highest key less than key
       The successor will be the one with the lowest greater than...
       Beware of equal keys, unless for deleted records. We might
       be tempted to use setkey, but setkey selects the last valid
       record - and we are not interested in that now. Copy some code,
       though. */

    block_no = index_keyToBlock(isam_ident->index, key);
    rec_no = 0;
    /* Now make sure the block is in cache */
    iCache = isam_cache_block(isam_ident, block_no);

    if (iCache < 0) {
        return -1;
    }
    next = block_no * isam_ident->fHead.NrecPB;
    while ((rv = strncmp(key, key((*isam_ident),iCache,rec_no),
            isam_ident->fHead.KeyLen)) > 0) {
        next = head((*isam_ident),iCache,rec_no)->next;
        assert(next);
        block_no = next / isam_ident->fHead.NrecPB;
        rec_no = next % isam_ident->fHead.NrecPB;
        iCache = isam_cache_block(isam_ident, block_no);

        if (iCache < 0) {
            return -1;
        }
    }
    /* As we (should have) avoided the last record by going to append
       when needed - and by doing this "assert(next)" stuff, we can
       now proceed on the assumption that the record found really
       has an equal or larger key */
    /* Is the key equal ? */
    if (rv == 0) {
        if (head((*isam_ident),iCache,rec_no)->statusFlags & ISAM_DELETED) {
            /* Should be simple and OK */
                /* We only need to copy the data and mark the record as valid */
                memcpy(data(*isam_ident, iCache, rec_no), data,
                       isam_ident->fHead.DataLen);
                head(*isam_ident, iCache, rec_no)->statusFlags = ISAM_VALID;
                isam_ident->fHead.Nrecords++;
                /* No what do we write first? - writing the header twice is
                   extra work, but at least makes it easy to identify an
                   inconsistent file. Writing intentions would be even more
                   secure*/
                isam_ident->fHead.FileState |= ISAM_STATE_UPDATING;
                writeHead(isam_ident);
                if (write_cache_block(isam_ident, iCache))
                {
                    return -1;
                }
                isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
            isam_ident->cur_id = iCache;
            isam_ident->cur_recno = rec_no;
            return writeHead(isam_ident);
        } else {
            /* Someone is trying to update a record with isam_writeNew,
               we are not going to allow that */
            isam_error = ISAM_RECORD_EXISTS;
            return -1;
        }
    }
    /* So, now we know that the key of the record pointed to by "next" is
       larger, and its predecessor must be smaller (possibly the empty key
       "")
       We first look for the previous record (bearing in mind that we may
       lose the current in doing so), then look for a free slot from
       that point forward. Here we have the choice to only use last slots,
       or any free slot. Let's keep it simple....*/
    prev = head(*isam_ident, iCache, rec_no)->previous;
    prev_block_no = prev / isam_ident->fHead.NrecPB;
    prev_rec_no = prev % isam_ident->fHead.NrecPB;
    pCache = isam_cache_block(isam_ident, prev_block_no);
    if (pCache < 0)
    {
    return -1;
    }
    nCache = pCache;
    new_block_no = prev_block_no;
    new_rec_no = free_record_in_block(isam_ident, nCache);
    if (new_rec_no < 0)
    {
    new_block_no = isam_ident->fHead.Nblocks - 1;
    while (new_rec_no < 0)
    {
        new_block_no ++;
        nCache = isam_cache_block(isam_ident, new_block_no);
        if (nCache < 0)
        {
        return -1;
        }
        new_rec_no = free_record_in_block(isam_ident, nCache);
    }
    }
    /* Set new record as current (isam_readPrev should allow you to re-read it)
       and store all necessary information (key, data, prev and next pointers
       and set to VALID */
    isam_ident->cur_id = nCache;
    isam_ident->cur_recno = new_rec_no;
    memcpy(cur_key(*isam_ident), key, isam_ident->fHead.KeyLen);
    memcpy(cur_data(*isam_ident), data, isam_ident->fHead.DataLen);
    cur_head(*isam_ident)->statusFlags = ISAM_VALID;
    cur_head(*isam_ident)->previous = prev;
    cur_head(*isam_ident)->next = next;
    /* Update file header and prepare for writing. */
    isam_ident->fHead.Nrecords++;
    isam_ident->fHead.FileState |= ISAM_STATE_UPDATING;
    writeHead(isam_ident);
    /* Now we have three records that may or may not lie in the same block.
       We'll not explore all possibilities, but check at least this:
       prev_block_no == new_block_no
       prev_block_no == new_block_no && new_block_no == block_no */
    if (prev_block_no == new_block_no) {
        /* Also update pointer in preceding record when writing block */
        assert(pCache == nCache);
        head(*isam_ident, pCache, prev_rec_no)->next = new_rec_no +
        new_block_no * isam_ident->fHead.NrecPB;
    if (new_block_no == block_no) {
        /* And also that in the next record */
        assert(iCache == nCache);
        head(*isam_ident, iCache, rec_no)->previous = new_rec_no +
        new_block_no * isam_ident->fHead.NrecPB;
        if (write_cache_block(isam_ident, nCache))
        {
        return -1;
        }
        /* Complete update and return */
        isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
        return writeHead(isam_ident);
    }
        if (write_cache_block(isam_ident, nCache))
    {
        return -1;
    }
    /* Make sure next record is in cache, update and write */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
    head(*isam_ident, iCache, rec_no)->previous = new_rec_no +
        new_block_no * isam_ident->fHead.NrecPB;
    if (write_cache_block(isam_ident, iCache))
    {
        return -1;
    }
    /* Finish update and return */
        nCache = isam_cache_block(isam_ident, new_block_no);
    if (nCache < 0)
    {
        return -1;
    }
        isam_ident->cur_id = nCache;
        isam_ident->cur_recno = new_rec_no;
    isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
    return writeHead(isam_ident);
    }
    /* Process all three records separately */
    if (write_cache_block(isam_ident, nCache))
    {
    return -1;
    }
    /* Preceding record */
    pCache = isam_cache_block(isam_ident, prev_block_no);
    if (pCache < 0)
    {
    return -1;
    }
    head(*isam_ident, pCache, prev_rec_no)->next = new_rec_no +
    new_block_no * isam_ident->fHead.NrecPB;
    if (write_cache_block(isam_ident, pCache))
    {
    return -1;
    }
    /* Next record */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
    return -1;
    }
    head(*isam_ident, iCache, rec_no)->previous = new_rec_no +
    new_block_no * isam_ident->fHead.NrecPB;
    if (write_cache_block(isam_ident, iCache))
    {
    return -1;
    }
    /* Finish and return */
    nCache = isam_cache_block(isam_ident, new_block_no);
    if (nCache < 0)
    {
    return -1;
    }
    isam_ident->cur_id = nCache;
    isam_ident->cur_recno = new_rec_no;
    isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
    return writeHead(isam_ident);
}

/* The following routine should give a lot more explanation of the nature
   of the error than it does now. */

int isam_perror(const char *str) {
    /* STEP 1: instead of printing an error number, print textual
       representation for every possible error.
     */

    char *msg;

    switch ((int)isam_error) {
        case ISAM_NO_ERROR:
            msg = "Number error";
            break;
        case ISAM_WRITE_FAIL:
            msg = "Write failure";
            break;
        case ISAM_KEY_LEN:
            msg = "key length";
            break;
        case ISAM_FILE_EXISTS:
            msg = "File doesn't exist";
            break;
        case ISAM_LINK_EXISTS:
            msg = "Link doesn't exist";
            break;
        case ISAM_OPEN_FAIL:
            msg = "open fail";
            break;
        case ISAM_NO_SUCH_FILE:
            msg = "no such file";
            break;
        case ISAM_OPEN_COUNT:
            msg = "open count fail";
            break;
        case ISAM_INDEX_ERROR:
            msg = "index error";
            break;
        case ISAM_READ_ERROR:
            msg = "read error";
            break;
        case ISAM_BAD_MAGIC:
            msg = "bad magic ";
            break;
        case ISAM_BAD_VERSION:
            msg = "bad version ";
            break;
        case ISAM_HEADER_ERROR:
            msg = "header error ";
            break;
        case ISAM_OPEN_FOR_UPDATE:
            msg = "open for update ";
            break;
        case ISAM_IDENT_INVALID:
            msg = "identity invalid ";
            break;
        case ISAM_NO_SUCH_KEY:
            msg = "no such key ";
            break;
        case ISAM_NULL_KEY:
            msg = "key is null ";
            break;
        case ISAM_DATA_MISMATCH:
            msg = "data mismatch ";
            break;
        case ISAM_RECORD_EXISTS:
            msg = "record exists ";
            break;
        case ISAM_SEEK_ERROR:
            msg = "seek error ";
            break;
        case ISAM_SOF:
            msg = "Start of file ";
            break;
        case ISAM_EOF:
            msg = "End of file ";
            break;
    }

    if (str) {
        fprintf(stderr, "isam_perror: %s - error no %d, %s", str, (int)isam_error, msg);
    } else {
        fprintf(stderr, "isam_perror: error no %d, %s", (int)isam_error, msg);
    }
    fprintf(stderr, "\n");

    return 0;
}

/* isam_delete will delete a record if
   - it exists,
   - is valid
   - key and data match */

int isam_delete(isamPtr isam_ident, const char *key, const void *data)
{
    unsigned long block_no;
    int rec_no;
    unsigned long next, prev, prev_valid;
    int iCache;
    int rv;
    int keep_link = 0;
    int prev_block_no, prev_rec_no, pCache;
    int next_block_no, next_rec_no, nCache;
    int prev_valid_block_no, prev_valid_rec_no, pvCache;


    if (testPtr(isam_ident))
    {
    return -1;
    }
    if (key[0] == 0)
    {
    isam_error = ISAM_NULL_KEY;
    return -1;
    }
     /* First find block number from index */
    block_no = index_keyToBlock(isam_ident->index, key);
    rec_no = 0;
    /* Now make sure the block is in cache */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
    return -1;
    }
    /* Skip all records with smaller keys */
    while ((rv = strncmp(key, key((*isam_ident),iCache,rec_no),
              isam_ident->fHead.KeyLen)) > 0)
    {
    next = head((*isam_ident),iCache,rec_no)->next;
    debugRecord(isam_ident, next, "delete #1");
    if (!next)
    {
        /* There is no next record */
        break;
    }
    block_no = next / isam_ident->fHead.NrecPB;
    rec_no = next % isam_ident->fHead.NrecPB;
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
      }
    /* Now either rv != 0 - in which case there is no matching key,
       independent of next being zero or not, or rv == 0, in which
       case at least the key matches */
    if (rv || !(head(*isam_ident, iCache, rec_no)->statusFlags & ISAM_VALID)) {
        isam_error = ISAM_NO_SUCH_KEY;
        return -1;
    }
    /* So the key matches and the record is valid, now see if the data
       match. The data may not be strings - so compare the full length.
       So the user must beware to include the same junk in the remainder
       of a data field, if it happens to be a string (isam_readByKey will
       take care of that) */
    if (memcmp(data, data(*isam_ident, iCache, rec_no), isam_ident->fHead.DataLen)) {
        isam_error = ISAM_DATA_MISMATCH;
        return -1;
    }
    /* So we can now delete the record. Begin by marking it "deleted" */
    head(*isam_ident, iCache, rec_no)->statusFlags = ISAM_DELETED;
    isam_ident->fHead.FileState |= ISAM_STATE_UPDATING;
    isam_ident->fHead.Nrecords--;

    if (writeHead(isam_ident)) {
        return -1;
    }
    if ((rec_no == 0) && (block_no < isam_ident->fHead.Nblocks)) {
    /* This record occurs in the index, so we only mark it as deleted,
       but keep it in the linked list */
        keep_link = 1;
        if (write_cache_block(isam_ident, iCache)) {
            return -1;
        }
    }
    /* We'll need two kinds of preceding record - the immediately preceding
       (which will always exist), and the preceding valid (which may exist).
       The latter is needed to reposition the cur_id and the cur_block_no.
       Let's begin with the preceding*/
    /* The record cannot be the dummy first, so there must be a previous
       record */
    next = head(*isam_ident, iCache, rec_no)->next;
    prev = head(*isam_ident, iCache, rec_no)->previous;
    prev_block_no = prev / isam_ident->fHead.NrecPB;
    prev_rec_no   = prev % isam_ident->fHead.NrecPB;
    pCache = isam_cache_block(isam_ident, prev_block_no);
    if (pCache < 0)
    {
    return -1;
    }
    prev_valid = prev;
    prev_valid_block_no = prev_block_no;
    prev_valid_rec_no  = prev_rec_no;
    pvCache = pCache;
    isam_ident->cur_id = pvCache;
    isam_ident->cur_recno = prev_valid_rec_no;
    if (!keep_link)
    {
    cur_head(*isam_ident)->next = next;
    if (write_cache_block(isam_ident, pCache))
    {
        return -1;
    }
    /* The next record need not exist (we may have deleted the last)*/
    if (next)
    {
        next_block_no = next / isam_ident->fHead.NrecPB;
        next_rec_no   = next % isam_ident->fHead.NrecPB;
        nCache = isam_cache_block(isam_ident, next_block_no);
        if (nCache < 0)
        {
        return -1;
        }
        head(*isam_ident, nCache, next_rec_no)->previous = prev;
        if (write_cache_block(isam_ident, nCache))
        {
        return -1;
        }
    } else
    {
    /* This is likely to be the record with the maxKey; if it is,
        maxKey must be set to that of the preceding record */
        if (!strncmp(key, isam_ident->maxKey, isam_ident->fHead.KeyLen))
        {
            memcpy(isam_ident->maxKey, key(*isam_ident, pCache, prev_rec_no),
             isam_ident->fHead.KeyLen);
        isam_ident->fHead.MaxKeyRec = prev;
        }
    }
    /* Now clear all status flags, but first ensure that the record
       still/again is cached */
    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }
    head(*isam_ident, iCache, rec_no)->statusFlags = 0;
    if (write_cache_block(isam_ident, iCache))
    {
        return -1;
    }
    }
    isam_ident->fHead.FileState &= ~ISAM_STATE_UPDATING;
    if (writeHead(isam_ident))
    {
    return -1;
    }

    /* Now make sure the previous valid record is found and cached */
    pvCache = isam_cache_block(isam_ident, prev_valid_block_no);
    if (pvCache < 0)
    {
    return -1;
    }
    while (prev_valid && (!(cur_head(*isam_ident)->statusFlags & ISAM_VALID)))
    {
    prev_valid = cur_head(*isam_ident)->previous;
    prev_valid_block_no = prev_valid / isam_ident->fHead.NrecPB;
    prev_valid_rec_no   = prev_valid % isam_ident->fHead.NrecPB;
    pvCache = isam_cache_block(isam_ident, prev_valid_block_no);
    if (pvCache < 0)
    {
        return -1;
    }
    isam_ident->cur_id = pvCache;
    isam_ident->cur_recno = prev_valid_rec_no;
    }
    return 0;
}

/* STEP 5:
   Updating a record effectively has the same effect as first deleting it,
   and the rewriting it again (though the resulting file structure may
   be different). Here we choose the quick-and-dirty implementation,
   deleting and writing. The correct and more efficient implementation,
   where far fewer records need to be written, is left as an exercise.
*/

int isam_update(isamPtr isam_ident, const char *key, const void *old_data,
        const void *new_data)
{
    int rv = isam_delete(isam_ident, key, old_data);
    if (rv)
    {
    return -1;
    }
    return isam_writeNew(isam_ident, key, new_data);
}

/* Like strlen, but with a maximum length allowed.  There is "strnlen" in
   GNU libc, but it's non-standard, hence we provide our own version.
 */
static long my_strnlen(const char* str, int maxLen)
{
    const char* s = str;
    while (maxLen-- > 0 && *s)
    {
    s++;
    }
    return s-str;
}

/* Go through the file and collect statistics on the filling of records
   and complete blocks, separately for sequential part and for overflow
   part.  Also collect statistics on the key length used.
 */
int isam_fileStats(isamPtr isam_ident, struct ISAM_FILE_STATS* stats) {
    int iCache;
    unsigned long block_no;
    unsigned long keySum = 0;
    unsigned long keyNo = 0;
    unsigned long blocksRegularUsedSum = 0;
    unsigned long blocksOverflowUsedSum = 0;

    if (testPtr(isam_ident))
    {
    return -1;
    }

    /* Initialise statistics.  */
    stats->blocksRegularNEmpty = 0;
    stats->blocksRegularNPartial = 0;
    stats->blocksRegularNFull = 0;
    stats->blocksRegularUsedMin = 0xffffffff;
    stats->blocksRegularUsedMax = 0;
    stats->blocksRegularUsedAverage = 0;
    stats->recordsRegularNEmpty = 0;
    stats->recordsRegularNUsed = 0;
    stats->blocksOverflowNEmpty = 0;
    stats->blocksOverflowNPartial = 0;
    stats->blocksOverflowNFull = 0;
    stats->blocksOverflowUsedMin = 0xffffffff;
    stats->blocksOverflowUsedMax = 0;
    stats->blocksOverflowUsedAverage = 0;
    stats->recordsOverflowNEmpty = 0;
    stats->recordsOverflowNUsed = 0;
    stats->keyMin = -1;
    stats->keyMax = 0;
    stats->keyAverage = 0;

    /* Iterate through all the blocks.  */
    for (block_no = 0; block_no < isam_ident->fHead.CurBlocks; block_no++)
    {
    unsigned long rec_no;

    unsigned long empty = 0;
    unsigned long used = 0;

    iCache = isam_cache_block(isam_ident, block_no);
    if (iCache < 0)
    {
        return -1;
    }

    /* Iterate through all the records in each block.  */
    for (rec_no = 0; rec_no < isam_ident->fHead.NrecPB; rec_no++)
    {
        recordHead* rec = head(*isam_ident, iCache, rec_no);

        if (rec->statusFlags & ISAM_VALID)
        {
        /* Record is used.  Collect key length statistics.  */
        int keyLen = my_strnlen(key(*isam_ident, iCache, rec_no),
                    isam_ident->fHead.KeyLen);
        used++;

        if (stats->keyMin == -1 || keyLen < stats->keyMin)
        {
            stats->keyMin = keyLen;
        }
        if (keyLen > stats->keyMax)
        {
            stats->keyMax = keyLen;
        }
        keySum += keyLen;
        keyNo++;
        }
        else if (rec->statusFlags & ISAM_SPECIAL)
        {
        /* Special null start record.  */
        used++;
        }
        else
        {
        /* The record is empty.  Either it's ISAM_DELETED (being
           the index record), or it doesn't serve any function.  */
        empty++;
        }
    }

    /* Collect statistics after iterating through all the records of
       a block.  */
    if (block_no < isam_ident->fHead.Nblocks)
    {
        /* Ordinary, sequential block.  */
        stats->recordsRegularNEmpty += empty;
        stats->recordsRegularNUsed += used;

        if (empty == isam_ident->fHead.NrecPB)
        {
        stats->blocksRegularNEmpty++;
        }
        else if (used == isam_ident->fHead.NrecPB)
        {
        stats->blocksRegularNFull++;
        }
        else
        {
        stats->blocksRegularNPartial++;
        }

        if (stats->blocksRegularUsedMin == 0xffffffff ||
        used < stats->blocksRegularUsedMin)
        {
        stats->blocksRegularUsedMin = used;
        }
        if (used > stats->blocksRegularUsedMax)
        {
        stats->blocksRegularUsedMax = used;
        }
        blocksRegularUsedSum += used;
    }
    else
    {
        /* Overflow block.  */
        stats->recordsOverflowNEmpty += empty;
        stats->recordsOverflowNUsed += used;

        if (empty == isam_ident->fHead.NrecPB)
        {
        stats->blocksOverflowNEmpty++;
        }
        else if (used == isam_ident->fHead.NrecPB)
        {
        stats->blocksOverflowNFull++;
        }
        else
        {
        stats->blocksOverflowNPartial++;
        }

        if (stats->blocksOverflowUsedMin == 0xffffffff ||
        used < stats->blocksOverflowUsedMin)
        {
        stats->blocksOverflowUsedMin = used;
        }
        if (used > stats->blocksOverflowUsedMax)
        {
        stats->blocksOverflowUsedMax = used;
        }
        blocksOverflowUsedSum += used;
    }
    }

    /* Collect statistics after iterating through all the records of all
       blocks.  Take into account the possibility that no statistics could
       have been collected.  */
    if (stats->blocksRegularNEmpty + stats->blocksRegularNFull +
    stats->blocksRegularNPartial > 0)
    {
    stats->blocksRegularUsedAverage = blocksRegularUsedSum /
        (stats->blocksRegularNEmpty + stats->blocksRegularNFull +
         stats->blocksRegularNPartial);
    }
    if (stats->blocksOverflowNEmpty + stats->blocksOverflowNFull +
    stats->blocksOverflowNPartial)
    {
    stats->blocksOverflowUsedAverage = blocksOverflowUsedSum /
        (stats->blocksOverflowNEmpty + stats->blocksOverflowNFull +
         stats->blocksOverflowNPartial);
    }
    if (keyNo > 0)
    {
    stats->keyAverage = keySum / keyNo;
    }
    if (stats->blocksRegularUsedMin == 0xffffffff)
    {
    stats->blocksRegularUsedMin = 0;
    }
    if (stats->blocksOverflowUsedMin == 0xffffffff)
    {
    stats->blocksOverflowUsedMin = 0;
    }

    /* Leave the ISAM file in a well defined state.  */
    iCache = isam_cache_block(isam_ident, 0);
    if (iCache < 0)
    {
    return -1;
    }
    isam_ident->cur_id = iCache;
    isam_ident->cur_recno = 0;

    return 0;
}

/* The isam_cacheStats routine updates the counters used to 
 * measure performance.
 */
int isam_cacheStats(struct ISAM_CACHE_STATS* stats) {
    stats->cache_call = cache_call_global;
    stats->disk_reads = disk_reads_global;
    stats->disk_writes = disk_writes_global;
    
    cache_call_global = 0;
    disk_reads_global = 0;
    disk_writes_global = 0;
    
    return 0;
}

