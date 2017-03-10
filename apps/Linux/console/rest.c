#include "rest.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  char* request = (char*) stream;
  size_t maxlen = size * nmemb;
  size_t len = strlen(request);
  strncpy(ptr, request, maxlen);
  return len < maxlen ? len : maxlen;
}

static CURL* curlInit(const char* url, struct MemoryStruct* mem)
{
  mem->memory = malloc(1);
  mem->size = 0;

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)mem);

  return curl;
}

static void curlCleanup(CURL* curl, struct curl_slist* headers, struct MemoryStruct* mem)
{
  free(mem->memory);
  if (headers)
    curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
}

static long curlExecute(CURL* curl, struct curl_slist* headers, struct MemoryStruct* mem, char* result, size_t size)
{
  if (headers)
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    fprintf(stderr, "REST call failed: %s\n", curl_easy_strerror(res));
    return -1;
  }
  strncpy(result, mem->memory, size);
  return mem->size;
}

static void addHeader(struct curl_slist** headers, const char* header)
{
  *headers = curl_slist_append(*headers, header);
}

long restGet(const char* url, char* result, size_t size)
{
  return restPost(url, 0, result, size);
}

long restPost(const char* url, const char* request, char* result, size_t size)
{
  struct MemoryStruct mem;
  struct curl_slist* headers = NULL;
  CURL* curl = curlInit(url, &mem);
  addHeader(&headers, "content-type: application/json");
  if (request)
  {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
  }
  size_t res = curlExecute(curl, headers, &mem, result, size);
  curlCleanup(curl, headers, &mem);
  return res;
}

long restPut(const char* url, const char* request, char* result, size_t size)
{
  struct MemoryStruct mem;
  struct curl_slist* headers = NULL;
  CURL* curl = curlInit(url, &mem);
  addHeader(&headers, "content-type: application/json");
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(curl, CURLOPT_READDATA, request);
  curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)strlen(request));
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(curl, CURLOPT_PUT, 1L);
  size_t res = curlExecute(curl, headers, &mem, result, size);
  curlCleanup(curl, headers, &mem);
  return res;
}

void runRestTest()
{
  const size_t BUF_SIZE=1000;
  char buf[BUF_SIZE];

  long res = restGet("http://localhost:8080/test", buf, BUF_SIZE);
  if (res >= 0) printf("Result: %s\n", buf);
  res = restPut("http://localhost:8080/testput", "{\"testkey\":\"Dette er en test\"}", buf, BUF_SIZE);
  if (res >= 0) printf("Result: %s\n", buf);
//  res = restPost("http://localhost:8080/testPost", "{\"testkey\":\"Dette er en test\"}", buf, BUF_SIZE);
//  if (res >= 0) printf("Result: %s\n", buf);
//  res = restGet("http://google.dk", buf, BUF_SIZE);
//  if (res >= 0) printf("Result: %s\n", buf);
//  res = restGet("http://192.168.1.108:8080", buf, BUF_SIZE);
//  if (res >= 0) printf("Result: %s\n", buf);
}
