#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include <cctype>

#include "logging.h"
#include "searchmanager.h"
#include "botmanager.h"

xdccd::SearchResultItem::SearchResultItem(DCCAnnouncePtr announce, unsigned int score)
    : announce(announce), score(score)
{}

xdccd::SearchResult::SearchResult(std::size_t total_results, std::size_t result_start)
    : total_results(total_results), result_start(result_start)
{}

xdccd::CacheEntry::CacheEntry(const std::string &query, std::vector<SearchResultItemPtr> results)
    : query(query), results(std::move(results)), created(std::chrono::system_clock::now())
{}

xdccd::SearchResultPtr xdccd::SearchManager::search(xdccd::BotManager &manager, const std::string &query, std::size_t start, std::size_t limit)
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
        std::vector<xdccd::SearchResultItemPtr> tmp;

        // Split the query up
        std::vector<std::string> split_query;
        boost::algorithm::split(split_query, query, boost::algorithm::is_space(), boost::algorithm::token_compress_on );

        for (auto bot : manager.get_bots())
            search_in_announces(bot->get_announces(), split_query, tmp);

        // Do not cache empty search results
        if (tmp.empty())
            return sr;

        std::sort(tmp.begin(), tmp.end(), [](SearchResultItemPtr& x, SearchResultItemPtr& y) { return x->score > y->score; });

        BOOST_LOG_TRIVIAL(debug) << "Cached search result for '" << query << "' with " << tmp.size() << " results.";

        // Insert newly generated CacheEntry
        auto insert_result = cache.insert(std::make_pair(query, std::make_unique<CacheEntry>(query, std::move(tmp))));
        cache_it = insert_result.first;
    }

    sr->total_results = cache_it->second->results.size();

    if (start >= sr->total_results)
        return sr;

    std::size_t end = std::min(sr->total_results - start, static_cast<std::size_t>(limit));
    sr->begin = cache_it->second->results.begin() + start;
    sr->end = cache_it->second->results.begin() + start + end;

    return sr;
}

void xdccd::SearchManager::search_in_announces(const std::map<std::string, xdccd::DCCAnnouncePtr> &announces, const std::vector<std::string> &query, std::vector<xdccd::SearchResultItemPtr> &results) const
{
    // Iterate all announces
    for (auto announce : announces)
    {
        // Split & iterate the filename by '.'
        auto start_it = boost::algorithm::make_split_iterator(announce.second->filename, boost::algorithm::first_finder(".", boost::algorithm::is_equal()));
        auto end = boost::split_iterator<std::string::iterator>();

        unsigned int score = 0;
        // Check if our query contains the current filename part
        for (auto qit = query.begin(); qit != query.end(); ++qit)
        {
            for(auto it = start_it; it != end; ++it)
            {
                if (boost::algorithm::icontains(*it, *qit))
                {
                    score += 1 + ((*qit).size() == (*it).size());
                    break;
                }
            }
        }

        // Do not add results with score == 0
        if (score >= query.size())
            results.push_back(std::make_unique<SearchResultItem>(announce.second, score));
    }
}

void xdccd::SearchManager::clear()
{
    BOOST_LOG_TRIVIAL(debug) << "Clearing SearchManager!";
    cache.clear();
}
