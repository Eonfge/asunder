#include <sys/types.h>
#include <stdbool.h>

extern pid_t cdparanoia_pid;
extern pid_t lame_pid;
extern pid_t oggenc_pid;
extern pid_t flac_pid;

extern int numCdparanoiaFailed;
extern int numLameFailed;
extern int numOggFailed;
extern int numFlacFailed;

extern int numCdparanoiaOk;
extern int numLameOk;
extern int numOggOk;
extern int numFlacOk;

// signal handler to find out when out child has exited
void sigchld(int signum);

// fork() and exec() the file listed in "args"
//
// args - a valid array for execvp()
// toread - the file descriptor to pipe back to the parent
// p - a place to write the PID of the exec'ed process
// 
// returns - a file descriptor that reads whatever the program outputs on "toread"
int exec_with_output(const char * args[], int toread, pid_t * p);

// uses cdparanoia to rip a WAV track from a cdrom
//
// cdrom    - the path to the cdrom device
// tracknum - the track to rip
// filename - the name of the output WAV file
// progress - the percent done
void cdparanoia(char * cdrom, int tracknum, char * filename, double * progress);

// uses LAME to encode a WAV file into a MP# and tag it
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
		double * progress);

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
		double * progress);

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
		double * progress);
