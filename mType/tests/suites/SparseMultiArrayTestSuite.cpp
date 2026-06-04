#include "SparseMultiArrayTestSuite.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;

    namespace
    {
        std::string indicesToString(const std::vector<size_t>& idx)
        {
            std::string s = "[";
            for (size_t i = 0; i < idx.size(); ++i)
            {
                if (i > 0) s += ",";
                s += std::to_string(idx[i]);
            }
            s += "]";
            return s;
        }
    }

    void SparseMultiArrayTestSuite::setupTests()
    {
        addCallbackTest("FlatMultiArray cached sub-view does not keep root alive", "",
            [](services::ScriptAPI&)
        {
            std::weak_ptr<value::FlatMultiArray> weakRoot;

            {
                auto root = std::make_shared<value::FlatMultiArray>(
                    std::vector<size_t>{2, 2}, value::Value(static_cast<int64_t>(0)));
                weakRoot = root;

                auto view = root->getSubArray(0);
                auto cachedView = root->getSubArray(0);
                if (view.get() != cachedView.get())
                    throw std::runtime_error("expected live FlatMultiArray sub-view cache hit");

                root.reset();
                view.reset();
                cachedView.reset();
            }

            if (!weakRoot.expired())
                throw std::runtime_error("FlatMultiArray cached sub-view kept root alive");
        });

        addCallbackTest("SparseMultiArray cached sub-view does not keep root alive", "",
            [](services::ScriptAPI&)
        {
            std::weak_ptr<value::SparseMultiArray> weakRoot;

            {
                auto root = std::make_shared<value::SparseMultiArray>(
                    std::vector<size_t>{100, 100}, value::Value{});
                weakRoot = root;

                auto view = root->getSubArray(7);
                auto cachedView = root->getSubArray(7);
                if (view.get() != cachedView.get())
                    throw std::runtime_error("expected live SparseMultiArray sub-view cache hit");

                root.reset();
                view.reset();
                cachedView.reset();
            }

            if (!weakRoot.expired())
                throw std::runtime_error("SparseMultiArray cached sub-view kept root alive");
        });

        // ===== DENSE path =====
        // Small arrays (< MIN_SIZE_FOR_SPARSE = 1000 elements) start in
        // DENSE mode; this exercises the row-major linear scan.
        addCallbackTest("forEachNonDefault DENSE path emits only non-default entries", "",
            [](services::ScriptAPI&)
        {
            value::SparseMultiArray arr({2, 3}, value::Value{}); // VOID default
            arr.set({0, 1}, value::Value(static_cast<int64_t>(5)));
            arr.set({1, 2}, value::Value(static_cast<int64_t>(7)));

            std::vector<std::pair<std::vector<size_t>, int64_t>> hits;
            arr.forEachNonDefault([&](const std::vector<size_t>& idx, const value::Value& v) {
                hits.emplace_back(idx, value::asInt(v));
            });

            if (hits.size() != 2)
                throw std::runtime_error("expected 2 callbacks, got " + std::to_string(hits.size()));

            bool sawFirst = false, sawSecond = false;
            for (const auto& [idx, val] : hits)
            {
                if (idx == std::vector<size_t>{0, 1} && val == 5) sawFirst = true;
                if (idx == std::vector<size_t>{1, 2} && val == 7) sawSecond = true;
            }
            if (!sawFirst)
                throw std::runtime_error("expected callback ({0,1}, 5)");
            if (!sawSecond)
                throw std::runtime_error("expected callback ({1,2}, 7)");
        });

        // ===== SPARSE root path =====
        // Arrays >= MIN_SIZE_FOR_SPARSE elements start in SPARSE mode and
        // never adapt away unless many writes occur. 100x100 = 10000 ≥ 1000.
        addCallbackTest("forEachNonDefault SPARSE root path emits hash-bucket entries", "",
            [](services::ScriptAPI&)
        {
            value::SparseMultiArray arr({100, 100}, value::Value{});
            if (arr.getStorageMode() != value::SparseMultiArray::StorageMode::SPARSE)
                throw std::runtime_error("test precondition: expected initial SPARSE mode for 100x100");

            arr.set({10, 20}, value::Value(static_cast<int64_t>(5)));
            arr.set({50, 75}, value::Value(static_cast<int64_t>(7)));
            arr.set({99, 99}, value::Value(static_cast<int64_t>(9)));

            std::vector<std::pair<std::vector<size_t>, int64_t>> hits;
            arr.forEachNonDefault([&](const std::vector<size_t>& idx, const value::Value& v) {
                hits.emplace_back(idx, value::asInt(v));
            });

            if (hits.size() != 3)
                throw std::runtime_error("expected 3 callbacks, got " + std::to_string(hits.size()));

            // Order is unspecified (hash-bucket order); check by membership.
            auto containsHit = [&](const std::vector<size_t>& expectedIdx, int64_t expectedVal) {
                for (const auto& [idx, val] : hits)
                {
                    if (idx == expectedIdx && val == expectedVal) return true;
                }
                return false;
            };
            if (!containsHit({10, 20}, 5))
                throw std::runtime_error("missing callback ({10,20}, 5)");
            if (!containsHit({50, 75}, 7))
                throw std::runtime_error("missing callback ({50,75}, 7)");
            if (!containsHit({99, 99}, 9))
                throw std::runtime_error("missing callback ({99,99}, 9)");
        });

        // ===== SPARSE view path =====
        // getSubArray(i) returns a view; forEachNonDefault on the view must
        // filter to that window of the parent's storage and report local
        // (view-relative) indices.
        addCallbackTest("forEachNonDefault SPARSE view filters by offset and emits local indices", "",
            [](services::ScriptAPI&)
        {
            // Use shared_ptr-backed root so getSubArray's view setup
            // (which calls shared_from_this internally) is honoured.
            auto root = std::make_shared<value::SparseMultiArray>(
                std::vector<size_t>{100, 100}, value::Value{});
            if (root->getStorageMode() != value::SparseMultiArray::StorageMode::SPARSE)
                throw std::runtime_error("test precondition: expected initial SPARSE mode for 100x100");

            // Three rows touched; the view of row 7 must see only the row-7 write.
            root->set({3, 4}, value::Value(static_cast<int64_t>(11)));
            root->set({7, 50}, value::Value(static_cast<int64_t>(22)));
            root->set({9, 0}, value::Value(static_cast<int64_t>(33)));

            auto rowView = root->getSubArray(7); // 1D view of length 100

            std::vector<std::pair<std::vector<size_t>, int64_t>> hits;
            rowView->forEachNonDefault([&](const std::vector<size_t>& idx, const value::Value& v) {
                hits.emplace_back(idx, value::asInt(v));
            });

            if (hits.size() != 1)
                throw std::runtime_error(
                    "expected exactly 1 callback inside row-7 view, got "
                    + std::to_string(hits.size()));

            // Local index: row-view of row 7 collapses dimension 0; the
            // remaining coordinate is column 50.
            const auto& [idx, val] = hits[0];
            if (idx != std::vector<size_t>{50})
                throw std::runtime_error(
                    "view callback received indices " + indicesToString(idx)
                    + " — expected [50] (column in view-local coordinates)");
            if (val != 22)
                throw std::runtime_error(
                    "view callback received value " + std::to_string(val)
                    + " — expected 22");
        });

        // ===== Sparsity-stats sanity check (indirect coverage of decodeLocalLinearIndex) =====
        // decodeLocalLinearIndex is private; we exercise it transitively
        // through the DENSE + SPARSE paths above. This test verifies that
        // multi-rank index decoding is row-major for a 2x3x4 shape, which
        // is where row-major bugs (e.g. wrong stride order) would surface.
        addCallbackTest("forEachNonDefault 2x3x4 array decodes row-major indices", "",
            [](services::ScriptAPI&)
        {
            value::SparseMultiArray arr({2, 3, 4}, value::Value{});
            arr.set({0, 0, 0}, value::Value(static_cast<int64_t>(1)));
            arr.set({1, 2, 3}, value::Value(static_cast<int64_t>(2)));
            arr.set({0, 1, 2}, value::Value(static_cast<int64_t>(3)));

            std::vector<std::pair<std::vector<size_t>, int64_t>> hits;
            arr.forEachNonDefault([&](const std::vector<size_t>& idx, const value::Value& v) {
                hits.emplace_back(idx, value::asInt(v));
            });

            if (hits.size() != 3)
                throw std::runtime_error("expected 3 callbacks, got " + std::to_string(hits.size()));

            auto containsHit = [&](const std::vector<size_t>& expectedIdx, int64_t expectedVal) {
                for (const auto& [idx, val] : hits)
                {
                    if (idx == expectedIdx && val == expectedVal) return true;
                }
                return false;
            };
            if (!containsHit({0, 0, 0}, 1))
                throw std::runtime_error("missing callback ({0,0,0}, 1)");
            if (!containsHit({1, 2, 3}, 2))
                throw std::runtime_error("missing callback ({1,2,3}, 2) — row-major decode broken?");
            if (!containsHit({0, 1, 2}, 3))
                throw std::runtime_error("missing callback ({0,1,2}, 3)");
        });
    }
}
