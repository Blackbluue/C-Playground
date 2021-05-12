#include <stdio.h>     /* puts(), fopen(), fclose(), FILE, FILENAME_MAX */
#include <string.h>    /* strcmp(), strlen(), strcpy() */
#include <sys/stat.h>  /* lstat(), struct stat */
#include <dirent.h>    /* DIR*, struct dirent, opendir(), readdir() */
#include <err.h>       /* warn() */
#include <errno.h>     /* errno */

int walk_dir(char *, int, FILE *);
void write_meta(FILE *, struct stat);

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
    char *dname = "./";
    FILE *outfile = fopen("results.txt", "w");
    walk_dir(dname, WS_DEFAULT, outfile);
    fclose(outfile);

    return 0;
}

/* function to walk a directory tree 
   dname -> directory name
   spec  -> bit specification for operation */
int walk_dir(char *dname, int spec, FILE *outfile) {
    DIR *dir; /* pointer to directory */
    struct dirent *entry; /* single directory entry */
    struct stat st;  /* file stat */
    char fn[FILENAME_MAX]; /* path name string */
    int res = WALK_OK;  /* default return code */
    int len = strlen(dname);
    int dname_len;
    if(len >= FILENAME_MAX - 1)
        return WALK_NAMETOOLONG;

    /* set path name */
    strcpy(fn, dname);

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
        dname_len = len + strlen(entry->d_name);
        if(lstat(fn, &st) == -1) {
            warn("Can't stat %s", fn);
            res = WALK_BADIO;
            continue;
        }

        if(S_ISLNK(st.st_mode) && !(spec & WS_FOLLOWLINK))
            continue;  /* don't follow symlink unless told so */

        /* will be false for symlinked dirs */
        if(S_ISDIR(st.st_mode)) {
            fn[dname_len++] = '/';
            /* recursively follow dirs */
            if((spec & WS_RECURSIVE))
                walk_dir(fn, spec, outfile);
        }
        /* print file name to stdout */
        fprintf(outfile, "%s\n", fn);
        write_meta(outfile, st);
    }

    if(dir)
        closedir(dir);  /* close directory stream if open */

    return res ? res : errno ? WALK_BADIO : WALK_OK;
}

void write_meta(FILE *outfile, struct stat st) {
    fprintf(outfile, "\tINODE:\t\t\t%ld\n", st.st_ino);
    //fprintf(outfile, "\tMode:\t\t\t%d\n", st.st_mode);
    fprintf(outfile, "\tUID:\t\t\t%d\n", st.st_uid);
    fprintf(outfile, "\tGID:\t\t\t%d\n", st.st_gid);
    fprintf(outfile, "\tSize:\t\t\t%ld\n", st.st_size);
    fprintf(outfile, "\tAccess Time:\t%ld\n", st.st_atime);
    fprintf(outfile, "\tModify Time:\t%ld\n", st.st_mtime);
    fprintf(outfile, "\tCTime:\t\t\t%ld\n", st.st_ctime);
    fprintf(outfile, "\tBlock Size:\t\t%ld\n", st.st_blksize);
    fprintf(outfile, "\tBlcok Count:\t%ld\n", st.st_blocks);
}