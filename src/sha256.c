/* Adapted from Obscura64 3db0122560853a8f3db3ba6888067394dc1a3e62; MIT, (c)
 * 2026 RXY712200. */
#define WIN32_LEAN_AND_MEAN
#include "wal_internal.h"
// clang-format off
// clang-format off
#include <windows.h>
#include <bcrypt.h>
// clang-format on
// clang-format on

static int hash_segment(BCRYPT_HASH_HANDLE hash, const void *bytes,
                        size_t length) {
  const unsigned char *p = (const unsigned char *)bytes;
  while (length != 0) {
    ULONG chunk = length > MAXDWORD ? MAXDWORD : (ULONG)length;
    if (!BCRYPT_SUCCESS(BCryptHashData(hash, (PUCHAR)p, chunk, 0)))
      return 0;
    p += chunk;
    length -= chunk;
  }
  return 1;
}

OwStatus ow_sha256(const void *first, size_t first_len, const void *second,
                   size_t second_len, unsigned char digest[32]) {
  BCRYPT_ALG_HANDLE alg = NULL;
  BCRYPT_HASH_HANDLE hash = NULL;
  OwStatus status = OW_IO;
  if ((first == NULL && first_len != 0) ||
      (second == NULL && second_len != 0) || digest == NULL)
    return OW_INVALID;
  if (!BCRYPT_SUCCESS(
          BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0)))
    goto done;
  if (!BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0)))
    goto done;
  if (!hash_segment(hash, first, first_len) ||
      !hash_segment(hash, second, second_len))
    goto done;
  if (BCRYPT_SUCCESS(BCryptFinishHash(hash, digest, 32, 0)))
    status = OW_OK;
done:
  if (hash != NULL)
    BCryptDestroyHash(hash);
  if (alg != NULL)
    BCryptCloseAlgorithmProvider(alg, 0);
  return status;
}
