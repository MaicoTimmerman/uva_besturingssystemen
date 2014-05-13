#ifndef INDEX_H
#define INDEX_H

/* -------------------------------------------------------------------------
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
         It demonstrates many of the administrative and layering structures
	 that are also used in normal file systems.
----------------------------------------------------------------------------*/

extern int index_error;

#define INDEX_FULL			(100)
#define INDEX_ALLOCATION_FAILURE	(101)
#define INDEX_INDEXING_ERROR		(102)
#define INDEX_KEY_NOT_LARGER		(103)
#define INDEX_WEIRD_ERROR		(104)
#define INDEX_READ_ERROR		(105)
#define INDEX_INVALID_HANDLE		(106)
#define INDEX_BAD_KEYLENGTH		(107)
#define INDEX_WRITE_FAIL		(108)

typedef struct INDEX_IN_CORE *index_handle;

/* index_makeNew will construct a new index (in memory only),
   characterised by Nblocks (the maximum number if entries in
   the index) and KeyLength (the length of the key strings).
   */
index_handle index_makeNew(unsigned long Nblocks, unsigned long KeyLength);

/* The following routine writes the index to disk. It assumes that
   the file is already correctly positioned. It returns the
   file position after the write.
   Required input:
   The index handle
   The file handle */
long index_writeToDisk(index_handle in, int fid);

/* index_readFromDisk will read an index from disk. This is a three step
   process. First the header is read to determine the size of the
   index, then index_makeNew is called to reserve and initialise the
   required memory, and finally, the actual index records are retrieved.
   The required input is the integer identifier of the (correctly positioned)
   file.
   */
index_handle index_readFromDisk(int fid);

/* The following routine will use a complete index to look for a
   given key. The value it should return is the number of the data
   block where the key-search should continue.
   It needs as input:
   The index handle for the index.
   The key sought
 */
long index_keyToBlock(index_handle in, const char * key);

/* The following routine must add a key at the end of an index.
   It needs as input:
   The index handle for the index.
   A poiner to the key string
   The integer index value to be associated with this key
 */
int index_addKey(index_handle in, const char * key, int index);

/* Call this function to free the memory used by the index */
int index_free(index_handle in);

#endif
