#ifndef _RAR_EXTRACT_
#define _RAR_EXTRACT_

#include "archive.h"
#include "rdwrfn.h"
#include "unrar.h"

enum EXTRACT_ARC_CODE {EXTRACT_ARC_NEXT,EXTRACT_ARC_REPEAT};

class CmdExtract
{
  private:
    ComprDataIO DataIO;
    Unpack *Unp;
    long TotalFileCount;

    long FileCount;
    long MatchedArgs;
    bool FirstFile;
    bool AllMatchesExact;
    bool ReconstructDone;

    char ArcName[NM];
    wchar ArcNameW[NM];

    char Password[MAXPASSWORD];
    bool PasswordAll;
    bool PrevExtracted;
    bool SignatureFound;
    char DestFileName[NM];
    wchar DestFileNameW[NM];
  public:
    CmdExtract();
    ~CmdExtract();
    void				DoExtract			(CommandData *Cmd, UncompressedArchive *_uncomp);
    void				ExtractArchiveInit	(CommandData *Cmd, Archive &Arc);
    EXTRACT_ARC_CODE	ExtractArchive		(CommandData *Cmd, UncompressedArchive *_uncomp);
    bool				ExtractCurrentFile	(CommandData *Cmd, Archive &Arc,
											 int HeaderSize, bool &Repeat);
    static void			UnstoreFile			(ComprDataIO &DataIO,Int64 DestUnpSize);
};

#endif
