#pragma once
#include <cstring>
#include <curl/curl.h>
#include <mxml.h>
#include <optional>
#include <sstream>
#include <string>
#include <wups.h>
#include <wups/meta.h>

#include "inicpp.h"
#include "json_struct.h"
#include "region.h"
#include "topic.h"

/*
Topic positions in Wara Wara Plaza (number is 0-based index):
  3  5  7  2
8            9
  0  6  4  1
*/

class Config {
private:
  uint64_t game_ids[10];

public:
  Config(Region reg) {
    // Defaults (10 best selling Wii U games):
    switch (reg) {
    case Region::Europe:
      game_ids[0] = 14866558073268532646u;
      game_ids[1] = 14866558073090176828u;
      game_ids[2] = 14866558072986287271u;
      game_ids[3] = 14866558073037299866u;
      game_ids[4] = 14866558072985245727u;
      game_ids[5] = 14866558073673370369u;
      game_ids[6] = 6437256808751874782u;
      game_ids[7] = 14866558073038706245u;
      game_ids[8] = 14866558073070242127u;
      game_ids[9] = 14866558073605628444u;
      break;

    case Region::America:
      game_ids[0] = 14866558073268532646u;
      game_ids[1] = 14866558073088060601u;
      game_ids[2] = 14866558072985245729u;
      game_ids[3] = 14866558073037299866u;
      game_ids[4] = 14866558073673551269u;
      game_ids[5] = 14866558073673370369u;
      game_ids[6] = 6437256808751874782u;
      game_ids[7] = 14866558073038702637u;
      game_ids[8] = 14866558073070141981u;
      game_ids[9] = 14866558073606562115u;
      break;

    case Region::Japan:
      game_ids[0] = 14866558073268532646u;
      game_ids[1] = 14866558073086179650u;
      game_ids[2] = 14866558072987250173u;
      game_ids[3] = 14866558073037299866u;
      game_ids[4] = 14866558073673172583u;
      game_ids[5] = 14866558073673370369u;
      game_ids[6] = 6437256808751874782u;
      game_ids[7] = 14866558073038678957u;
      game_ids[8] = 14866558073071750223u;
      game_ids[9] = 14866558073592323832u;
      break;
    }
  }

  static size_t cb(void *data, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)data, size * nmemb);
    return size * nmemb;
  }

  std::optional<std::string> to_xml() const {
    mxml_node_t *root = mxmlNewElement(NULL, "result");
    mxmlNewInteger(mxmlNewElement(root, "version"), 1);
    mxmlNewInteger(mxmlNewElement(root, "has_error"), 0);
    mxmlNewText(mxmlNewElement(root, "request_name"), 0, "topics");
    mxmlNewText(mxmlNewElement(root, "expire"), 0, "2100-01-01 10:00:00");

    CURL *curl = curl_easy_init();
    if (!curl) {
      mxmlDelete(root);
      return {};
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://archiverse.guide/api/community/getgame");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(
        curl, CURLOPT_HTTPHEADER,
        curl_slist_append(curl_slist_append(NULL, "Accept: application/json"),
                          "Content-Type: application/json"));

    mxml_node_t *topics = mxmlNewElement(root, "topics");
    for (uint64_t game_id : game_ids) {
      std::string json;
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);

      std::stringstream req("{ \"gameId\": \"");
      req << game_id;
      req << "\" }";
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.str().c_str());

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        mxmlDelete(root);
        curl_easy_cleanup(curl);

        return {};
      }

      Topic comm;
      if (auto res = Topic::from_json(json)) {
        comm = *res;
      } else {
        mxmlDelete(root);
        curl_easy_cleanup(curl);

        return {};
      }

      mxml_node_t *topic = mxmlNewElement(topics, "topic");
      mxmlNewText(mxmlNewElement(topic, "icon"), 0, comm.icon64.c_str());
      mxmlNewInteger(mxmlNewElement(topic, "title_id"), comm.title_id);
      mxmlNewInteger(mxmlNewElement(topic, "community_id"), game_id);
      mxmlNewInteger(mxmlNewElement(topic, "is_recommended"), 0);
      mxmlNewText(mxmlNewElement(topic, "name"), 0, comm.name.c_str());
      mxmlNewInteger(mxmlNewElement(topic, "participant_count"), 0);
      mxmlNewElement(topic, "people");
      mxmlNewInteger(mxmlNewElement(topic, "empathy_count"), 0);
      mxmlNewInteger(mxmlNewElement(topic, "has_shop_page"),
                     0); // There is no shop anymore sooo...
      mxmlNewText(
          mxmlNewElement(topic, "modified_at"), 0,
          "2017-11-08 07:00:00"); // Miiverse shutdown date (idk if the time is
                                  // supposed to be UTC but it is so whatever).
      mxmlNewInteger(mxmlNewElement(topic, "position"), 2);
    }

    curl_easy_cleanup(curl);

    const char *xml_raw = mxmlSaveAllocString(root, MXML_NO_CALLBACK);
    std::string xml = std::string(xml_raw);
    delete[] xml_raw;

    mxmlDelete(root);

    return xml;
  }
};
