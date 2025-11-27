#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void
print_usage (char *argv[])
{
    printf ("Usage: %s -n -f <db filepath>\n", argv[0]);
    printf ("\t -n - create new database file\n");
    printf ("\t -f - (required) path to database file\n");
    printf ("\t -l - list the employees\n");
    printf ("\t -a - add employee (name,addres,salary)\n");
    return;
}

int
main (int argc, char *argv[])
{
    bool newfile = false;
    char *filepath = NULL;
    char *addstring = NULL;
    bool list = false;
    // Getopt: library to check command line arguments
    int c;

    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;
    struct employee_t *employees = NULL;

    // n is a boolean, f is a string, thats why it has a : at the end.
    while ((c = getopt (argc, argv, "nf:a:l")) != -1)
        {
            switch (c)
                {
                case 'n':
                    newfile = true;
                    break;
                case 'f':
                    filepath = optarg;
                    break;
                case 'a':
                    addstring = optarg;
                    break;
                case 'l':
                    list = true;
                    break;
                case '?':
                    printf ("Unknow option -%c\n", c);
                    break;
                default:
                    return -1;
                }
        }

    // printf ("%d newfile\n", newfile);
    // printf ("%s filepath\n", filepath);
    // printf ("%s addstring\n", addstring);
    // printf ("%d list\n", list);

    if (filepath == NULL)
        {
            printf ("Filepath is a required argument\n");
            print_usage (argv);
            return 0;
        }

    if (newfile)
        {
            dbfd = create_db_file (filepath);
            if (dbfd == STATUS_ERROR)
                {
                    printf ("Unable to create database file\n");
                    return -1;
                }

            if (create_db_header (&dbhdr) == STATUS_ERROR)
                {
                    printf ("Failed to create database header\n");
                    return -1;
                }
        }
    else
        {
            dbfd = open_db_file (filepath);
            if (dbfd == STATUS_ERROR)
                {
                    printf ("Unable to open database file\n");
                    return -1;
                }

            if (validate_db_header (dbfd, &dbhdr) == STATUS_ERROR)
                {
                    printf ("Failed to validate database header\n");
                    return -1;
                }
        }

    if (read_employees (dbfd, dbhdr, &employees) != STATUS_SUCCESS)
        {
            printf ("Failed to read employees");
            return 0;
        }

    if (addstring)
        {
            if (add_employee (dbhdr, &employees, addstring) != STATUS_SUCCESS)
                {
                    printf ("Error adding employee");
                    return 0;
                }
        }

    if (list)
        {
            list_employees (dbhdr, employees);
        }

    output_file (dbfd, dbhdr, employees);

    return 0;
}
