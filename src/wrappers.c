/*
Asunder

Copyright(C) 2005 Eric Lathrop <eric@ericlathrop.com>
Copyright(C) 2007 Andrew Smith <http://littlesvr.ca/misc/contactandrew.php>

Any code in this file may be redistributed or modified under the terms of
the GNU General Public Licence as published by the Free Software 
Foundation; version 2 of the licence.

*/

#include <sys/types.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "main.h"
#include "wrappers.h"
#include "threads.h"
#include "prefs.h"
#include "util.h"

pid_t cdparanoia_pid;
pid_t lame_pid;
pid_t oggenc_pid;
pid_t flac_pid;

int numCdparanoiaFailed;
int numLameFailed;
int numOggFailed;
int numFlacFailed;

int numCdparanoiaOk;
int numLameOk;
int numOggOk;
int numFlacOk;

int numchildren = 0;

void blockSigChld(void)
{
    sigset_t block_chld;
    
    sigemptyset(&block_chld);
    sigaddset(&block_chld, SIGCHLD);
    
    sigprocmask(SIG_BLOCK, &block_chld, NULL);
}

void unblockSigChld(void)
{
    sigset_t block_chld;
    
    sigemptyset(&block_chld);
    sigaddset(&block_chld, SIGCHLD);
    
    sigprocmask(SIG_UNBLOCK, &block_chld, NULL);
}

// signal handler to find out when our child has exited
void sigchld(int signum)
{
    int status;
    pid_t pid;
    
    pid = wait(&status);
    
    if (pid != cdparanoia_pid && pid != lame_pid && pid != oggenc_pid && pid != flac_pid)
        printf("SIGCHLD for unknown pid, report bug please\n");
#ifdef DEBUG
    printf("%d exited with %d: ", pid, status);
    if (WIFEXITED(status))
        printf("exited, status=%d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("killed by signal %d\n", WTERMSIG(status));
    else if (WIFSTOPPED(status))
        printf("stopped by signal %d\n", WSTOPSIG(status));
    else if (WIFCONTINUED(status))
        printf("continued\n");
    fflush(NULL);
#endif

    if (status != 0)
    {
        if (pid == cdparanoia_pid)
        {
            cdparanoia_pid = 0;
            if(global_prefs->rip_wav)
                numCdparanoiaFailed++;
        }
        else if (pid == lame_pid)
        {
            lame_pid = 0;
            numLameFailed++;
        }
        else if (pid == oggenc_pid)
        {
            oggenc_pid = 0;
            numOggFailed++;
        }
        else if (pid == flac_pid)
        {
            flac_pid = 0;
            numFlacFailed++;
        }
    }
    else
    {
        if (pid == cdparanoia_pid)
        {
            cdparanoia_pid = 0;
            if(global_prefs->rip_wav)
                numCdparanoiaOk++;
        }
        else if (pid == lame_pid)
        {
            lame_pid = 0;
            numLameOk++;
        }
        else if (pid == oggenc_pid)
        {
            oggenc_pid = 0;
            numOggOk++;
        }
        else if (pid == flac_pid)
        {
            flac_pid = 0;
            numFlacOk++;
        }
    }
}

// fork() and exec() the file listed in "args"
//
// args - a valid array for execvp()
// toread - the file descriptor to pipe back to the parent
// p - a place to write the PID of the exec'ed process
// 
// returns - a file descriptor that reads whatever the program outputs on "toread"
int exec_with_output(const char * args[], int toread, pid_t * p)
{
    int pipefd[2];
    
    blockSigChld();
    
    if (pipe(pipefd) != 0)
        fatalError("exec_with_output(): failed to create a pipe");
    
    if ((*p = fork()) == 0)
    {
        // im the child
        // i get to execute the command
        
        //!! Sleep a little (hopefully enough) to allow the *p to actually get set.
        //!! For some reason despite the blockSigChld() above
        //!! sigchld() is called anyway after this thread ends
        //!! and before the unblockSigChld() below.
        //!! To reproduce, set to encode in every possible format
        //!! and try a few times on the 3 song CD.
        usleep(100000);
        
        // close the side of the pipe we don't need
        close(pipefd[0]);
        
        /* this causes a segfault on fedora, and I don't really see why it's needed anyway */
        //~ // close all standard streams to keep output clean
        //~ close(STDOUT_FILENO);
        //~ close(STDIN_FILENO);
        //~ close(STDERR_FILENO);
        
        /* instead redirect to /dev/null */
        if (gbl_null_fd != -1)
        {
            dup2(STDOUT_FILENO, gbl_null_fd);
            close(STDOUT_FILENO);
            dup2(STDERR_FILENO, gbl_null_fd);
            close(STDERR_FILENO);
        }
        
        // setup output
        dup2(pipefd[1], toread);
        close(pipefd[1]);
        
        // call execvp
        execvp(args[0], (char **)args);
        
        // should never get here
        fatalError("exec_with_output(): execvp() failed");
    }
    
#ifdef DEBUG
    int count;
    printf("%d started: %s ", *p, args[0]);
    for (count = 1; args[count] != NULL; count++)
    {
        printf("%s ", args[count]);
    }
    printf("\n");
    fflush(NULL);
#endif
    
    // i'm the parent, get ready to wait for children
    numchildren++;
    
    // close the side of the pipe we don't need
    close(pipefd[1]);
    
    unblockSigChld();
    
    return pipefd[0];
}

// uses cdparanoia to rip a WAV track from a cdrom
//
// cdrom    - the path to the cdrom device
// tracknum - the track to rip
// filename - the name of the output WAV file
// progress - the percent done
void cdparanoia(char * cdrom, int tracknum, char * filename, double * progress)
{
    int fd;
    char buf[256];
    int size;
    int pos;
    
    int start;
    int end;
    int code;
    char type[20];
    int sector;
    
    char trackstring[3];
    
    snprintf(trackstring, 3, "%d", tracknum);
    
    const char * args[] = { "cdparanoia", "-e", "-d", cdrom, trackstring, filename, NULL };
    
    fd = exec_with_output(args, STDERR_FILENO, &cdparanoia_pid);
    
    // to convert the progress number stat cdparanoia spits out
    // into sector numbers divide by 1176
    // note: only use the "[wrote]" numbers
    do
    {
        pos = -1;
        do
        {
            pos++;
            size = read(fd, &buf[pos], 1);
            
            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                size = 1;
            }
            
        } while ((buf[pos] != '\n') && (size > 0));
        buf[pos] = '\0';

        if ((buf[0] == 'R') && (buf[1] == 'i'))
        {
            sscanf(buf, "Ripping from sector %d", &start);
        } else if (buf[0] == '\t') {
            sscanf(buf, "\t to sector %d", &end);
        } else if (buf[0] == '#') {
            sscanf(buf, "##: %d %s @ %d", &code, type, &sector);
            sector /= 1176;
            if (strncmp("[wrote]", type, 7) == 0)
            {
                *progress = (double)(sector-start)/(end-start);
            }
        }
    } while (size > 0);
    
    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (cdparanoia_pid != 0)
    {
#ifdef DEBUG
        printf("w3");
#endif
        usleep(100000);
    }
    
}

// uses LAME to encode a WAV file into a MP3 and tag it
//
// tracknum - the track number
// artist - the artist's name
// album - the album the song came from
// title - the name of the song
// wavfilename - the path to the WAV file to encode
// mp3filename - the path to the output MP3 file
// vbr - (boolean) wether or not to encode with Variable Bit Rate
// bitrate - the bitrate to encode at (or maximum bitrate if using VBR)
// progress - the percent done
void lame(int tracknum,
        char * artist,
        char * album,
        char * title,
        char * wavfilename,
        char * mp3filename,
        int vbr,
        int bitrate,
        double * progress)
{
    int fd;

    char buf[256];
    int size;
    int pos;
    
    int sector;
    int end;

    char tracknum_text[3];
    char bitrate_text[4];
    const char * args[15];

    snprintf(tracknum_text, 3, "%d", tracknum);
    snprintf(bitrate_text, 4, "%d", bitrate);
    
    pos = 0;
    args[pos++] = "lame";
    if (vbr)
    {
        args[pos++] = "-v";
        args[pos++] = "-B";
    } else {
        args[pos++] = "-b";
    }
    args[pos++] = bitrate_text;
    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "--tn";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "--ta";
        args[pos++] = artist;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "--tl";
        args[pos++] = album;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "--tt";
        args[pos++] = title;
    }
    args[pos++] = wavfilename;
    args[pos++] = mp3filename;
    args[pos++] = NULL;

    fd = exec_with_output(args, STDERR_FILENO, &lame_pid);
    
    do
    {
        pos = -1;
        do
        {
            pos++;
            size = read(fd, &buf[pos], 1);
            
            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                size = 1;
            }
            
        } while ((buf[pos] != '\r') && (buf[pos] != '\n') && (size > 0));
        buf[pos] = '\0';
        
        if (sscanf(buf, "%d/%d", &sector, &end) == 2)
        {
            *progress = (double)sector/end;
        }
    } while (size > 0);
    
    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (lame_pid != 0)
    {
#ifdef DEBUG
        printf("w4");
#endif
        usleep(100000);
    }
    
}

// uses oggenc to encode a WAV file into a OGG and tag it
//
// tracknum - the track number
// artist - the artist's name
// album - the album the song came from
// title - the name of the song
// wavfilename - the path to the WAV file to encode
// oggfilename - the path to the output OGG file
// quality_level - how hard to compress the file (0-10)
// progress - the percent done
void oggenc(int tracknum,
        char * artist,
        char * album,
        char * title,
        char * wavfilename,
        char * oggfilename,
        int quality_level,
        double * progress)
{
    int fd;

    char buf[256];
    int size;
    int pos;
    
    int sector;
    int end;

    char tracknum_text[3];
    char quality_level_text[3];
    const char * args[14];

    snprintf(tracknum_text, 3, "%d", tracknum);
    snprintf(quality_level_text, 3, "%d", quality_level);
    
    pos = 0;
    args[pos++] = "oggenc";
    args[pos++] = "-q";
    args[pos++] = quality_level_text;
    
    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "-N";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "-a";
        args[pos++] = artist;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "-l";
        args[pos++] = album;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "-t";
        args[pos++] = title;
    }
    args[pos++] = wavfilename;
    args[pos++] = "-o";
    args[pos++] = oggfilename;
    args[pos++] = NULL;
    
    fd = exec_with_output(args, STDERR_FILENO, &oggenc_pid);
    
    do
    {
        pos = -1;
        do
        {
            pos++;
            size = read(fd, &buf[pos], 1);
            
            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                size = 1;
            }
            
        } while ((buf[pos] != '\r') && (buf[pos] != '\n') && (size > 0));
        buf[pos] = '\0';

        if (sscanf(buf, "\t[\t%d.%d%%]", &sector, &end) == 2)
        {
            *progress = (double)(sector + (end*0.1))/100;
        }
        else if (sscanf(buf, "\t[\t%d,%d%%]", &sector, &end) == 2)
        {
            *progress = (double)(sector + (end*0.1))/100;
        }
    } while (size > 0);
    
    close(fd);
    /* don't go on until the signal for the previous call is handled */
    while (oggenc_pid != 0)
    {
#ifdef DEBUG
        printf("w5");
#endif
        usleep(100000);
    }
    
}

// uses the FLAC reference encoder to encode a WAV file into a FLAC and tag it
//
// tracknum - the track number
// artist - the artist's name
// album - the album the song came from
// title - the name of the song
// wavfilename - the path to the WAV file to encode
// flacfilename - the path to the output FLAC file
// compression_level - how hard to compress the file (0-8) see flac man page
// progress - the percent done
void flac(int tracknum,
        char * artist,
        char * album,
        char * title,
        char * wavfilename,
        char * flacfilename,
        int compression_level,
        double * progress)
{
    int fd;

    char buf[256];
    int size;
    int pos;
    
    int sector;
    
    char tracknum_text[15];
    char * artist_text;
    char * album_text;
    char * title_text;
    char compression_level_text[3];
    const char * args[15];
    
    snprintf(tracknum_text, 15, "TRACKNUMBER=%d", tracknum);
    
    artist_text = malloc(sizeof(char) * (strlen(artist)+8));
    if (artist_text == NULL)
        fatalError("malloc(sizeof(char) * (strlen(artist)+8)) failed. Out of memory.");
    snprintf(artist_text, strlen(artist)+8, "ARTIST=%s", artist);

    album_text = malloc(sizeof(char) * (strlen(album)+7));
    if (album_text == NULL)
        fatalError("malloc(sizeof(char) * (strlen(album)+7)) failed. Out of memory.");
    snprintf(album_text, strlen(album)+7, "ALBUM=%s", album);
    
    title_text = malloc(sizeof(char) * (strlen(title)+7));
    if (title_text == NULL)
        fatalError("malloc(sizeof(char) * (strlen(title)+7) failed. Out of memory.");
    snprintf(title_text, strlen(title)+7, "TITLE=%s", title);
    
    snprintf(compression_level_text, 3, "-%d", compression_level);
    
    pos = 0;
    args[pos++] = "flac";
    args[pos++] = "-f";
    args[pos++] = compression_level_text;
    if ((tracknum > 0) && (tracknum < 100))
    {
        args[pos++] = "-T";
        args[pos++] = tracknum_text;
    }
    if ((artist != NULL) && (strlen(artist) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = artist_text;
    }
    if ((album != NULL) && (strlen(album) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = album_text;
    }
    if ((title != NULL) && (strlen(title) > 0))
    {
        args[pos++] = "-T";
        args[pos++] = title_text;
    }
    args[pos++] = wavfilename;
    args[pos++] = "-o";
    args[pos++] = flacfilename;
    args[pos++] = NULL;
    
    fd = exec_with_output(args, STDERR_FILENO, &flac_pid);
    
    free(artist_text);
    free(album_text);
    free(title_text);
    
    do
    {
        pos = -1;
        do
        {
            pos++;
            size = read(fd, &buf[pos], 1);
            
            if (size == -1 && errno == EINTR)
            /* signal interrupted read(), try again */
            {
                pos--;
                size = 1;
            }
            
        } while ((buf[pos] != '\r') && (buf[pos] != '\n') && (size > 0));
        buf[pos] = '\0';

        for (; pos>0; pos--)
        {
            if (buf[pos] == ':')
            {
                pos++;
                break;
            }
        }

        if (sscanf(&buf[pos], "%d%%", &sector) == 1)
        {
            *progress = (double)sector/100;
        }
    } while (size > 0);
    
    close(fd);
    
    /* don't go on until the signal for the previous call is handled */
    while (flac_pid != 0)
    {
#ifdef DEBUG
        printf("w6");
#endif
        usleep(100000);
    }
}
