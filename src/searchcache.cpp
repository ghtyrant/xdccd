#include <boost/log/trivial.hpp>

#include "logging.h"
#include "searchcache.h"
#include "botmanager.h"

xdccd::SearchResult::SearchResult(std::size_t total_results, std::size_t result_start)
    : total_results(total_results), result_start(result_start)
{}

xdccd::CacheEntry::CacheEntry(const std::string &query, const std::vector<DCCAnnouncePtr> &announces)
    : query(query), announces(announces), created(std::chrono::system_clock::now())
{}

xdccd::SearchResultPtr xdccd::SearchCache::search(xdccd::BotManager &manager, const std::string &query, std::size_t start, std::size_t limit)
{
    std::lock_guard<std::mutex> lock(cache_lock);

    auto cache_it = cache.find(query);
    std::chrono::duration<double> age(0);

    if (cache_it != cache.end())
    {
        age = std::chrono::system_clock::now() - cache_it->second->created;

        // Remove cache entries older than MAX_CACHE_AGE
        if (age >= xdccd::search::MAX_CACHE_AGE)
        {
            BOOST_LOG_TRIVIAL(debug) << "Removing cached search result for '" << query << "' due to old age.";
            cache.erase(cache_it);
            cache_it = cache.end();
        }
    }

    SearchResultPtr sr = std::make_shared<SearchResult>(0, start);

    if (cache_it == cache.end())
    {
        std::vector<xdccd::DCCAnnouncePtr> tmp;

        for (auto bot : manager.get_bots())
            bot->find_announces(query, tmp);

        // Do not cache empty search results
        if (tmp.empty())
            return sr;

        BOOST_LOG_TRIVIAL(debug) << "Cached search result for '" << query << "' with " << tmp.size() << " results.";

        // Insert newly generated CacheEntry
        auto insert_result = cache.insert(std::make_pair(query, std::make_unique<CacheEntry>(query, tmp)));
        cache_it = insert_result.first;
    }

    sr->total_results = cache_it->second->announces.size();

    if (start >= sr->total_results)
        return sr;

    std::size_t end = std::min(sr->total_results - start, static_cast<std::size_t>(limit));
    sr->begin = cache_it->second->announces.begin() + start;
    sr->end = cache_it->second->announces.begin() + start + end;

    return sr;
}

void xdccd::SearchCache::clear()
{
    BOOST_LOG_TRIVIAL(debug) << "Clearing SearchCache!";
    cache.clear();
}
