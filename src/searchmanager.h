#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "dccbot.h"
#include "botmanager.h"

namespace xdccd
{

namespace search
{
static const std::chrono::duration<double> MAX_CACHE_AGE(5 * 60);
static const std::size_t RESULTS_PER_PAGE(25);
}

struct SearchResultItem
{
    SearchResultItem(DCCAnnouncePtr announce, unsigned int score);

    DCCAnnouncePtr announce;
    unsigned int score;
};

typedef std::unique_ptr<SearchResultItem> SearchResultItemPtr;

struct SearchResult
{
    SearchResult(std::size_t total_results, std::size_t result_start);
    std::size_t total_results;
    std::size_t result_start;
    std::vector<SearchResultItemPtr>::const_iterator begin;
    std::vector<SearchResultItemPtr>::const_iterator end;
};

typedef std::shared_ptr<SearchResult> SearchResultPtr;

struct CacheEntry
{
    CacheEntry(const std::string &query, std::vector<SearchResultItemPtr> results);
    std::string query;
    std::vector<SearchResultItemPtr> results;
    std::chrono::time_point<std::chrono::system_clock> created;
};

typedef std::unique_ptr<CacheEntry> CacheEntryPtr;

class SearchManager
{
    public:
        SearchResultPtr search(BotManager &manager, const std::string &query, std::size_t start = 0, std::size_t limit = 25);
        void clear();

    private:
        void search_in_announces(const std::map<std::string, xdccd::DCCAnnouncePtr> &announces, const std::vector<std::string> &query, std::vector<SearchResultItemPtr> &results) const;
        std::mutex cache_lock;
        std::map<std::string, CacheEntryPtr> cache;
};

}
