#include <myhtml/api.h>
#include "downloader.cpp"
#include <experimental/filesystem>

using namespace experimental::filesystem;

class parser {
  myhtml_t* myHTML;
  myhtml_tree_t* tree;
  string relative_URL_base, relative_URL_base_root;
  enum attribute_types { with_possible_URL, with_possible_comma_separated_URLs, with_CSS_possibly_containing_URLs_in_url_data_type };
  unordered_set<string> URLs;
  
  void iterate_attribute_occurrences_and_possibly_start_downloading(downloader& downloader, const string& attribute, attribute_types attribute_type = with_possible_URL) {
    myhtml_collection_t* tag_collection = myhtml_get_nodes_by_attribute_key(tree, NULL, NULL, attribute.c_str(), attribute.size(), NULL);

    if (tag_collection && tag_collection->list && tag_collection->length)
      for (size_t i = 0; i < tag_collection->length; ++i) {
        string attribute_value = myhtml_attribute_value(myhtml_attribute_by_key(tag_collection->list[i], attribute.c_str(), strlen(attribute.c_str())), NULL);

        if (attribute_type == with_CSS_possibly_containing_URLs_in_url_data_type) {
          size_t url_pos = attribute_value.find("url(");
          while (url_pos != string::npos) {
            size_t url_end_bracket_pos = attribute_value.find(')', url_pos + 4);
            int url_escaped = attribute_value[url_pos + 4] == '"' || attribute_value[url_pos + 4] == '\'' ? 1 : 0; // TODO maybe add range check
            construct_absolute_URL_and_possibly_start_downloading(downloader, attribute_value.substr(url_pos + 4 + url_escaped, url_end_bracket_pos - url_pos - 4 - 2 * url_escaped));
            url_pos = attribute_value.find("url(", url_end_bracket_pos + 1);
          }
        }
        else if (attribute_type == with_possible_comma_separated_URLs) {
          size_t url_pos = attribute_value.find_first_not_of(' ');
          do {
            size_t url_end_pos = attribute_value.find_first_of(" ,", url_pos + 1);
            construct_absolute_URL_and_possibly_start_downloading(downloader, attribute_value.substr(url_pos, url_end_pos - url_pos));
            url_pos = attribute_value.find_first_not_of(' ', attribute_value.find(',', url_end_pos) + 1);
          } while (url_pos != string::npos);
        }
        else
          construct_absolute_URL_and_possibly_start_downloading(downloader, move(attribute_value.erase(0, attribute_value.find_first_not_of(' '))));
      }

    myhtml_collection_destroy(tag_collection);
  }

  void construct_absolute_URL_and_possibly_start_downloading(downloader& downloader, string URL) {
    size_t pos = URL.find_first_of(" #?");
    if (pos != string::npos)
      URL.erase(pos);
    if (URL.empty() || URL.back() == '/')
      return;
    pos = URL.find("//");
    if (pos == 0 || (pos != string::npos && URL[pos - 1] == ':')) {
      if (URL.find('/', pos + 3) == string::npos)
        return;
    }
    else if (URL.front() == '/')
      URL.insert(0, relative_URL_base_root);
    else // not sure if cURL handles ".." inside URLs, but I guess it's not relevant
      URL.insert(0, relative_URL_base);

    auto return_value = URLs.insert(move(URL));
    if (return_value.second)
      downloader.start_download(*return_value.first);
  }

public:
  parser(const string& HTML) {
    myHTML = myhtml_create();
    myhtml_init(myHTML, MyHTML_OPTIONS_DEFAULT, 1, 0);
    tree = myhtml_tree_create();
    myhtml_tree_init(tree, myHTML);

    myencoding_t encoding;
    if (!myencoding_detect(HTML.c_str(), HTML.size(), &encoding))
      encoding = MyENCODING_UTF_8;

    myhtml_parse(tree, encoding, HTML.c_str(), HTML.size());
  }

  void set_relative_URL_bases(string effective_URL) {
    myhtml_collection_t* base_tag_collection = myhtml_get_nodes_by_tag_id(tree, NULL, MyHTML_TAG_BASE, NULL);
    if (base_tag_collection && base_tag_collection->list && base_tag_collection->length) {
      myhtml_tree_attr_t* base_href_attribute = myhtml_attribute_by_key(base_tag_collection->list[0], "href", strlen("href"));
      if (base_href_attribute)
        effective_URL = myhtml_attribute_value(base_href_attribute, NULL);
    }

    myhtml_collection_destroy(base_tag_collection);
    relative_URL_base_root = effective_URL.substr(0, effective_URL.find('/', effective_URL.find("//") + 2));
    relative_URL_base = effective_URL;
  }

  downloader start_downloading_referenced_files() {
    if ((!exists("download") && !create_directory("download")) // I want to delete neither the files inside possibly existing folder
    || (exists("download") && !is_directory("download")))      // nor file with a name "download"
      throw runtime_error("problem creating directory \"download\" for downloading files");

    downloader downloader;

    for (const string& attribute : { "action", "cite", "data", "formaction", "href", "manifest", "poster", "src" })
      iterate_attribute_occurrences_and_possibly_start_downloading(downloader, attribute, with_possible_URL);
    iterate_attribute_occurrences_and_possibly_start_downloading(downloader, "srcset", with_possible_comma_separated_URLs);
    iterate_attribute_occurrences_and_possibly_start_downloading(downloader, "style", with_CSS_possibly_containing_URLs_in_url_data_type);

    return downloader;
  }

  ~parser() {
    myhtml_tree_destroy(tree);
    myhtml_destroy(myHTML);
  }
};