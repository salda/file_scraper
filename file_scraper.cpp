#include "parser.cpp"
#include "response_getter.cpp"

downloader initialize_downloads(char* URL) {
    response_getter response_getter(URL);
    const string response_body = response_getter.get_response();

    parser parser(response_body);
    parser.set_relative_URL_bases(response_getter.get_effective_URL());
    
    return parser.start_downloading_referenced_files();
}

int main(int argc, char* argv[]) {
  if (!argv[1] || !strlen(argv[1])) {
    cout << "Usage: " + string(argv[0]) + " <HTML5_valid_source_URL>" << endl;
    cin.sync();
    cin.ignore();
    return 0;
  }

  if (curl_global_init(CURL_GLOBAL_ALL)) {
    cout << "cURL global not initialized correctly" << endl;
    cin.sync();
    cin.ignore();
    return -1;
  }

  try {
    while (true) {
      downloader downloader = initialize_downloads(argv[1]);
      downloader.wait_for_running_downloads();
    }
  } catch (const exception& e) {
    return -1;
  }

  curl_global_cleanup();
}