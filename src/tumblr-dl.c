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

char* extract_url(char* content, char* regex_s)
{
  char* url;
  regex_t regex;
  int reti;
  char msgbuf[100];
  regmatch_t matches[2];

  reti = regcomp(&regex, regex_s, REG_EXTENDED);
  if (reti) {
    fprintf(stderr, "Regex Compile Error\n");
    return "";
  }

  reti = regexec(&regex, content, 2, matches, 0);
  if (!reti) {
    // puts("Match");
    // printf("Line: %.*s\n", (int)(matches[0].rm_eo - matches[0].rm_so), content + matches[0].rm_so);
    // printf("Group: %.*s\n", (int)(matches[1].rm_eo - matches[1].rm_so), content + matches[1].rm_so);
    asprintf(&url, "%.*s", (int)(matches[1].rm_eo - matches[1].rm_so), content + matches[1].rm_so);
  }
  else if (reti == REG_NOMATCH) {
    puts("No match");
  }
  else {
    regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return "";
  }

  regfree(&regex);
  return url;
}

int main(int argc, char* argv[])
{
  char* url = argv[1];
  char* target_url;
  FILE* file = fopen("./tumblr-video.mp4", "wb");

  struct memory_struct chunk;
  chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;

  curl(url, write_memory_callback, &chunk);
  target_url = extract_url(chunk.memory, "<iframe src=['\"]([^']+)['\"].*tumblr_video_iframe[^>]+>");

  curl(target_url, write_memory_callback, &chunk);
  target_url = extract_url(chunk.memory, "<source src=['\"]([^\"]+)['\"][^>]+>");

  curl(target_url, write_file_callback, file);

  free(chunk.memory);
  fclose(file);
  return 0;
}
