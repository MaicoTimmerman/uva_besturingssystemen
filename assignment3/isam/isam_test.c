/* This file contains the source for a very basic program to test the
   isam library.
   The program should be linked with the isam.o and index.o files.
   To run it, give it one parameter: the isam file. If no such file
   exists, it will be created.
   The program will ask for a key; if no record with that key exists,
   you will be allowed to create it, otherwise you are offered several
   possibilities.
   -------------------------------------------------------------------------
   Author: G.D. van Albada
   University of Amsterdam
   Faculty of Science
   Informatics Institute
   dick at science.uva.nl
   Version: 0.0
   Date: December 2001 / January 2002
   -------------------------------------------------------------------------
   The program should not be considered to be a good example of production
   code - it lacks comments, naming is ad-hoc, etc.
   */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "isam.h"

void
instruct(void)
{
    char    str[256];
    printf("Do you want instuctions? (y/n) [y]\n");
    if (!fgets(str, 256, stdin))
    {
	printf("Stopping program\n");
	exit(0);
    }
    if (str[0] != 'y' && str[0] != 'Y' && str[0] != 'j' && str[0] != 'J')
	return;
    printf("This program allows you to test your isam routines.\n");
    printf("Run it as\n");
    printf("isam_test file_name\n");
    printf("If a file file_name exists, isam_test will open it.\n");
    printf("    (if the format and magic number is correct)\n");
    printf("If it does not exist, isam_test will try to create a \n");
    printf("    file of that name.\n");
    printf("Next, isam_test will enter a loop in which it\n");
    printf("1. asks for a key value.\n");
    printf("2. attempts to read a record with that key from the file.\n");
    printf("3. if it succeeds, it will ask you if you want to delete or\n");
    printf("   modify the record.\n");
    printf("4. if it fails, isam_test will allow you to create a new record.\n");
    printf("Isam_test leaves the loop only at the end of the input file.\n");
    printf("Type a [ctrl]D to generate such an EOF from your keyboard.\n");
    printf("The program can be made to execute illegal actions, like\n");
    printf("creating a record with an existing key, or deleting a\n");
    printf("non-existent record by specifying a second command-line\n");
    printf("parameter. If this parameter is numerical with value N,\n");
    printf("an illegal command will be generated every 1 in N\n");
    printf("commands; otherwise N will be set to 4\n");
    printf("\n");
}

int
main(int argc, char *argv[])
{
    int     rv, i;
    char    key[17], data[68], dat[68], str[256];
    char    keyx[17];
    int     N_illegal = 4, count;
    isamPtr fd;

    instruct();

    if (argc < 2)
    {
	printf("isam_test needs one (file) argument\n");
	exit(23);
    }
    printf("Calling isam_open for %s\n", argv[1]);
    fd = isam_open(argv[1], 1);
    printf("Isam_open returned %p\n", fd);
    if (fd == NULL)
    {
	isam_perror("I.e.");
	if (isam_error == ISAM_NO_SUCH_FILE || isam_error == ISAM_OPEN_FAIL)
	{
	    printf("Now attempting to use isam_create\n");
	    fd = isam_create(argv[1], 17, 68, 12, 160);
	    printf("Isam_create returned %p\n", fd);
	    if (fd == NULL)
	    {
		isam_perror("I.e.");
		printf("Stopping program\n");
		exit(-3);
	    }
	} else
	{
	    printf("Stopping program\n");
	    exit(-3);
	}
    }
    if (argc == 3)
    {
	sscanf(argv[2], "%d", &N_illegal);
	N_illegal = (N_illegal < 2) ? 2 : N_illegal;
	printf("Attempting an illegal action every 1 out of %d times\n",
	       N_illegal);
    } else
    {
	N_illegal = 1 << 30;
    }
    printf("Give key >");
    count = 1;
    while (fgets(str, 256, stdin))
    {
	i = strlen(str);
	while (i && str[i] < ' ')
	{
	    str[i] = '\0';
	    i--;
	}
	str[16] = '\0';
	strcpy(key, str);
	rv = isam_readByKey(fd, key, data);
	if (rv)
	{
	    isam_perror("Not found: ");
	    printf("No record with key '%s' found\n", key);
	} else
	{
	    printf("Key: '%s'\nData: '%s'\n", key, data);
	}
	if (count++ >= N_illegal)
	{
	    count = 1;
	    rv = (rv) ? 0 : -1;
	    printf("Attempting illegal action - beware!\n");
	}
	if (rv)
	{
	    printf("Create a record with key '%s' ?\n", key);
	    if (!fgets(str, 256, stdin))
	    {
		printf("Stopping program\n");
		if (isam_close(fd))
		{
		    isam_perror("Isam_closed failed miserably");
		}
		exit(0);
	    }
	    if (str[0] == 'y' || str[0] == 'Y')
	    {
		for (i = 0; i < 68; i++)
		    data[i] = '#';
		printf("Give data >");
		if (!fgets(str, 256, stdin))
		{
		    printf("Stopping program\n");
		    if (isam_close(fd))
		    {
			isam_perror("Isam_closed failed miserably");
		    }
		    exit(0);
		}
		i = strlen(str);
		while (i && str[i] < ' ')
		{
		    str[i] = '\0';
		    i--;
		}
		str[67] = '\0';
		for (i = 0; i < 68; i++)
		    data[i] = '#';
		strcpy(data, str);
		if (isam_writeNew(fd, key, data))
		    isam_perror("Error in isam_writeNew");
	    } else
	    {
	        strcpy(keyx, key);
		isam_setKey(fd, key);
		rv = isam_readNext(fd, key, data);
		printf("Next record :");
		if (rv)
		{
		    isam_perror("Not found: ");
		    printf("No record with key > '%s' found\n", key);
		} else
		{
		    printf("Key: '%s'\nData: '%s'\n", key, data);
		}
		isam_setKey(fd, keyx);
		rv = isam_readPrev(fd, key, data);
		printf("Preceding record :");
		if (rv)
		{
		    isam_perror("Not found: ");
		    printf("No record with key < '%s' found\n", keyx);
		} else
		{
		    printf("Key: '%s'\nData: '%s'\n", key, data);
		}
	    }
	} else
	{
	    nogeens:
    printf("[N]ext, [P]revious, [D]elete or [U]pdate record with this key?\n");
	    if (!fgets(str, 256, stdin))
	    {
		printf("Stopping program\n");
		if (isam_close(fd))
		{
		    isam_perror("Isam_closed failed miserably");
		}
		exit(0);
	    }
	    if (count++ >= N_illegal)
	    {
		count = 1;
		data[0]++;
		data[17] = '\0';
		printf("Attempting illegal action - beware!\n");
	    }
	    switch (str[0])
	    {
	    case 'd':
	    case 'D':
		if (isam_delete(fd, key, data))
		    isam_perror("Error in isam_delete");
		break;
	    case 'u':
	    case 'U':
		printf("Give data >");
		if (!fgets(str, 256, stdin))
		{
		    printf("Stopping program\n");
		    if (isam_close(fd))
		    {
			isam_perror("Isam_closed failed miserably");
		    }
		    exit(0);
		}
		i = strlen(str);
		while (i && str[i] < ' ')
		{
		    str[i] = '\0';
		    i--;
		}
		str[67] = '\0';
		for (i = 0; i < 68; i++)
		    dat[i] = '#';
		strcpy(dat, str);
		if (isam_update(fd, key, data, dat))
		    isam_perror("Error in isam_update");
		break;
	    case 'n':
	    case 'N':
		rv = isam_readNext(fd, key, data);
		printf("Next record :");
		if (rv)
		{
		    isam_perror("Not found: ");
		    printf("No record with key > '%s' found\n", key);
		    break;
		} else
		{
		    printf("Key: '%s'\nData: '%s'\n", key, data);
		}
		goto nogeens;
	    case 'p':
	    case 'P':
		rv = isam_readPrev(fd, key, data);
		printf("Preceding record :");
		if (rv)
		{
		    isam_perror("Not found: ");
		    printf("No record with key < '%s' found\n", key);
		    break;
		} else
		{
		    printf("Key: '%s'\nData: '%s'\n", key, data);
		}
		goto nogeens;
	    }
	}
	printf("Give key >");
    }
    printf("Normal end of programme\n");
    isam_close(fd);
    return 0;
}
