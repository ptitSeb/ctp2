#ifndef PRJFILE_H
#define PRJFILE_H


#include "c3files.h"


typedef struct _iobuf FILE;
typedef void *HANDLE;
struct ZFS_FHEADER;
struct ZFS_DTABLE;

#define MAX_PRJFILE_PATHS 16
#define MAX_RECORDNAME_LENGTH 16

enum PRJFILE_PATH_TYPE {
    PRJFILE_PATH_NULL,
    PRJFILE_PATH_DOS,                   
    PRJFILE_PATH_ZFS,                   
    PRJFILE_PATH_ZMS                    
};

struct PFPath {
    PRJFILE_PATH_TYPE type;

    FILE *zfs_fp;

    void *zms_start;
    void *zms_end;
    HANDLE zms_hf;
    HANDLE zms_hm;

    char dos_path[256];
};

struct PFEntry {
    char rname[MAX_RECORDNAME_LENGTH];
    long offset;
    long size;
    long path;
};

class ProjectFile {
private:
    PFPath m_paths[MAX_PRJFILE_PATHS];
    long m_num_paths;

    PFEntry *m_entries;
    long m_num_entries;

    int mergeEntries(PFEntry *newList, int newCount);
    PFEntry *findRecord(char *rname);

    int readDOSdir(long path, PFEntry *table);
    int verify_ZFS_header(ZFS_FHEADER *header);
    void read_ZFS_dtable(int pathnum, ZFS_DTABLE *dtable,
                         PFEntry **tlp, long *rcount);

    int addPath_DOS(char *path);
    int addPath_ZFS(char *path);
    int addPath_ZMS(char *path);

    void *getData_DOS(PFEntry *entry, long *size, C3DIR dir = C3DIR_DIRECT);
    void *getData_ZFS(PFEntry *entry, long *size);
    void *getData_ZMS(PFEntry *entry, long *size);
    void *getData_ZMS(PFEntry *entry, long *size, 
                      HANDLE *hFileMap, long *offset);

    char m_error_string[256];

public:
    ProjectFile();
    ~ProjectFile();

    
    int addPath(char *path, int use_filemapping = 0);

    
    void *getData(char *rname, long *size, C3DIR dir = C3DIR_DIRECT);

    
    
    void *getData(char *rname, long *size, HANDLE *hFileMap, long *offset);

    
    void freeData(void *);

    
    int exists(char *fname);

    
    char *getError() { return m_error_string; } 
};


#endif