#pragma once

#include <gestic_api.h>
#include <stdlib.h>

struct MemoryStruct
{
  char *memory;
  size_t size;
};

long restGet(const char* url, char* result, size_t size);
long restPost(const char* url, const char* request, char* result, size_t size);
long restPut(const char* url, const char* request, char* result, size_t size);
void runRestTest();
