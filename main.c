#include <stdio.h>     /* puts(), FILENAME_MAX */
#include <string.h>    /* strcmp(), strlen(), strcpy() */
#include <sys/stat.h>  /* lstat(), struct stat */
#include <dirent.h>    /* DIR*, struct dirent, opendir(), readdir() */
#include <err.h>       /* warn() */
#include <errno.h>     /* errno */

int walk_dir(char *, int);

/* error numbers */
enum {
    WALK_OK,
    WALK_NAMETOOLONG,
    WALK_BADIO
};

#define WS_NONE        0
#define WS_RECURSIVE   (1 << 0)      /* recursively search directory tree */
#define WS_DEFAULT     WS_RECURSIVE  /* default behavior */
#define WS_FOLLOWLINK  (1 << 1)      /* follow symbolic links */
#define WS_DOTFILES    (1 << 2)      /* handle dot files */

int main(void) {
    char *dname = ".";
    walk_dir(dname, WS_DEFAULT);  // fix spec

    return 0;
}

/* function to walk a directory tree 
   dname -> directory name
   spec  -> bit specification for operation */
int walk_dir(char *dname, int spec) {
    DIR *dir; /* pointer to directory */
    struct dirent *entry; /* single directory entry */
    struct stat st;  /* file stat */
    char fn[FILENAME_MAX]; /* path name string */
    int res = WALK_OK;  /* default return code */
    int len = strlen(dname);
    if(len >= FILENAME_MAX - 1)
        return WALK_NAMETOOLONG;

    /* set path name */
    strcpy(fn, dname);
    fn[len++] = '/';

    /* check if directory can be opened */
    if(!(dir = opendir(dname))) {
        warn("can't open %s", dname);
        return WALK_BADIO;
    }

    errno = 0;
    /* loop over directory for each entry */
    while((entry = readdir(dir))) {
        if(!(spec & WS_DOTFILES) && entry->d_name[0] == '.')
            continue;  /* skip hidden files if requested */
        if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;  /* skip "." and ".." */

        strncpy(fn + len, entry->d_name, FILENAME_MAX - len);
        if(lstat(fn, &st) == -1) {
            warn("Can't stat %s", fn);
            res = WALK_BADIO;
            continue;
        }

        if(S_ISLNK(st.st_mode) && !(spec & WS_FOLLOWLINK))
            continue;  /* don't follow symlink unless told so */

        /* will be false for symlinked dirs */
        if(S_ISDIR(st.st_mode)) {
            /* recursively follow dirs */
            if((spec & WS_RECURSIVE))
                walk_dir(fn, spec);
        }
        /* print file name to stdout */
        puts(fn);
    }

    if(dir)
        closedir(dir);  /* close directory stream if open */

    return res ? res : errno ? WALK_BADIO : WALK_OK;
}