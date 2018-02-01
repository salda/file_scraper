#include "parser.cpp"
#include "response_getter.cpp"

int main(int argc, char* argv[]) {
  if (!argv[1] || !strlen(argv[1])) {
    cout << "Usage: " + string(argv[0]) + " <URL>" << endl;
    cin.sync();
    cin.ignore();
    return 0;
  }

  if (curl_global_init(CURL_GLOBAL_ALL)) {
    cout << "cURL global not initialized correctly" << endl;
    cin.sync();
    cin.ignore();
  }

  try {
    response_getter response_getter(argv[1]);
    const string response_body = response_getter.get_response();

    parser parser(response_body);
    parser.set_relative_URL_bases(response_getter.get_effective_URL());
    parser.find_and_download_files(); // TODO separate downloader from parser so downloads will be called from downloader instance
  } catch (const exception& e) {
    return -1;
  }

  curl_global_cleanup();
}