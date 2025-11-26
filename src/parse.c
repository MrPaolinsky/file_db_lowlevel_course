#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void
list_employees (struct dbheader_t *dbhdr, struct employee_t *employees)
{
    int i = 0;
    for (; i < dbhdr->count; i++)
        {
            printf ("Employee %d\n", i);
            printf ("\tName: %s\n", employees[i].name);
            printf ("\tAddress: %s\n", employees[i].address);
            printf ("\tHours: %d\n", employees[i].hours);
        }
}

int
add_employee (struct dbheader_t *dbhdr, struct employee_t **employees,
              char *addstring)
{
    if (dbhdr == NULL)
        return STATUS_ERROR;
    if (employees == NULL)
        return STATUS_ERROR;
    if (*employees == NULL)
        return STATUS_ERROR;
    if (addstring == NULL)
        return STATUS_ERROR;

    char *name = strtok (addstring, ",");
    if (name == NULL)
        return STATUS_ERROR;
    char *addr = strtok (NULL, ",");
    if (addr == NULL)
        return STATUS_ERROR;
    char *hours = strtok (NULL, ",");
    if (hours == NULL)
        return STATUS_ERROR;

    struct employee_t *e = *employees; // Copy of employees
    e = realloc (e, sizeof (struct employee_t) * dbhdr->count + 1);
    if (e == NULL)
        {
            return STATUS_ERROR;
        }

    dbhdr->count++;

    // strncpy can prevent buffer overflows, since it stops cpoying
    // a string once it reachs the specified size. It might not add the
    // required nul terminator, so we add a -1 at the end to add it ourselves
    // if needed
    strncpy (e[dbhdr->count - 1].name, name,
             sizeof (e[dbhdr->count - 1].name) - 1);
    strncpy (e[dbhdr->count - 1].address, addr,
             sizeof (e[dbhdr->count - 1].address) - 1);
    e[dbhdr->count - 1].hours = atoi (hours);
    *employees = e;

    return STATUS_SUCCESS;
}

int
read_employees (int fd, struct dbheader_t *dbhdr,
                struct employee_t **employeesOut)
{
    if (fd < 0)
        {
            printf ("Got a bad FD from the user\n");
            return STATUS_ERROR;
        }

    int count = dbhdr->count;
    struct employee_t *employees = calloc (count, sizeof (struct employee_t));
    if (employees == NULL)
        {
            printf ("Malloc failed \n");
            return STATUS_ERROR;
        }

    // File offset is already after the header
    read (fd, employees, count * sizeof (struct employee_t));
    int i = 0;
    for (; i < count; i++)
        {
            employees[i].hours = ntohl (employees[i].hours);
        }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int
output_file (int fd, struct dbheader_t *dbhdr, struct employee_t *employees)
{
    if (fd < 0)
        {
            printf ("Got a bad FD from the user\n");
            return STATUS_ERROR;
        }

    int realcount = dbhdr->count;

    // Parse from host endinanness to network endinanness
    dbhdr->magic = htonl (dbhdr->magic);
    dbhdr->filesize = htonl (sizeof (struct dbheader_t)
                             + sizeof (struct employee_t) * realcount);
    dbhdr->count = htons (dbhdr->count);
    dbhdr->version = htons (dbhdr->version);

    lseek (fd, 0, SEEK_SET);
    write (fd, dbhdr, sizeof (struct dbheader_t));

    int i = 0;
    for (; i < realcount; i++)
        {
            employees[i].hours = htonl (employees[i].hours);
            write (fd, &employees[i], sizeof (struct employee_t));
        }

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
