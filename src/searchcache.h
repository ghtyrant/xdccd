#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "dccbot.h"
#include "botmanager.h"

namespace xdccd
{

static const std::chrono::duration<double> MAX_CACHE_AGE(5 * 60);

struct SearchResult
{
    SearchResult(std::size_t total_results, std::size_t result_start);
    std::size_t total_results;
    std::size_t result_start;
    std::vector<DCCAnnouncePtr> announces;
};

typedef std::shared_ptr<SearchResult> SearchResultPtr;

struct CacheEntry
{
    CacheEntry(const std::string &query, const std::vector<DCCAnnouncePtr> &announces);
    std::string query;
    std::vector<DCCAnnouncePtr> announces;
    std::chrono::time_point<std::chrono::system_clock> created;
};

typedef std::unique_ptr<CacheEntry> CacheEntryPtr;

class SearchCache
{
    public:
        static SearchCache& get_instance()
        {
            static SearchCache instance;
            return instance;
        }

        // Delete copy constructor & assignment
        SearchCache(SearchCache const&) = delete;
        void operator=(SearchCache const&) = delete;

        SearchResultPtr search(BotManager &manager, const std::string &query, std::size_t start = 0);

    private:
        SearchCache() {};

        std::mutex cache_lock;
        std::map<std::string, CacheEntryPtr> cache;

};

}
