/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include "util.h"

void fatalError(const char* message)
{
    fprintf(stderr, "Fatal error: %s\n", message);
    exit(-1);
}

// construct a filename from various parts
//
// path - the path the file is placed in (don't include a trailing '/')
// dir - the parent directory of the file (don't include a trailing '/')
// file - the filename
// extension - the suffix of a file (don't include a leading '.')
//
// NOTE: caller must free the returned string!
// NOTE: any of the parameters may be NULL to be omitted
char * make_filename(const char * path, const char * dir, const char * file, const char * extension)
{
    int len = 1;
    char * ret = NULL;
    int pos = 0;
    
    if (path)
    {
        len += strlen(path) + 1;
    }
    if (dir)
    {
        len += strlen(dir) + 1;
    }
    if (file)
    {
        len += strlen(file);
    }
    if (extension)
    {
        len += strlen(extension) + 1;
    }
    
    ret = malloc(sizeof(char) * len);
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * len) failed. Out of memory.");
    
    if (path)
    {
        strncpy(&ret[pos], path, strlen(path));
        pos += strlen(path);
        ret[pos] = '/';
        pos++;
    }
    if (dir)
    {
        strncpy(&ret[pos], dir, strlen(dir));
        pos += strlen(dir);
        ret[pos] = '/';
        pos++;
    }
    if (file)
    {
        strncpy(&ret[pos], file, strlen(file));
        pos += strlen(file);
    }
    if (extension)
    {
        ret[pos] = '.';
        pos++;
        strncpy(&ret[pos], extension, strlen(extension));
        pos += strlen(extension);
    }
    ret[pos] = '\0';

    return ret;
}

// substitute various items into a formatted string (similar to printf)
//
// format - the format of the filename
// tracknum - gets substituted for %N in format
// artist - gets substituted for %A in format
// album - gets substituted for %L in format
// title - gets substituted for %T in format
//
// NOTE: caller must free the returned string!
char * parse_format(const char * format, int tracknum, const char * artist, const char * album, const char * title)
{
    unsigned i = 0;
    int len = 0;
    char * ret = NULL;
    int pos = 0;
    
    for (i=0; i<strlen(format); i++)
    {
        if ((format[i] == '%') && (i+1 < strlen(format)))
        {
            switch (format[i+1])
            {
                case 'A':
                    if (artist) len += strlen(artist);
                    break;
                case 'L':
                    if (album) len += strlen(album);
                    break;
                case 'N':
                    if ((tracknum > 0) && (tracknum < 100)) len += 2;
                    break;
                case 'T':
                    if (title) len += strlen(title);
                    break;
                case '%':
                    len += 1;
                    break;
            }
            
            i++; // skip the character after the %
        } else {
            len++;
        }
    }
    
    ret = malloc(sizeof(char) * (len+1));
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * (len+1)) failed. Out of memory.");
    
    for (i=0; i<strlen(format); i++)
    {
        if ((format[i] == '%') && (i+1 < strlen(format)))
        {
            switch (format[i+1])
            {
                case 'A':
                    if (artist)
                    {
                        strncpy(&ret[pos], artist, strlen(artist));
                        pos += strlen(artist);
                    }
                    break;
                case 'L':
                    if (album)
                    {
                        strncpy(&ret[pos], album, strlen(album));
                        pos += strlen(album);
                    }
                    break;
                case 'N':
                    if ((tracknum > 0) && (tracknum < 100))
                    {
                        ret[pos] = '0'+((int)tracknum/10);
                        ret[pos+1] = '0'+(tracknum%10);
                        pos += 2;
                    }
                    break;
                case 'T':
                    if (title)
                    {
                        strncpy(&ret[pos], title, strlen(title));
                        pos += strlen(title);
                    }
                    break;
                case '%':
                    ret[pos] = '%';
                    pos += 1;
            }
            
            i++; // skip the character after the %
        } else {
            ret[pos] = format[i];
            pos++;
        }
    }
    ret[pos] = '\0';
    
    return ret;
}

// searches $PATH for the named program
// returns 1 if found, 0 otherwise
int program_exists(const char * name)
{
    unsigned i;
    unsigned numpaths = 1;
    char * path;
    char * strings;
    char ** paths;
    struct stat s;
    int ret = 0;
    char * filename;
    
    path = getenv("PATH");
    strings = malloc(sizeof(char) * (strlen(path)+1));
    if (strings == NULL)
        fatalError("malloc(sizeof(char) * (strlen(path)+1)) failed. Out of memory.");
    strncpy(strings, path, strlen(path)+1);
    
    for (i=0; i<strlen(path); i++)
    {
        if (strings[i] == ':')
        {
            numpaths++;
        }
    }
    paths = malloc(sizeof(char *) * numpaths);
    if (paths == NULL)
        fatalError("malloc(sizeof(char *) * numpaths) failed. Out of memory.");
    numpaths = 1;
    paths[0] = &strings[0];
    for (i=0; i<strlen(path); i++)
    {
        if (strings[i] == ':')
        {
            strings[i] = '\0';
            paths[numpaths] = &strings[i+1];
            numpaths++;
        }
    }
    
    for (i=0; i<numpaths; i++)
    {
        filename = make_filename(paths[i], NULL, name, NULL);
        if (stat(filename, &s) == 0)
        {
            ret = 1;
            free(filename);
            break;
        }
        free(filename);
    }
    
    free(strings);
    free(paths);
    
    return ret;
}

// reads an entire line from a file and returns it
//
// NOTE: caller must free the returned string!
char * read_line(int fd)
{
    int pos = 0;
    char cur;
    char * ret;
    int rc;
    
    do
    {
        rc = read(fd, &cur, 1);
        if (rc != 1)
        {
            // If a read fails before a newline, the file is corrupt
            // or it's from an old version.
            pos = 0;
            break;
        }
        pos++;
    } while (cur != '\n');
    
    if (pos == 0)
        return NULL;
    
    ret = malloc(sizeof(char) * pos);
    if (ret == NULL)
        fatalError("malloc(sizeof(char) * pos) failed. Out of memory.");
    
    lseek(fd, -pos, SEEK_CUR);
    rc = read(fd, ret, pos);
    if (rc != pos)
    {
        free(ret);
        return NULL;
    }
    ret[pos-1] = '\0';
    
    return ret;
}

// reads an entire line from a file and turns it into a number
int read_line_num(int fd)
{
    long ret = 0;
    
    char * line = read_line(fd);
    if (line == NULL)
        return 0;
    
    ret = strtol(line, NULL, 10);
    if (ret == LONG_MIN || ret == LONG_MAX)
        ret = 0;
    
    free(line);
    
    return ret;
}

// Uses mkdir() for every component of the path
// and returns if any of those fails with anything other than EEXIST.
int recursive_mkdir(char* pathAndName, mode_t mode)
{
    int count;
    int pathAndNameLen = strlen(pathAndName);
    int rc;
    char charReplaced;
    
    for(count = 0; count < pathAndNameLen; count++)
    {
        if(pathAndName[count] == '/')
        {
            charReplaced = pathAndName[count + 1];
            pathAndName[count + 1] = '\0';
            
            rc = mkdir(pathAndName, mode);
            
            pathAndName[count + 1] = charReplaced;
            
            if(rc != 0 && !(errno == EEXIST || errno == EISDIR))
                return rc;
        }
    }
    
    // in case the path doesn't have a trailing slash:
    return mkdir(pathAndName, mode);
}

// Uses mkdir() for every component of the path except the last one,
// and returns if any of those fails with anything other than EEXIST.
int recursive_parent_mkdir(char* pathAndName, mode_t mode)
{
    int count;
    bool haveComponent = false;
    int rc = 1; // guaranteed fail unless mkdir is called
    
    // find the last component and cut it off
    for(count = strlen(pathAndName) - 1; count >= 0; count--)
    {
        if(pathAndName[count] != '/')
            haveComponent = true;
        
        if(pathAndName[count] == '/' && haveComponent)
        {
            pathAndName[count] = 0;
            rc = mkdir(pathAndName, mode);
            pathAndName[count] = '/';
        }
    }
    
    return rc;
}

// removes all instances of bad characters from the string
//
// str - the string to trim
// bad - the sting containing all the characters to remove
void trim_chars(char * str, const char * bad)
{
    int i;
    int pos;
    int len = strlen(str);
    unsigned b;
    
    for (b=0; b<strlen(bad); b++)
    {
        pos = 0;
        for (i=0; i<len+1; i++)
        {
            if (str[i] != bad[b])
            {
                str[pos] = str[i];
                pos++;
            }
        }
    }
}

// removes leading and trailing whitespace as defined by isspace()
//
// str - the string to trim
void trim_whitespace(char * str)
{
    int i;
    int pos = 0;
    int len = strlen(str);
    
    // trim leading space
    for (i=0; i<len+1; i++)
    {
        if (!isspace(str[i]) || (pos > 0))
        {
            str[pos] = str[i];
            pos++;
        }
    }
    
    // trim trailing space
    len = strlen(str);
    for (i=len-1; i>=0; i--)
    {
        if (!isspace(str[i]))
        {
            break;
        }
        str[i] = '\0';
    }
}
