#include <switch.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>
#include <mbedtls/base64.h>
#include <mbedtls/md5.h>
#include <jansson.h>

#include "common.h"
#include "fs.h"

#define BUFFER_SIZE (16 * 1024) /* 16 KB */
#define DW(p, n) (*((u32 *)(p) + (n)))

static void unpack_node_key(u8 node_key[32], u8 aes_key[16], u8 nonce[8]) {
  if (aes_key) {
    DW(aes_key, 0) = DW(node_key, 0) ^ DW(node_key, 4);
    DW(aes_key, 1) = DW(node_key, 1) ^ DW(node_key, 5);
    DW(aes_key, 2) = DW(node_key, 2) ^ DW(node_key, 6);
    DW(aes_key, 3) = DW(node_key, 3) ^ DW(node_key, 7);
  }

  if (nonce) {
    DW(nonce, 0) = DW(node_key, 4);
    DW(nonce, 1) = DW(node_key, 5);
  }
}

typedef struct write_result {
  u8 *data;
  u32 pos;
} write_result;

typedef struct write_encrypted_result {
  FILE *file;

  u32 pos;
  u32 fileSize;

  Aes128CtrContext ctx;
} write_encrypted_result;

typedef struct binary_data {
  u8 *data;
  u8 length;
} binary_data;

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream) {
  write_result *result = (write_result *) stream;

  if (result->pos + size * nmemb >= BUFFER_SIZE - 1) {
    fprintf(stderr, "error: too small buffer\n");
    return 0;
  }

  memcpy(result->data + result->pos, ptr, size * nmemb);
  result->pos += size * nmemb;

  return size * nmemb;
}

static size_t write_and_decrypt_response(void *ptr, size_t size, size_t nmemb, void *stream) {
  write_encrypted_result *result = (write_encrypted_result *) stream;
  u32 step = size * nmemb;

  u8 *newDataDecrypted = malloc(step);
  aes128CtrCrypt(&result->ctx, newDataDecrypted, (u8 *) ptr, step);
  fwrite(newDataDecrypted, 1, step, result->file);
  free(newDataDecrypted);

  u64 prevPos = ((u64) result->pos * 80) / result->fileSize;
  result->pos += step;
  u64 newPos = ((u64) result->pos * 80) / result->fileSize;
  for (u64 i = 0; i < newPos - prevPos; i++) {
    printf("|");
  }

  consoleUpdate(NULL);
  return step;
}

static binary_data fromBase64(char *data) {
  size_t len = strlen(data);
  u8 add = (len * 3) & 0x03;
  size_t full_len = len + add;

  char *buf = malloc(full_len + 1);

  char *p = buf;
  for (u8 i = 0; i < len; i++) {
    if (data[i] == '-')
      *p = '+';
    else if (data[i] == '_')
      *p = '/';
    else
      *p = data[i];
    p++;
  }
  for (u8 i = 0; i < add; i++) {
    *p = '=';
    p++;
  }

  *p = '\0';

  size_t n;
  mbedtls_base64_decode(NULL, 0, &n, (u8 *) buf, full_len);
  u8 *buffer = malloc(n + 1);
  mbedtls_base64_decode(buffer, n + 1, &n, (u8 *) buf, full_len);
  binary_data bd = {.data = buffer, .length = (n)};
  return bd;
}

static Aes128CtrContext getCtrContext(u8 *key, u8 *iv) {
  Aes128CtrContext ctx;
  aes128CtrContextCreate(&ctx, key, iv);
  return ctx;
}

static Aes128CtrContext buildCtrContext(char *base64) {
  binary_data fileKeyBinary = fromBase64(base64);
  u8 fileKey[16];
  u8 nonce[8];
  u8 iv[16] = {0};
  unpack_node_key(fileKeyBinary.data, fileKey, nonce);
  memcpy(iv, nonce, 8);
  return getCtrContext(fileKey, iv);
}

long downloadFile(const char *url, u32 fileSize, const char *fileName, Aes128CtrContext ctx) {
  CURL *curl;

  u8 *data = malloc(BUFFER_SIZE);
  if (access(fileName, F_OK) != -1) {
    s64 size;
    FS_GetFileSize(fs, fileName, &size);
    if ((u32) size == fileSize) {
      printf("Target archive already present and has a proper size, not downloading.\n");
      return 0;
    } else {
      printf("Target archive already present but has wrong size, re-downloading it.\n");
      remove(fileName);
      return 0;
    }
  }
  FILE *fp = fopen(fileName, "ab");
  write_encrypted_result write_result = {
    .pos = 0,
    .fileSize = fileSize,
    .file = fp,
    .ctx = ctx
  };

  printf("\n 0%%            20%%            40%%            60%%            80%%            100%% ");

  curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_and_decrypt_response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // libnx specific
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // libnx specific

  CURLcode res = curl_easy_perform(curl); // perform tasks curl_easy_setopt asked before

  long response_code;
  if (res != CURLE_OK) {
    printf("\nCURL failed: %s\n", curl_easy_strerror(res));
  } else {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code >= 400) {
      printf("\n RESPONSE CODE is %ld\n", response_code);
      remove(fileName);
    }
  }

  fclose(fp);
  curl_easy_cleanup(curl);
  free(data);
  return response_code;
}

json_t *performMegaRequest(json_t *params) {
  json_auto_t *value = NULL;

  u8 *data = malloc(BUFFER_SIZE);
  write_result write_result = {.data = data, .pos = 0};

  CURL *curl = curl_easy_init();

  json_t *bodyArray = json_array();
  json_array_append_new(bodyArray, params);
  char *body = json_dumps(bodyArray, JSON_ENCODE_ANY);
  json_decref(bodyArray);

  curl_easy_setopt(curl, CURLOPT_URL, "https://g.api.mega.co.nz/cs");
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // following HTTP redirects
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // libnx specific
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // libnx specific

  CURLcode res = curl_easy_perform(curl); // perform tasks curl_easy_setopt asked before
  if (res != CURLE_OK) {
    printf("\nCURL failed: %s\n", curl_easy_strerror(res));
    return NULL;
  }

  curl_easy_cleanup(curl);

  /* zero-terminate the result */
  data[write_result.pos] = '\0';

  json_error_t error;
  json_t *root = json_loads((char *) data, 0, &error);

  if (!root) {
    fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
    printf((char *) data);
    printf("\n");
    free(data);
    return NULL;
  }
  free(data);

  return json_array_get(root, 0);
}

void downloadArchive(char *fileId, char *fileKey, char *archiveLocation) {
  json_t *getFileDataRequest = json_object();
  json_object_set_new(getFileDataRequest, "a", json_string("g"));
  json_object_set_new(getFileDataRequest, "g", json_integer(1));
  json_object_set_new(getFileDataRequest, "p", json_string(fileId));
  json_t *getFileDataResult = performMegaRequest(getFileDataRequest);

  const char *fileUrl = json_string_value(json_object_get(getFileDataResult, "g"));
  u32 fileSize = json_integer_value(json_object_get(getFileDataResult, "s"));
  printf("Archive size is %d MiB\n", fileSize / 1024 / 1024);
  downloadFile(fileUrl, fileSize, archiveLocation, buildCtrContext(fileKey));

//  json_decref(getFileDataRequest);
//  json_decref(getFileDataResult);
}
