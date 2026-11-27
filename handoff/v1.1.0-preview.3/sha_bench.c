#define WIN32_LEAN_AND_MEAN
#include "wal_internal.h"
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>
static double now(void){LARGE_INTEGER a,b;QueryPerformanceCounter(&a);QueryPerformanceFrequency(&b);return(double)a.QuadPart/b.QuadPart;}
int main(void){unsigned char expected[32],got[32];if(ow_sha256("abc",3,NULL,0,expected)!=OW_OK)return 2;
for(unsigned repeat=0;repeat<4;repeat++)for(unsigned mode=0;mode<3;mode++){
 BCRYPT_ALG_HANDLE reusable=NULL;if(mode==2&&!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&reusable,BCRYPT_SHA256_ALGORITHM,NULL,0)))return 3;
 double start=now();for(unsigned i=0;i<10000;i++){
  if(mode==0){if(ow_sha256("abc",3,NULL,0,got)!=OW_OK||memcmp(got,expected,32))return 4;}
  else if(mode==1){BCRYPT_ALG_HANDLE a;if(!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&a,BCRYPT_SHA256_ALGORITHM,NULL,0)))return 5;BCryptCloseAlgorithmProvider(a,0);}
  else {BCRYPT_HASH_HANDLE h;if(!BCRYPT_SUCCESS(BCryptCreateHash(reusable,&h,NULL,0,NULL,0,0)))return 6;if(!BCRYPT_SUCCESS(BCryptHashData(h,(PUCHAR)"abc",3,0))||!BCRYPT_SUCCESS(BCryptFinishHash(h,got,32,0)))return 7;BCryptDestroyHash(h);if(memcmp(got,expected,32))return 8;}
 }double elapsed=now()-start;if(reusable)BCryptCloseAlgorithmProvider(reusable,0);printf("%u,%u,10000,%.9f\n",repeat,mode,elapsed);
}return 0;}
