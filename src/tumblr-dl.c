#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>

struct memory_struct {
  char* memory;
  size_t size;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct memory_struct *mem = (struct memory_struct*) userp;
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);

  if (mem->memory == NULL) {
    /* out of memory */
    fprintf(stderr, "Not Enough Memory Error \n");
    return 1;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int request_content(char* url, char** content)
{
  CURL* curl;
  CURLcode result;

  struct memory_struct chunk;
  chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
  chunk.size = 0;

  curl = curl_easy_init();

  if (!curl) {
    fprintf(stderr, "Failed");
    return 1;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/46.0.2460.0 Safari/537.36"); /* requesting like Chrome */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &chunk);

  result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fprintf(stderr, "Request Falied Error: %s\n", curl_easy_strerror(result));
    return 2;
  }

  /* chunk.memory points to a memory block that is chunk.size bytes big with the content */
  printf("%lu bytes received\n", (long) chunk.size);
  *content = chunk.memory;

  curl_easy_cleanup(curl);
  free(chunk.memory);

  return 0;
}

int extract_url(char* content, char* regex_s, char** url)
{
  regex_t regex;
  int reti;
  char msgbuf[100];
  regmatch_t matches[2];

  reti = regcomp(&regex, regex_s, REG_EXTENDED);
  if (reti) {
    fprintf(stderr, "Regex Compile Error\n");
    return 1;
  }

  reti = regexec(&regex, content, 2, matches, 0);
  if (!reti) {
    puts("Match");
    // printf("Line: %.*s\n", (int)(matches[0].rm_eo - matches[0].rm_so), content + matches[0].rm_so);
    // printf("Group: %.*s\n", (int)(matches[1].rm_eo - matches[1].rm_so), content + matches[1].rm_so);
    asprintf(url, "%.*s", (int)(matches[1].rm_eo - matches[1].rm_so), content + matches[1].rm_so);
  }
  else if (reti == REG_NOMATCH) {
    puts("No match");
  }
  else {
    regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return 2;
  }

  regfree(&regex);
  return 0;
}

int main(int argc, char* argv[])
{
  char* url = argv[1];
  char* content;
  char* target_url;

  request_content(url, &content);
  extract_url(content, "<iframe src=['\"]([^']+)['\"].*tumblr_video_iframe[^>]+>", &target_url);

  request_content(target_url, &content);
  extract_url(content, "<source src=['\"]([^\"]+)['\"][^>]+>" , &target_url);

  printf("%s\n", target_url);

  return 0;
}
