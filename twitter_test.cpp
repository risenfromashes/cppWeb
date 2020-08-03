#include "src/Json.h"
#include <fstream>

struct Metadata {
    std::string result_type;
    std::string iso_language_code;
    JSONABLE(Metadata);
};
SET_JSONABLE(Metadata, result_type, iso_language_code);

struct Url {
    std::string url;
    std::string expanced_url;
    std::string display_url;
    unsigned    indices[2];
    JSONABLE(Url);
};
SET_JSONABLE(Url, url, expanced_url, display_url, indices);

struct Urls {
    std::vector<Url> urls;
    JSONABLE(Urls);
};
SET_JSONABLE(Urls, urls);

struct Size {
    unsigned    w;
    unsigned    h;
    std::string resize;
    JSONABLE(Size);
};
SET_JSONABLE(Size, w, h, resize);

struct Sizes {
    Size large;
    Size medium;
    Size thumb;
    Size small;
    JSONABLE(Sizes);
};
SET_JSONABLE(Sizes, large, medium, thumb, small);

struct UserMention {
    std::string screen_name;
    std::string name;
    size_t      id;
    std::string id_str;
    unsigned    indices[2];
    JSONABLE(UserMention);
};
SET_JSONABLE(UserMention, screen_name, name, id, id_str, indices);

struct HashTag {
    std::string text;
    int         indices[2];
    JSONABLE(HashTag);
};
SET_JSONABLE(HashTag, text, indices);

struct Media {
    size_t      id;
    std::string id_str;
    unsigned    indices[2];
    std::string media_url;
    std::string media_url_https;
    std::string url;
    std::string display_url;
    std::string type;
    Sizes       sizes;
    size_t      source_status_id;
    std::string source_status_id_str;
    JSONABLE(Media);
};
SET_JSONABLE(Media,
             id,
             id_str,
             indices,
             media_url,
             media_url_https,
             url,
             display_url,
             type,
             sizes,
             source_status_id,
             source_status_id_str);

struct Entities {
    std::vector<HashTag>     hashtags;
    std::vector<std::string> symbols;
    Urls                     url;
    std::vector<Url>         urls;
    std::vector<UserMention> user_mentions;
    std::vector<Media>       media;
    JSONABLE(Entities);
};
SET_JSONABLE(Entities, hashtags, symbols, url, urls, user_mentions, media);

struct User {
    size_t       id;
    std::string  id_str;
    std::string  name;
    std::string  screen_name;
    std::string  location;
    std::string  description;
    std::string* url;
    Entities     entites;
    bool         _protected;
    size_t       followers_count;
    size_t       friends_count;
    size_t       listed_count;
    std::string  created_at;
    size_t       favourites_count;
    int*         utc_offset;
    std::string* time_zone;
    bool         verified;
    size_t       status_count;
    std::string  lang;
    bool         contributors_enabled;
    bool         is_translator;
    bool         is_translation_enabled;
    std::string  profile_background_color;
    std::string  profile_background_image_url;
    std::string  profile_background_image_url_https;
    std::string  profile_background_title;
    std::string  profile_image_url;
    std::string  profile_image_url_https;
    std::string  profile_banner_url;
    std::string  profile_link_color;
    std::string  profile_sidebar_border_color;
    std::string  profile_sidebar_fill_color;
    std::string  profile_text_color;
    bool         profile_use_background_image;
    bool         default_profile;
    bool         default_profile_image;
    bool         following;
    bool         follow_request_sent;
    bool         notifications;
    JSONABLE(User);
};
SET_JSONABLE(User,
             id,
             id_str,
             name,
             screen_name,
             location,
             description,
             url,
             entites,
             _protected,
             followers_count,
             friends_count,
             listed_count,
             created_at,
             favourites_count,
             utc_offset,
             time_zone,
             verified,
             status_count,
             lang,
             contributors_enabled,
             is_translator,
             is_translation_enabled,
             profile_background_color,
             profile_background_image_url,
             profile_background_image_url_https,
             profile_background_title,
             profile_image_url,
             profile_image_url_https,
             profile_banner_url,
             profile_link_color,
             profile_sidebar_border_color,
             profile_sidebar_fill_color,
             profile_text_color,
             profile_use_background_image,
             default_profile,
             default_profile_image,
             following,
             follow_request_sent,
             notifications);

struct Coordinate {
    double x, y;
    JSONABLE(Coordinate);
};
SET_JSONABLE(Coordinate, x, y);

struct Status {
    Metadata                  metadata;
    std::string               created_at;
    size_t                    id;
    std::string               id_str;
    std::string               text;
    std::string               source;
    bool                      truncated;
    size_t*                   in_reply_to_status_id;
    std::string*              in_reply_to_status_id_str;
    size_t*                   in_reply_to_user_id;
    std::string*              in_reply_to_user_id_str;
    std::string*              in_reply_to_screen_name;
    User                      user;
    std::string*              geo;
    std::vector<Coordinate>*  coordinates;
    std::string*              place;
    std::vector<std::string>* contributors;
    Status*                   retweeted_status;
    size_t                    retweet_count;
    size_t                    favorite_count;
    Entities                  entities;
    bool                      favorited;
    bool                      retweeted;
    std::string               lang;
    JSONABLE(Status);
};
SET_JSONABLE(Status,
             metadata,
             created_at,
             id,
             id_str,
             text,
             source,
             truncated,
             in_reply_to_status_id,
             in_reply_to_status_id_str,
             in_reply_to_user_id,
             in_reply_to_user_id_str,
             in_reply_to_screen_name,
             user,
             geo,
             coordinates,
             place,
             contributors,
             retweeted_status,
             retweet_count,
             favorite_count,
             entities,
             favorited,
             retweeted,
             lang);

struct SearchMetadata {
    double      completed_in;
    size_t      max_id;
    std::string max_id_str;
    std::string next_results;
    std::string query;
    std::string refresh_url;
    size_t      count;
    size_t      since_id;
    std::string since_id_str;
    JSONABLE(SearchMetadata);
};
SET_JSONABLE(SearchMetadata,
             completed_in,
             max_id,
             max_id_str,
             next_results,
             query,
             refresh_url,
             count,
             since_id,
             since_id_str);

struct Twitter {
    std::vector<Status> statuses;
    // SearchMetadata      search_metadata;
    JSONABLE(Twitter);
};
SET_JSONABLE(Twitter, statuses);

struct A {
    int          x, y;
    std::string  f;
    std::string* name;
    A() { std::cout << "Ctor" << std::endl; }
    JSONABLE(A);
};
SET_JSONABLE(A, x, y, name);

int main()
{

    // std::ifstream  file("test.json");
    // cW::JsonParser parser(file);
    // A*             a = A::fromJson(parser.rootNode);
    cW::Clock::start();
    std::ifstream  file("test_resources/twitter.json");
    cW::JsonParser parser(file);
    auto           tweets = Twitter::parseJson(parser.rootNode);
    std::cout << "Count of tweet statuses: " << tweets.statuses.size() << std::endl;
}