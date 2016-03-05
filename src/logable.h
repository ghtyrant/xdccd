#pragma once

namespace xdccd
{
    template <typename T>
    class Logable
    {
        public:
            virtual std::string to_string() const = 0;

            friend std::ostream &operator<<(std::ostream &os, T const &o)
            {
                return os << o.to_string();
            }

            friend std::ostream &operator<<(std::ostream &os, std::shared_ptr<T> o)
            {
                return os << o->to_string();
            }

    };
}
