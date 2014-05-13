#ifndef ISAM_H
#define ISAM_H

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
--------------------------------------------------------------------------*/

typedef struct ISAM *isamPtr;
struct ISAM_FILE_STATS;
struct ISAM_CACHE_STATS;

/* isam_create will create an isam_file, but only if a file of that name
   does not yet exist.
   The parameters are:
   name:     name of the file, possibly including directory information
   key_len:  maximum length of the (character string) key.
   data_len: length of the data field
   NrecPB:   number of records that should be put into one block (including
         one overflow record per block).
   Nblocks:  The number of regular data blocks = number of entries in the
         index
   isam_create will return an isamPtr on success, NULL on failure
*/

isamPtr isam_create(const char *name, unsigned long key_len,
    unsigned long data_len, unsigned long NrecPB, unsigned long Nblocks);

/* isam_open will open an existing isam file.
   The parameters are:
   name:     name of the file, possibly including directory information
   update:   (not used) when != 0 the file is opened for reading and writing
         (it actually always is).
   isam_open will return an isamPtr on success, NULL on failure
*/

isamPtr isam_open(const char *name, int update);

/* isam_close will close a previously opened/created isam_file
   The parameters are:
   isam_ident: the isamPtr for the file.
   isam_close will return 0 on success, -1 on failure.
*/

int isam_close(isamPtr isam_ident);

/* isam_setKey will position an isam file on the last valid record with
   a key smaller than the requested key, such that the next call to
   isam_readNext will return the record with the given key, if it exists.
   If no such record exists, the first record with a larger key will
   be returned, if that exists.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a string containing the requested key.
   isam_setKey will return 0 on success, -1 on failure.
*/

int isam_setKey(isamPtr isam_ident, const char *key);

/* isam_readNext will read the next valid record, if that exists.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a character array where the key will be stored.
   data:       pointer to the location where the data are to be stored.
   isam_readNext will return 0 on success, -1 on failure.
*/
int isam_readNext(isamPtr isam_ident, char *key, void *data);

/* isam_readPrev will read the current record, if that exists and is valid and
   afterwards position the file at the preceding valid record, if that exists.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a character array where the key will be stored.
   data:       pointer to the location where the data are to be stored.
   isam_readPrev will return 0 on success, -1 on failure.
*/

int isam_readPrev(isamPtr isam_ident, char *key, void *data);

/* isam_readByKey will try to read a record with the requested key. It behaves
   like an isam_setKey followed by an isam_readNext plus a check that the
   requested key and the retrieved key are the same.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a string containing the requested key.
   data:       pointer to the location where the data are to be stored.
   isam_readByKey will return 0 on success, -1 on failure.
*/

int isam_readByKey(isamPtr isam_ident, const char *key, void *data);

/* isam_update will replace the data field for a record with the given key,
   if such a record exists. As a security measure, it will verify that the
   user has the correct original data.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a string containing the requested key.
   old_data:   a pointer to a location containing a copy of the data that
           the record should contain before modification.
   new_data:   the new data to be stored in the record.
   isam_update will return 0 on success, -1 on failure.
*/

int isam_update(isamPtr isam_ident, const char *key, const void *old_data,
    const void *new_data);

/* isam_writeNew will write a new record to the file, but only if the key is
   not yet in use.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a string containing the new key.
   data:       the data to be stored in the new record.
   isam_writeNew will return 0 on success, -1 on failure.
*/

int isam_writeNew(isamPtr isam_ident, const char *key, const void *data);

/* isam_delete will delete the record with the given key. As a security
   measure, it will verify that the user has the correct original data.
   The parameters are:
   isam_ident: the isamPtr for the file.
   key:        a string containing the key of the record to be deleted.
   data:       a pointer to a location containing a copy of the data that
           the record should contain before deletion.
   isam_delete will return 0 on success, -1 on failure.
*/

int isam_delete(isamPtr isam_ident, const char *key, const void *data);

/* isam_fileStats will collect statistics about a given ISAM file.
   The parameters are:
   isam_ident: the isamPtr for the file.
   stats:      the structure to fill in with statistics.
   isam_fileStats will return 0 on success, -1 on failure.
 */

int isam_fileStats(isamPtr isam_ident, struct ISAM_FILE_STATS* stats);


/* All above routines will set the global variable isam_error when an
   error occurs. Like the standard routine perror, isam_perror should
   print a suitable error message to stderr, optionally preceded by the
   message mess provided by the user */

int isam_cacheStats(struct ISAM_CACHE_STATS* stats);

int isam_perror(const char * mess);

/* Not all of the following errors are actually used .... */
enum isam_error {
    ISAM_NO_ERROR = (0),
    ISAM_WRITE_FAIL,
    ISAM_KEY_LEN,
    ISAM_FILE_EXISTS,
    ISAM_LINK_EXISTS,
    ISAM_OPEN_FAIL,
    ISAM_NO_SUCH_FILE,
    ISAM_OPEN_COUNT,
    ISAM_INDEX_ERROR,
    ISAM_READ_ERROR,
    ISAM_BAD_MAGIC,
    ISAM_BAD_VERSION,
    ISAM_HEADER_ERROR,
    ISAM_OPEN_FOR_UPDATE,
    ISAM_IDENT_INVALID,
    ISAM_NO_SUCH_KEY,
    ISAM_NULL_KEY,
    ISAM_DATA_MISMATCH,
    ISAM_RECORD_EXISTS,
    ISAM_SEEK_ERROR,
    ISAM_SOF,
    ISAM_EOF
};

extern enum isam_error isam_error;

/* Statistics obtained using isam_fileStats.  */
struct ISAM_FILE_STATS {
    /* Statistics for regular (sequential) blocks.  */
    unsigned long blocksRegularNEmpty;       /* # of totally empty blocks    */
    unsigned long blocksRegularNPartial;     /* # of partially occupied blocks.  */
    unsigned long blocksRegularNFull;        /* # of fully occupied blocks.  */
    unsigned long blocksRegularUsedMin;      /* Smallest observed number of
                        used records in a block.     */
    unsigned long blocksRegularUsedMax;      /* Largest observed number of
                        used records in a block.     */
    unsigned long blocksRegularUsedAverage;  /* Average number of used records
                        in a block (floor).          */

    unsigned long recordsRegularNEmpty;      /* # of empty records.          */
    unsigned long recordsRegularNUsed;       /* # of used records.           */

    /* Same as above, but for overflow blocks.  */
    unsigned long blocksOverflowNEmpty;
    unsigned long blocksOverflowNPartial;
    unsigned long blocksOverflowNFull;
    unsigned long blocksOverflowUsedMin;
    unsigned long blocksOverflowUsedMax;
    unsigned long blocksOverflowUsedAverage;

    unsigned long recordsOverflowNEmpty;
    unsigned long recordsOverflowNUsed;

    /* Statistics for key length.  */
    int keyMin;                              /* Smallest observed key length */
    int keyMax;                              /* Largest observed key length  */
    int keyAverage;                          /* Average key length (floor)   */
};


struct ISAM_CACHE_STATS {
    int cache_call;
    int disk_reads;
    int disk_writes;
};

#endif /*ISAM_H */
