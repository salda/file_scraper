#include <curl/curl.h>
#include <iostream>

using namespace std;

class response_getter { // TODO make singleton?
  CURL* cURL;
  char* URL;

  static size_t CurlWrite_CallbackFunc_String(void* contents, size_t element_size, size_t element_count, string* output) {
    size_t additional_size = element_size * element_count;
    output->append((char*)contents, additional_size);
    return additional_size;
  }

public:
  response_getter(char* URL) : URL(URL) {
    cURL = curl_easy_init();
    if (!cURL) {
      cout << "cURL not initialized correctly" << endl;
      throw runtime_error("failed to construct");
    }
    curl_easy_setopt(cURL, CURLOPT_URL, URL);
    curl_easy_setopt(cURL, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(cURL, CURLOPT_SSL_VERIFYPEER, 0L);
  }

  const string get_response() {
    curl_easy_setopt(cURL, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_String);
    string response_body;
    curl_easy_setopt(cURL, CURLOPT_WRITEDATA, &response_body);

    CURLcode curl_code = curl_easy_perform(cURL);
    if (curl_code != CURLE_OK) {
      cout << string(curl_easy_strerror(curl_code));
      throw runtime_error("failed to perform");
    }

    long respCode;
    curl_easy_getinfo(cURL, CURLINFO_RESPONSE_CODE, &respCode);
    if (respCode != 200) {
      cout << "response code from " + string(URL) + ": " + to_string(respCode);
      throw runtime_error("response code not 200");
    }

    if (response_body.empty()) {
      cout << "no response from " + string(URL);
      throw runtime_error("response body empty");
    }
    
    return response_body;
  }

  string get_effective_URL() { // TODO handle that it can be called before get_response
    char* URL = NULL;
    curl_easy_getinfo(cURL, CURLINFO_EFFECTIVE_URL, &URL);
    if (!URL)
      throw runtime_error("effective URL empty");
    return URL;
  }

  ~response_getter() {
    curl_easy_cleanup(cURL);
  }
};
