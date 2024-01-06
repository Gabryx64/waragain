#include <content_redirection/redirection.h>
#include <coreinit/title.h>
#include <cstdlib>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <wups.h>
#include <wups/hooks.h>

#include "config.h"
#include "meta.h"
#include "region.h"

WUPS_PLUGIN_NAME("waragain");
WUPS_PLUGIN_DESCRIPTION("Repopulate the Wara Wara Plaza through"
                        " the public archiverse.guide api.");
WUPS_PLUGIN_VERSION(VERSION);
WUPS_PLUGIN_AUTHOR("Gabryx86_64");
WUPS_PLUGIN_LICENSE("Zlib");

WUPS_USE_WUT_DEVOPTAB();

namespace fs = std::filesystem;

CRLayerHandle cl_handl [[gnu::section(".data")]] = 0;

INITIALIZE_PLUGIN() { curl_global_init(CURL_GLOBAL_ALL); }

ON_APPLICATION_START() {
  uint64_t tid = OSGetTitleID();
  Region reg;
  switch (tid) {
  case 0x00050010'10040200:
    reg = Region::Europe;
    break;

  case 0x00050010'10040100:
    reg = Region::America;
    break;

  case 0x00050010'10040000:
    reg = Region::Japan;
    break;

  default:
    return;
  }

  Config cfg(reg);
  std::string xml;
  if (auto res = cfg.to_xml()) {
    xml = *res;
  } else {
    return;
  }

  fs::path checkpath{"fs:/vol/content"};
  const char *regstr;
  switch (reg) {
  case Region::Europe:
    regstr = "Eu";
    break;

  case Region::America:
    regstr = "Us";
    break;

  case Region::Japan:
    regstr = "Jp";
    break;
  }

  std::vector<std::string> dirs;
  std::string s;
  for (auto const &f : fs::directory_iterator{checkpath}) {
    std::string name = f.path().filename().string();
    if (name.size() != 2 && name[0] == regstr[0] && name[1] == regstr[1]) {
      dirs.push_back(name);
      s.append(name);
    }
  }

  OSFatal(s.c_str());

  fs::path tmp = fs::temp_directory_path();
  fs::create_directory(tmp / "waragain");
  for (auto const &dir : dirs) {
    fs::create_directory(tmp / "waragain" / dir);
    std::ofstream out(tmp / "waragain" / dir / "1stNUP.xml");
    out << xml;
  }

  ContentRedirection_AddFSLayer(&cl_handl, "waragain",
                                (tmp / "waragain").string().c_str(),
                                FS_LAYER_TYPE_CONTENT_MERGE);
}

ON_APPLICATION_ENDS() {
  if (cl_handl != 0) {
    ContentRedirection_RemoveFSLayer(cl_handl);
    cl_handl = 0;
  }
}

DEINITIALIZE_PLUGIN() { curl_global_cleanup(); }
