#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>

typedef struct {
  char* memory;
  size_t size;
} blob_t;

typedef size_t (write_cb)(void* content, size_t size, size_t nmemb, void* stream);

static size_t write_memory_callback(void* content, size_t size, size_t nmemb, void* stream)
{
  size_t realsize = size * nmemb;
  blob_t* blob = (blob_t*) stream;
  blob->memory = realloc(blob->memory, blob->size + realsize + 1);

  if (blob->memory == NULL) {
    /* out of memory */
    fprintf(stderr, "Not Enough Memory Error \n");
    return 1;
  }

  memcpy(&(blob->memory[blob->size]), content, realsize);
  blob->size += realsize;
  blob->memory[blob->size] = 0;

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
    fprintf(stderr, "Request Failed Error: %s\n", curl_easy_strerror(result));
    return 2;
  }

  curl_easy_cleanup(curl);

  return 0;
}

int extract_str(char* content, char* pattern, char*** matches, int** n_matches)
{
  regex_t preg;
  int reti = regcomp(&preg, pattern, REG_EXTENDED);

  if (reti) {
    fprintf(stderr, "Regex Compile Error\n");
    return 1;
  }

  *n_matches = malloc(sizeof(int));
  **n_matches = preg.re_nsub + 1;

  regmatch_t pmath[**n_matches];
  reti = regexec(&preg, content, **n_matches, pmath, 0);

  if (reti) {
    char msgbuf[100];
    regerror(reti, &preg, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Regex match failed: %s\n", msgbuf);
    return 2;
  }

  *matches = malloc(sizeof(char*) * **n_matches);
  for (int i = 0; i < **n_matches; ++i) {
    //printf("%lu, %d: %.*s\n", **n_matches, i, (int)(pmath[i].rm_eo - pmath[i].rm_so), content + pmath[i].rm_so);
    asprintf(&(*matches)[i], "%.*s", (int)(pmath[i].rm_eo - pmath[i].rm_so), content + pmath[i].rm_so);
  }

  regfree(&preg);
  return 0;
}

int main(int __unused argc, char* argv[])
{
  char* url = argv[1];
  char* filename;
  char** target_url;
  char** filename_parts;
  int* n_filename_parts;
  int* n_matches;

  blob_t blob;
  blob.memory = malloc(1); /* will be grown as needed by the realloc above */
  blob.size = 0;

  // request the whole page content
  curl(url, write_memory_callback, &blob);

  // extract iframe url
  extract_str(blob.memory, "<iframe src=['\"]([^']+)['\"].*tumblr_video_iframe[^>]+>", &target_url, &n_matches);

  // reset the blob
  free(blob.memory);
  blob.memory = malloc(1); /* will be grown as needed by the realloc above */
  blob.size = 0;

  // request the iframe content
  curl(target_url[1], write_memory_callback, &blob);

  // free iframe url data
  for (int i=0; i < *n_matches; i++) free(target_url[i]);
  free(target_url);
  free(n_matches);

  // extract filename from original post url
  extract_str(url, "^https?://([^.]+)[.]tumblr.com/post/([0-9]+).*$", &filename_parts, &n_filename_parts);
  asprintf(&filename, "%s-%s.mp4", filename_parts[1], filename_parts[2]);

  // free filename extraction data
  for (int i=0; i < *n_filename_parts; i++) free(filename_parts[i]);
  free(filename_parts);
  free(n_filename_parts);

  // extract video url from iframe content
  extract_str(blob.memory, "<source src=['\"]([^\"]+)['\"][^>]+>", &target_url, &n_matches);

  // free iframe data
  free(blob.memory);

  // create a file to write the video
  FILE* file = fopen(filename, "wb");

  // free video filename data
  free(filename);

  // write the video to a file
  curl(target_url[1], write_file_callback, file);
  fclose(file);

  // free video url data
  for (int i=0; i < *n_matches; i++) free(target_url[i]);
  free(target_url);
  free(n_matches);

  return 0;
}
