#include <fstream>
#include <forward_list>
#include <unordered_set>
#include <cmath>
#include <iomanip>
#include <curl/curl.h>
#include <iostream>

using namespace std;

struct download {
  const string file_name;
  ofstream download_file;
  CURL* cURL;
  size_t size{0};
  unsigned int Adler_32_a{1};
  unsigned long long int Adler_32_b{0};
  size_t minimum_iterations_for_overflow{257};

  download(const string file_name, const string& new_file_name, CURL* file_cURL) : file_name(file_name), download_file("download/" + new_file_name), cURL(file_cURL) {
    if (!cURL) {
      cout << "cURL not duplicated correctly" << endl;
      throw std::runtime_error("failed to duplicate cURL");
    }
    if (!download_file) {
      cout << "Error opening " << new_file_name << endl;
      throw std::runtime_error("failed to open file");
    }
  }

  bool operator==(const download& other) {
    return this == &other;
  }
};

class downloader { // TODO make singleton?
  CURL* cURL_for_copying;
  forward_list<download> downloads;
  unordered_set<string> file_names;
  static const int largest_short_int_prime_number{65521};
  int running_downloads_count{0};

  static size_t CurlWrite_CallbackFunc_File(void* contents, size_t element_size, size_t element_count, forward_list<download>::iterator download) {
    size_t additional_size = element_size * element_count;
    download->download_file.write((char*)contents, additional_size);
    size_t processed_size = 0;
    do { // TODO optimize more?
      size_t last_element = min(processed_size + download->minimum_iterations_for_overflow, additional_size);
      for (size_t i = processed_size; i != last_element; ++i) {
        download->Adler_32_a += ((unsigned char*)contents)[i];
        download->Adler_32_b += download->Adler_32_a;
      }
      processed_size = last_element;
      if (download->Adler_32_a >= largest_short_int_prime_number) {
        download->Adler_32_a %= largest_short_int_prime_number;
        download->minimum_iterations_for_overflow = download->Adler_32_a > 240 ? 256 : 257;
      }
      else
        download->minimum_iterations_for_overflow = ceil((largest_short_int_prime_number - download->Adler_32_a) / 255.0);
    } while (processed_size != additional_size);
    download->size += additional_size;
    return additional_size;
  }

  void delete_file(const string& new_file_name, forward_list<download>::iterator DL) {
    if (remove(("download/" + new_file_name).c_str())) // maybe call download.download_file.close(); before remove on some OSs
      cout << "Error deleting " << new_file_name << endl;
    downloads.remove(*DL);
  }

public:
  downloader() {
    cURL_for_copying = curl_easy_init();
    if (!cURL_for_copying) {
      cout << "cURL not initialized correctly" << endl;
      throw std::runtime_error("failed to construct");
    }
    curl_easy_setopt(cURL_for_copying, CURLOPT_FOLLOWLOCATION, 1L);
	  curl_easy_setopt(cURL_for_copying, CURLOPT_SSL_VERIFYPEER, 0L);
   	curl_easy_setopt(cURL_for_copying, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_File);
  }

  void download_file(const string& URL) {
    const string old_file_name = URL.substr(URL.find_last_of('/') + 1);
    auto return_value = file_names.insert(old_file_name);
    int number_of_duplicates = 0;
    while (!return_value.second) // TODO rename first file on file systems to <name>.0, other <name>.1, ..
      return_value = file_names.insert(old_file_name + '.' + to_string(++number_of_duplicates)); // TODO create a better strategy for naming duplicates
    const string& new_file_name = *return_value.first;
    forward_list<download>::iterator DL;
    try {
      DL = downloads.insert_after(downloads.before_begin(), download(move(old_file_name), new_file_name, curl_easy_duphandle(cURL_for_copying)));
    } catch (const exception& e) {
      return;
    }
    curl_easy_setopt(DL->cURL, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(DL->cURL, CURLOPT_WRITEDATA, DL);
    CURLcode curl_code = curl_easy_perform(DL->cURL);
    if (curl_code != CURLE_OK) {
      cout << string(curl_easy_strerror(curl_code)) << endl;
      delete_file(new_file_name, DL);
    }
    else {
  	  long respCode;
      curl_easy_getinfo(DL->cURL, CURLINFO_RESPONSE_CODE, &respCode);
      if (respCode != 200) {
        cout << "response code from " + URL + ": " + to_string(respCode) << endl;
        delete_file(new_file_name, DL);
      }
    }
  }

  ~downloader() {
    downloads.sort([](const download& a, const download& b) { return a.size > b.size; });
    for (download& download : downloads) {
      if (&download != &downloads.front())
        cout << endl;
      cout << "file name: " << setw(50) << left << download.file_name // TODO make width dynamic
        //<< " size: " << dec << setw(10) << download.size 
        << " hash: " << hex << setw(10) << right << (download.Adler_32_b % largest_short_int_prime_number << 16 | download.Adler_32_a);
      if (&download == &downloads.front())
        cout << " <- biggest file";
      curl_easy_cleanup(download.cURL);
    }
    cout << " <- smallest file" << endl;
    curl_easy_cleanup(cURL_for_copying);
  }
};