#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>

struct memory_struct {
  char* memory;
  size_t size;
};

typedef size_t (write_cb)(void* content, size_t size, size_t nmemb, void* stream);

static size_t write_memory_callback(void* content, size_t size, size_t nmemb, void* stream)
{
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct*) stream;
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);

  if (mem->memory == NULL) {
    /* out of memory */
    fprintf(stderr, "Not Enough Memory Error \n");
    return 1;
  }

  memcpy(&(mem->memory[mem->size]), content, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static size_t write_file_callback(void* content, size_t size, size_t nmemb, void* stream)
{
  size_t written = fwrite(content, size, nmemb, stream);
  return written;
}

int curl(char* url, write_cb write_function, void* write_data)
{
  CURL *curl;
  CURLcode result;
  curl = curl_easy_init();

  if (!curl){
    fprintf(stderr, "Failed");
    return 1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2460.0 Safari/537.36"); /* requesting like Chrome */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) write_data);

  result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fprintf(stderr, "Request Falied Error: %s\n", curl_easy_strerror(result));
    return 2;
  }

  curl_easy_cleanup(curl);

  return 0;
}

char** extract_str(char* content, char* pattern)
{
  regex_t preg;
  int reti = regcomp(&preg, pattern, REG_EXTENDED);

  if (reti) {
    fprintf(stderr, "Regex Compile Error\n");
    return NULL;
  }

  regmatch_t pmath[preg.re_nsub + 1];
  reti = regexec(&preg, content, preg.re_nsub + 1, pmath, 0);

  if (reti) {
    char msgbuf[100];
    regerror(reti, &preg, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return NULL;
  }

  char** matches = malloc(sizeof(char*) * (preg.re_nsub + 1));
  for (int i = 0; i < preg.re_nsub + 1; ++i) {
    // printf("%d: %.*s\n", i, (int)(pmath[i].rm_eo - pmath[i].rm_so), content + pmath[i].rm_so);
    asprintf(&matches[i], "%.*s", (int)(pmath[i].rm_eo - pmath[i].rm_so), content + pmath[i].rm_so);
  }

  regfree(&preg);
  return matches;
}

int main(int argc, char* argv[])
{
  char* url = argv[1];
  char** target_url;
  char* filename;
  char** filename_parts = extract_str(url, "^https?://([^.]+)[.]tumblr.com/post/([0-9]+)/.*$");
  if (!filename_parts) return 1;

  struct memory_struct chunk;
  chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;

  curl(url, write_memory_callback, &chunk);
  target_url = extract_str(chunk.memory, "<iframe src=['\"]([^']+)['\"].*tumblr_video_iframe[^>]+>");

  curl(target_url[1], write_memory_callback, &chunk);
  target_url = extract_str(chunk.memory, "<source src=['\"]([^\"]+)['\"][^>]+>");

  asprintf(&filename, "%s-%s.mp4", filename_parts[1], filename_parts[2]);
  FILE* file = fopen(filename, "wb");

  curl(target_url[1], write_file_callback, file);

  free(chunk.memory);
  fclose(file);
  return 0;
}
