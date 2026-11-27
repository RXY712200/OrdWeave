#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ordweave.h"
#include <stdio.h>
#include <string.h>
_Static_assert(sizeof(OwRecordId)==8 && sizeof(OwCollectionId)==8 && sizeof(OwOccurrenceId)==8,"ID layout");
_Static_assert(OW_OK==0 && OW_CORRUPT==8 && OW_RECOVERY_REQUIRED==12 && OW_RETENTION_UNSUPPORTED==14,"status compatibility");
int main(void) {
 OwDatabase *db=NULL;OwRecordId r;OwValue value={.type=OW_INT,.as.integer=42},out={0};char cwd[MAX_PATH],path[MAX_PATH];
 if(!GetCurrentDirectoryA(MAX_PATH,cwd)) return 1;
 if(snprintf(path,sizeof(path),"%s\\consumer-%lu",cwd,GetCurrentProcessId())>=(int)sizeof(path)) return 1;
 if(strcmp(ordweave_version(),"1.1.0-preview.2")) return 2;
 if(ow_database_create(&db)!=OW_OK || ow_record_create(db,&r)!=OW_OK) return 3;
 ow_database_destroy(db);
 if(ow_database_create_persistent(path,&db)!=OW_OK || ow_record_create(db,&r)!=OW_OK || ow_field_set(db,r,"answer",&value)!=OW_OK || ow_database_close(db)!=OW_OK) return 4;
 if(ow_database_open(path,&db)!=OW_OK || ow_field_get(db,r,"answer",&out)!=OW_OK || out.as.integer!=42) return 5;
 ow_value_release(&out);ow_database_close(db);puts("external public-target C17 lifecycle verified");return 0;
}
