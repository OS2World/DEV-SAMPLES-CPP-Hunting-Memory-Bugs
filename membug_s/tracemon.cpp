/* trace monitor
 * Sets up a pipe server and reads from it and writes that to a log file and
 * standard output
 */
#define INCL_DOSFILEMGR
#define INCL_DOSNMPIPES
#include <os2.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
        if(argc>2 || (argc==2 && strcmp(argv[1],"/?")==0)) {
                fprintf(stderr,"Usage: tracemon [logfile]\n");
                return -1;
        }
        const char *logfilename;
        if(argc==2)
                logfilename = argv[1];
        else
                logfilename = "tracemon.log";

        APIRET rc;
        
        HFILE hlog;
        ULONG dontcare;
        rc = DosOpen((PSZ)logfilename,
                     &hlog,
                     &dontcare,
                     0,
                     FILE_NORMAL,
                     OPEN_ACTION_CREATE_IF_NEW|OPEN_ACTION_REPLACE_IF_EXISTS,
                     OPEN_FLAGS_SEQUENTIAL|OPEN_FLAGS_NOINHERIT|OPEN_SHARE_DENYWRITE|OPEN_ACCESS_WRITEONLY,
                     NULL
                    );
        if(rc) {
                fprintf(stderr,"tracemon: DosOpen() returned %lu\n",rc);
                return 1;
        }        
        

        HPIPE hpipe;
        
        rc = DosCreateNPipe((PSZ)"\\pipe\\tracemon",
                            &hpipe,
                            NP_NOINHERIT|NP_ACCESS_INBOUND,
                            NP_WAIT|NP_TYPE_BYTE|NP_READMODE_BYTE|NP_UNLIMITED_INSTANCES,
                            4096,
                            4096,
                            5000
                           );
        if(rc) {
                fprintf(stderr,"tracemon: DosCreateNPipe() returned %lu\n",rc);
                return 2;
        }
        
        for(;;) {
                //wait for connection
                rc = DosConnectNPipe(hpipe);
                if(rc) {
                        fprintf(stderr,"tracemon: DosConnectNPipe returned %lu\n",rc);
                        return 3;
                }
                for(;;) {
                        //loop until an error occurs (most likely error_brokenpipe)
                        static char buf[4096];
                        //read some bytes
                        ULONG bytesRead=0;
                        rc = DosRead(hpipe,
                                     buf,
                                     4096,
                                     &bytesRead
                                    );
                        if(bytesRead!=0) {
                                //write to log file
                                ULONG bytesWritten;
                                DosWrite(hlog,
                                         buf,
                                         bytesRead,
                                         &bytesWritten
                                        );
                                //write to stdout        
                                DosWrite(1,
                                         buf,
                                         bytesRead,
                                         &bytesWritten
                                        );
                                        
                        }
                        if(rc || bytesRead==0) break;
                }
                DosDisConnectNPipe(hpipe);
        }        
        //This code is never reached
        //Your compiler may or may not realize this
        //DosClose(hpipe);
        //DosClose(hlog);
        //return 0;
}
