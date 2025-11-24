#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

// void
// list_employees (struct dbheader_t *dbhdr, struct employee_t *employees)
// {
// }
//
// int
// add_employee (struct dbheader_t *dbhdr, struct employee_t *employees,
//               char *addstring)
// {
// }
//
// int
// read_employees (int fd, struct dbheader_t *dbhdr,
//                 struct employee_t **employeesOut)
// {
// }

int
output_file (int fd, struct dbheader_t *dbhdr, struct employee_t *employees)
{
    if (fd < 0)
        {
            printf ("Got a bad FD from the user\n");
            return STATUS_ERROR;
        }

    // Parse from host endinanness to network endinanness
    dbhdr->magic = htonl (dbhdr->magic);
    dbhdr->filesize = htonl (dbhdr->filesize);
    dbhdr->count = htons (dbhdr->count);
    dbhdr->version = htons (dbhdr->version);

    lseek (fd, 0, SEEK_SET);
    write (fd, dbhdr, sizeof (struct dbheader_t));

    return STATUS_SUCCESS;
}

int
validate_db_header (int fd, struct dbheader_t **headerOut)
{
    if (fd < 0)
        {
            printf ("Bad file descriptor from user\n");
            return STATUS_ERROR;
        }

    unsigned long dhs_size = sizeof (struct dbheader_t);
    struct dbheader_t *header = calloc (1, dhs_size);
    if (header == NULL)
        {
            printf ("Malloc failed to create db header\n");
            return STATUS_ERROR;
        }

    // Read first N bytes from file, and parse them into header struct.
    if (read (fd, header, dhs_size) != dhs_size)
        {
            perror ("read");
            free (header);
            return STATUS_ERROR;
        }

    // Data unpacking, it comes from the OS as "network endinanness", we want
    // "host endinanness". Important topic when working with binaries.
    // Endinanness refers to how binaries are represented on different systems.
    header->version = ntohs (header->version); // ntohs translates to network
                                               // to host short (endinanness).
    header->count = ntohs (header->count);
    header->magic = ntohl (header->magic);
    header->filesize = ntohl (header->filesize);

    if (header->version != 1)
        {
            printf ("Improper header version\n");
            free (header);
            return STATUS_ERROR;
        }
    if (header->magic != HEADER_MAGIC)
        {
            printf ("Improper header magic\n");
            free (header);
            return STATUS_ERROR;
        }

    struct stat dbstat = { 0 };
    fstat (fd, &dbstat);
    if (header->filesize != dbstat.st_size)
        {
            printf ("Corrputed database file\n");
            free (header);
            return STATUS_ERROR;
        }

    *headerOut = header;

    return STATUS_SUCCESS;
}

int
create_db_header (struct dbheader_t **headerOut)
{
    struct dbheader_t *header = calloc (1, sizeof (struct dbheader_t));
    if (header == NULL)
        {
            printf ("Calloc failed to create db header\n");
            return STATUS_ERROR;
        }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof (struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}
