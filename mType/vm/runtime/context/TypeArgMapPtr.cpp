#include "TypeArgMapPtr.hpp"

namespace vm::runtime::detail
{
    TypeArgMap* TypeArgMapPool::acquire()
    {
        if (!free_.empty())
        {
            auto* m = free_.back();
            free_.pop_back();
            m->clear();
            return m;
        }
        owned_.push_back(std::make_unique<TypeArgMap>());
        return owned_.back().get();
    }

    void TypeArgMapPool::release(TypeArgMap* m) noexcept
    {
        if (m)
        {
            free_.push_back(m);
        }
    }

    TypeArgMapPool& threadLocalTypeArgPool()
    {
        thread_local TypeArgMapPool p;
        return p;
    }
}
