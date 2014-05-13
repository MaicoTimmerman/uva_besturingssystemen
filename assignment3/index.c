/* These routines will be used to create the index for the isam
   file system.
   -------------------------------------------------------------------------
   Author: G.D. van Albada
   University of Amsterdam
   Faculty of Science
   Informatics Institute
   Copyright (C) Universiteit van Amsterdam
   dick at science.uva.nl
   Version: 0.1
   Date: December 2001 / January 2002 / November 2004
   Goal: Part of an assignment on file system structure for the operating
         systems course.
   -------------------------------------------------------------------------
   The strucure/implementation of this index is not very efficient.
   We use an N-ary tree with a fixed branching factor of 4; it might
       have been more logical to make this variable, depending on the
       key length and an appropriate record/block size. This also depends on
       whether the index blocks are retained in memory, or read when
       needed.
       The choice for a small number of elements per record also was
       made to obtain a multi-level tree structure for a limited
       number of elements.

   Will we keep the index in memory?
   Well - that greatly simplifies matters - so let's do it */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>

/* Use assert to pinpoint fatal errors - should be removed later */
#include <assert.h>
#include "index.h"

/* C is not very helpful when you have to define structures with elements
   of which the size is not known at compile time. In this case, we will
   make full use of another weakness of C - the lack of array-boundary
   checking. The structure that we define contains the first few
   fixed length entries, followed by the first elements of the array
   that will contain the actual variable part. When allocating memory
   for this structure, we will allocate the actual required size, and
   use higher index values to access that part of the array.
   Also notice that the array (keys) will eventually contain four
   key strings. We have to compute the starting points of the first,
   second and third element when we actually need them.
   Macros can be very helpful to keep the code readable in such cases
   */
typedef struct
{
      unsigned long Nkeys;	/* The number of valid keys in this record */
      unsigned long index[4];	/* Data/index record nrs for the keys */
      char    keys[8];		/* Store 4 keys here; actual size will be larger */
} indexRecord;

/* The index will contain a number of index records (defined above) and
   a structure describing the actual size of the index (element size,
   number of elements, number of levels, degree of filling).
   On disk this index header will, of course, precede the index 
   records. */

typedef struct
{
      unsigned long Nlevels;
      unsigned long KeyLength;
      unsigned long Nkeys;	    /* The number of valid keys in the index */
      unsigned long iRecordLength;  /* The actual size of an index record    */
      unsigned long NperLevel[8];   /* The number of index records per level */
      indexRecord root;		    /* The root index */
} indexheader;

/* This structure will describe the index elements when stored in memory */

typedef struct INDEX_IN_CORE
{
      indexRecord *levels[8];	/* Arrays with index records */
      indexheader to_disk;	/* This goes to disk         */
} in_core;

#define KeyInRec(key,rec,len)    (&((rec).keys[(key) * (len)]))

int index_error = 0;

/* index_makeNew will construct a new index (in memory only),
   characterised by Nblocks (the maximum number of entries in
   the index) and KeyLength (the length of the key strings).
   */
in_core *
index_makeNew(unsigned long Nblocks, unsigned long KeyLength)
{
    unsigned long iRecordLength = sizeof(indexRecord) - sizeof(char[8]) +
                                  4 * KeyLength;
    in_core *in = calloc(1, sizeof(in_core) - sizeof(indexRecord) +
			 iRecordLength);
    int     i;
    int     levs;
    int     n;
    /* We will have NBlocks entries in the deepest level of nodes
       (the leaf nodes). These will be contained in Nrec = (NBlocks + 3) / 4
       index records.
       These, in turn will need (Nrec + 3) / 4 index records, etc. */

#ifdef DEBUG
    printf("iRecordLength = %d\n", iRecordLength);
#endif
    if (!in)
    {
	index_error = INDEX_ALLOCATION_FAILURE;
	return in;
    }
    if (KeyLength == 0)
    {
	index_error = INDEX_BAD_KEYLENGTH;
	free(in);
	return NULL;
    }
#ifdef DEBUG
    printf("-------> 1\n");
#endif
    for (levs = 0, i = Nblocks; i > 1; levs++, i = (i + 3) >> 2)
	;
    if (!levs)
    {
	levs = 1;
    }
#ifdef DEBUG
    printf("-------> 2\n");
#endif

    /* levs == 1 -> root only; otherwise (levs - 1) additional levels */

    in->to_disk.iRecordLength = iRecordLength;
    in->to_disk.KeyLength = KeyLength;
    in->to_disk.Nkeys = 1;	/* The single empty key for record 0! */
    in->to_disk.root.Nkeys = 1;
    in->to_disk.Nlevels = levs - 1;
    n = (Nblocks + 3) >> 2;
    for (i = levs - 2; i >= 0; i--)
    {
	in->levels[i] = calloc(n, iRecordLength);
	assert(in->levels[i] != NULL);
	in->levels[i]->Nkeys = 1;
	in->to_disk.NperLevel[i] = n;
#ifdef DEBUG
	printf("%d index records at level %d\n", n, i);
#endif
	n = (n + 3) >> 2;
    }
#ifdef DEBUG
    printf("-------> 3\n");
#endif
    return in;
}

/* The following routine writes the index to disk. It assumes that
   the file is already correctly positioned. It returns the
   file position after the write */

long 
index_writeToDisk(in_core * in, int fid)
{
    unsigned int     i;
    int     rv;
    if ((!in) || (!(in->to_disk.Nkeys)) || (!(in->to_disk.KeyLength)))
    {
	index_error = INDEX_INVALID_HANDLE;
	return -1;
    }
    rv = write(fid, &(in->to_disk), sizeof(indexheader) - sizeof(indexRecord)
	       + in->to_disk.iRecordLength);
    if (rv != (int) (sizeof(indexheader) - sizeof(indexRecord) +
	      in->to_disk.iRecordLength))
    {
	index_error = INDEX_WRITE_FAIL;
	return -1;
    }
#ifdef DEBUG
    printf("Wrote %d bytes\n", rv);
#endif
    for (i = 0; i < in->to_disk.Nlevels; i++)
    {
	rv = write(fid, in->levels[i], in->to_disk.NperLevel[i] *
		   in->to_disk.iRecordLength);
	if (rv != (int) (in->to_disk.NperLevel[i] * in->to_disk.iRecordLength))
	{
	    index_error = INDEX_WRITE_FAIL;
	    return -1;
	}
#ifdef DEBUG
	printf("Wrote %d bytes\n", rv);
#endif
    }
    return lseek(fid, 0, SEEK_CUR);
}

/* index_readFromDisk will read an index from disk. This is a three step
   process. First the header is read to determine the size of the
   index, then index_makeNew is called to reserve and initialise the
   required memory, and finally, the actual index records are retrieved.
   */
in_core *
index_readFromDisk(int fid)
{
    unsigned int     i;
    int     rv;
    int     NBlocks;
    indexheader head;
    in_core *in;

    rv = read(fid, &(head), offsetof(indexheader, root));
    if (rv != offsetof(indexheader, root))
    {
	index_error = INDEX_READ_ERROR;
	return NULL;
    }
    NBlocks = 4 * head.NperLevel[head.Nlevels - 1];
    in = index_makeNew(NBlocks, head.KeyLength);
    if (!in)
    {
	return NULL;
    }
    assert(head.iRecordLength == in->to_disk.iRecordLength);
    assert(head.Nlevels == in->to_disk.Nlevels);
    assert(head.NperLevel[head.Nlevels - 1] ==
	   in->to_disk.NperLevel[head.Nlevels - 1]);
    in->to_disk = head;
    rv = read(fid, &(in->to_disk.root), head.iRecordLength);
    if (rv != (int) head.iRecordLength)
    {
	index_error = INDEX_READ_ERROR;
	return NULL;
    }
#ifdef DEBUG
    printf("Read %d bytes\n", rv);
    printf("Nlevels = %d\n", in->to_disk.Nlevels);
#endif
    for (i = 0; i < in->to_disk.Nlevels; i++)
    {
#ifdef DEBUG
	printf "NperLevel[%d] = %d\n", i, in->to_disk.NperLevel[i]);
#endif
	if (read(fid, in->levels[i], in->to_disk.NperLevel[i] *
		 in->to_disk.iRecordLength) != (int) (in->to_disk.NperLevel[i] *
	    in->to_disk.iRecordLength))
	{
	    index_error = INDEX_READ_ERROR;
	    return NULL;
	}
    }
    return in;
}

/* The following function will seek for a key in an index record.
   It will return the index entry corresponding to the largest valid
   key in the record that is not larger than the key sought.
   If no such key exists, it will return -1. (Zero is a valid index value)
   It needs as input:
   The sought key
   The index record
   The length of the keys.
 */
static long 
key_to_index(indexRecord * rec, const char *key, unsigned long KeyLength)
{
    int     rv;
    long    index = -1;
    unsigned int     i;
    for (i = 0; i < rec->Nkeys; i++)
    {
	rv = strncmp(key, KeyInRec(i, *rec, KeyLength), KeyLength);
	if (rv < 0)
	{
	    break;
	}
	index = rec->index[i];
	if (rv == 0)
	{
	    break;
	}
    }
#ifdef DEBUG
    printf("index = %d ", index);
#endif
    return index;
}

/* The following routine will use a complete index to look for a
   given key. The value it should return is the number of the data
   block where the key-search should continue.
   It needs as input:
   The key sought
   A pointer to the in-core structure
   The key length
 */
long 
index_keyToBlock(in_core * in, const char *key)
{
    long    index = 0;
    unsigned int     i;
    int     KeyLength = in->to_disk.KeyLength;
    indexRecord *rec;

    if ((!in) || (!(in->to_disk.Nkeys)) || (!(in->to_disk.KeyLength)))
    {
	index_error = INDEX_INVALID_HANDLE;
	return -1;
    }
    rec = &(in->to_disk.root);
    index = key_to_index(rec, key, KeyLength);
    if (index < 0)
    {
	index_error = INDEX_INDEXING_ERROR;
	return index;
    }

    for (i = 0; i < in->to_disk.Nlevels; i++)
    {
	rec = (indexRecord *) (index * in->to_disk.iRecordLength +
			       (char *) in->levels[i]);
	index = key_to_index(rec, key, KeyLength);
	if (index < 0)
	{
	    index_error = INDEX_INDEXING_ERROR;
	    return index;
	}
    }
    return index;
}

/* The following routine must add a key at the end of an index. Now
   this is a complex operation.
   1) We must ensure that the key is indeed larger than the last key.
   2) We must check that we are not exceeding the maximum capacity of
   the index.
   3) At every level, we may have to insert extra keys and activate
   extra records.
 */
int 
index_addKey(in_core * in, const char *key, int index)
{
    unsigned int     maxKeys = 4 * in->to_disk.NperLevel[in->to_disk.Nlevels - 1];
    int     lev = in->to_disk.Nlevels - 1;
    int     KeyLength = in->to_disk.KeyLength;
    int     nrec;
    int     nkey;
    int     keyno = in->to_disk.Nkeys;
    int     do_prev;
    indexRecord *rec;
    int     rv;

    if ((!in) || (!(in->to_disk.Nkeys)) || (!(in->to_disk.KeyLength)))
    {
	index_error = INDEX_INVALID_HANDLE;
	return -1;
    }
    /* See if we can accommodate more keys */
    if (in->to_disk.Nkeys >= maxKeys)
    {
	index_error = INDEX_FULL;
	return -1;
    }
    /* Find highest key and compare */

    nrec = (keyno - 1) / 4;
    nkey = (keyno - 1) & 0x0003;
    rec = (indexRecord *) (nrec * in->to_disk.iRecordLength +
			   (char *) in->levels[lev]);
    rv = strncmp(key, KeyInRec(nkey, *rec, KeyLength), KeyLength);
    if (rv <= 0)
    {
	index_error = INDEX_KEY_NOT_LARGER;
	return -1;
    }
    /* Find insertion point at leaf level and insert */

    nrec = keyno / 4;
    nkey = keyno & 0x0003;
    rec = (indexRecord *) (nrec * in->to_disk.iRecordLength +
			   (char *) in->levels[lev]);
#ifdef DEBUG
    printf("nrec = %d, nkey = %d, lev = %d, NperLevel = %d\n",
	    nrec, nkey, lev, in->to_disk.NperLevel[lev]);
#endif
    strncpy(KeyInRec(nkey, *rec, KeyLength), key, KeyLength);
    do_prev = !nkey;
    rec->index[nkey] = index;
    rec->Nkeys++;
    in->to_disk.Nkeys++;

    /* If we have just initiated a new record (do_prev is true),
       we have to add the key also to the previous level in the
       index. nrec actually points to the number of the key at that
       level */
    for (; do_prev && (lev--);)
    {
	keyno = nrec;
	nrec = keyno / 4;
	nkey = keyno & 0x0003;
#ifdef DEBUG
	printf("nrec = %d, nkey = %d, lev = %d, NperLevel = %d\n",
		nrec, nkey, lev, in->to_disk.NperLevel[lev]);
#endif
	rec = (indexRecord *) (nrec * in->to_disk.iRecordLength +
			       (char *) in->levels[lev]);
	strncpy(KeyInRec(nkey, *rec, KeyLength), key, KeyLength);
	do_prev = !nkey;
	rec->index[nkey] = keyno;
	rec->Nkeys++;
    }
    if (do_prev)
    {
	/* Add key to root as well */
	if (nrec >= 4)
	{
	    index_error = INDEX_WEIRD_ERROR;
	    return -1;
	}
	nkey = nrec;
	rec = &(in->to_disk.root);
	strncpy(KeyInRec(nkey, *rec, KeyLength), key, KeyLength);
	rec->index[nkey] = nkey;
	rec->Nkeys++;
    }
    return in->to_disk.Nkeys;
}

/* Call this function to free the memory used by the index */
int 
index_free(in_core * in)
{
    unsigned int     i;

    /* Heap memory can remain accessible, even after it is freed,
       as long as you retain the pointer. However, its contents
       have become invalid, so we set a few values to mark it
       as invalid (this is not fool-proof, as new values may be
       set after a new allocation...
       */
    if ((!in) || (!(in->to_disk.Nkeys)) || (!(in->to_disk.KeyLength)))
    {
	index_error = INDEX_INVALID_HANDLE;
	return -1;
    }
    for (i = 0; i < in->to_disk.Nlevels; i++)
    {
	free(in->levels[i]);
	in->levels[i] = NULL;
    }
    in->to_disk.Nkeys = 0;
    in->to_disk.KeyLength = 0;
    free(in);
    return 0;
}
