#ifndef ASKUE_FILESYSTEM_H_
#define ASKUE_FILESYSTEM_H_

#ifndef ASKUE_DEBUG

    #define ASKUE_FILE_DB               "/mnt/askue/journal/askue.db.sqlite3"
    #define ASKUE_FILE_LOG              "/mnt/askue/reports/askue.log"
    #define ASKUE_FILE_PROTOCOL         "/mnt/askue/reports/askue.protocol"
    #define ASKUE_FILE_CONFIG           "/etc/askue/askue.cfg"
    #define ASKUE_FILE_HELP             "/etc/askue/askue.help"
    #define ASKUE_FILE_PID              "/var/askue.pid"
    
#else
    
    #define ASKUE_FILE_DB               "../../test/askue.db.sqlite3"
    #define ASKUE_FILE_LOG              "../../test/askue.log"
    #define ASKUE_FILE_PROTOCOL         "../../test/askue.protocol"
    #define ASKUE_FILE_CONFIG           "../../test/askue.cfg"
    #define ASKUE_FILE_HELP             "../../test/askue.help"
    #define ASKUE_FILE_PID              "../../test/askue.pid"

#endif

#endif /* ASKUE_FILESYSTEM_H_ */
