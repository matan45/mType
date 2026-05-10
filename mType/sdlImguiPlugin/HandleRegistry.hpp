#pragma once
/*
 * Plugin-internal handle registry. The mType plugin C ABI only exposes
 * primitive scalars / strings / arrays / objects to scripts. Native pointers
 * (SDL_Window*, SDL_Renderer*, ImGuiContext*, etc.) are stored here and
 * referenced from .mt code by an int id.
 *
 * One registry per native type. Threadsafe via a global mutex (the plugin
 * runs on the VM thread anyway, but locking keeps this safe if ABI v2
 * reentrancy ever lets the VM yield between plugin frames).
 */

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace sdlimgui
{
    template <typename T>
    class HandleRegistry
    {
    public:
        /* Register `ptr`, returning a fresh non-zero id. id 0 is reserved
         * to mean "invalid handle" so .mt code can use 0 as a null sentinel. */
        int64_t insert(T* ptr)
        {
            std::lock_guard<std::mutex> lk(mtx_);
            int64_t id = ++nextId_;
            map_[id] = ptr;
            return id;
        }

        /* Look up by id. Returns nullptr if id is invalid or 0. */
        T* find(int64_t id) const
        {
            if (id == 0) return nullptr;
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = map_.find(id);
            return it == map_.end() ? nullptr : it->second;
        }

        /* Remove by id. Returns the previously stored pointer (or nullptr if
         * id was unknown). The caller is responsible for the type-specific
         * destructor (SDL_DestroyWindow, etc.). */
        T* erase(int64_t id)
        {
            if (id == 0) return nullptr;
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = map_.find(id);
            if (it == map_.end()) return nullptr;
            T* p = it->second;
            map_.erase(it);
            return p;
        }

        /* Snapshot of every (id, ptr) pair. Used by shutdown helpers that
         * want to release leaked handles when the plugin is unloaded. */
        std::vector<std::pair<int64_t, T*>> drain()
        {
            std::lock_guard<std::mutex> lk(mtx_);
            std::vector<std::pair<int64_t, T*>> out;
            out.reserve(map_.size());
            for (auto& [id, p] : map_) out.emplace_back(id, p);
            map_.clear();
            return out;
        }

    private:
        mutable std::mutex mtx_;
        int64_t nextId_ = 0;
        std::unordered_map<int64_t, T*> map_;
    };
}
