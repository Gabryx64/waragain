#pragma once
#include <cstring>
#include <curl/curl.h>
#include <optional>
#include <string>

#include "img.h"
#include "json_struct.h"

struct RawTopic {
  uint16_t id;
  std::string gameId;
  std::string titleId;
  std::string communityBadge;
  std::string title;
  std::string titleUrl;
  std::string iconUri;
  std::string communityListIcon;
  size_t platform;
  std::string type;
  size_t viewRegion;
  size_t totalPosts;
  size_t totalReplies;
  size_t totalDeletedPosts;

  JS_OBJ(id, gameId, titleId, communityBadge, title, titleUrl, iconUri,
         communityListIcon, platform, type, viewRegion, totalPosts,
         totalReplies, totalDeletedPosts);

  inline static std::optional<RawTopic> from_json(const std::string &json) {
    RawTopic ret;

    JS::ParseContext ctx(json);
    if (ctx.error != JS::Error::NoError) {
      return {};
    }

    return ctx.parseTo(ret) == JS::Error::NoError ? std::optional{ret}
                                                  : std::nullopt;
  }
};

struct Topic {
  uint64_t game_id;
  uint64_t title_id;
  std::string name;
  size_t posts;
  std::string icon64;

  static size_t cb(void *data, size_t size, size_t nmemb, void *userp) {
    std::vector<uint8_t> &bytes = *(std::vector<uint8_t> *)userp;
    size_t cur_size = bytes.size();
    bytes.reserve(bytes.size() + size * nmemb);
    memcpy(bytes.data() + cur_size, data, size * nmemb);
    return size * nmemb;
  }

  static std::optional<Topic> from_json(const std::string &json) {
    RawTopic raw;
    if (auto ret = RawTopic::from_json(json)) {
      raw = *ret;
    } else {
      return {};
    }

    Topic ret;

    ret.game_id = std::stoull(raw.gameId);
    ret.title_id = std::stoull(raw.titleId);
    ret.name = raw.type;
    ret.posts = raw.totalPosts;

    std::string icon_link =
        std::string("https://web.archive.org/web/20171013103128im_/") +
        raw.iconUri;

    std::vector<uint8_t> png;
    CURL *curl = curl_easy_init();
    if (!curl) {
      return {};
    }
    curl_easy_setopt(curl, CURLOPT_URL, icon_link.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &png);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      return {};
    }

    if (auto res = png2tga64(png)) {
      ret.icon64 = *res;
    } else {
      return {};
    }

    return ret;
  }
};
