////////////////////////////////////////////////////////////////////////////////
//
// lockfree/spsc_queue.hpp
//
////////////////////////////////////////////////////////////////////////////////

#ifndef LOCKFREE_INCLUDED_90B692D8_37D0_413D_AD7B_D839780C74F6
#define LOCKFREE_INCLUDED_90B692D8_37D0_413D_AD7B_D839780C74F6


#include <atomic>
#include <cassert>
#include <memory>
#include <utility>

#include "compressed_pair.hpp"


namespace lockfree {

template<typename T, typename Alloc = std::allocator<T>>
class spsc_queue
{
private:
    struct node_type
    {
        // Must construct and destruct via allocator methods only.
        node_type() = delete;
        ~node_type() = delete;

        T data;
        std::atomic<node_type*> next;
    };

    using alloc_traits = std::allocator_traits<Alloc>;
    using node_allocator =
        typename alloc_traits::template rebind_alloc<node_type>;
    using node_alloc_traits =
        typename alloc_traits::template rebind_traits<node_type>;

    // Back of the queue; only accessed by the producer thread.
    node_type* tail_;

    // Sentinel before the front of the queue; only written to by consumer
    // thread, but is read by both threads.
    std::atomic<node_type*> before_head_;

    // Cached copy of before_head that is only accessed by the producer
    // thread. Whenever this node is equal to cache_head, it is synced with
    // before_head to avoid unnecessary memory fences.
    node_type* cache_tail_;

    // Pair containing the front of the node cache and the node allocator.
    // XXX: use [[no_unique_address]] for the allocator in C++20.
    detail::compressed_pair<node_type*, node_allocator> cache_head_and_alloc_;

    explicit spsc_queue(node_allocator a) :
        tail_(node_alloc_traits::allocate(a, 1)),
        before_head_(tail_),
        cache_tail_(tail_),
        cache_head_and_alloc_(tail_, std::move(a))
    {
        node_alloc_traits::construct(alloc_(), std::addressof(tail_->next),
                                     nullptr);
    }

public:
    using allocator_type = Alloc;
    using value_type     = T;
    using pointer        = typename alloc_traits::pointer;
    using const_pointer  = typename alloc_traits::const_pointer;
    using size_type      = typename alloc_traits::size_type;

    spsc_queue(spsc_queue const&) = delete;
    spsc_queue& operator=(spsc_queue const&) = delete;

    explicit spsc_queue(allocator_type const& a = allocator_type()) :
        spsc_queue(node_allocator(a))
    {}

    ~spsc_queue()
    {
        auto first = cache_head_();

        // Deallocate cached nodes.
        for (auto const cache_end = head_(); first != cache_end; ) {
            auto const x = spsc_queue::advance_(first);
            node_alloc_traits::deallocate(alloc_(), x, 1);
        }

        // Destruct and deallocate enqueued nodes.
        while (first != nullptr) {
            auto const x = spsc_queue::advance_(first);
            node_alloc_traits::destroy(alloc_(), std::addressof(x->data));
            node_alloc_traits::deallocate(alloc_(), x, 1);
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Producer functions.
    ////////////////////////////////////////////////////////////////////////////

    template<typename... Args>
    void emplace(Args&&... args)
    {
        auto const x = make_node_(std::forward<Args>(args)...);
        tail_->next.store(x, std::memory_order_release);
        tail_ = x;
    }

    void push(value_type&& v)
    {
        emplace(std::move(v));
    }

    void push(value_type const& v)
    {
        emplace(v);
    }

    template<typename InIt>
    void push(InIt first, InIt const last)
    {
        if (first == last) {
            return;
        }

        auto insert_head = make_node_(*first);
        auto insert_tail = insert_head;

        try {
            while (++first != last) {
                auto const x = make_node_(*first);
                insert_tail->next.store(x, std::memory_order_relaxed);
                insert_tail = x;
            }
        }
        catch (...) {
            while (insert_head != nullptr) {
                auto const x = spsc_queue::advance_(insert_head);
                node_alloc_traits::destroy(alloc_(), std::addressof(x->data));
                node_alloc_traits::deallocate(alloc_(), x, 1);
            }
            throw;
        }

        tail_->next.store(insert_head, std::memory_order_release);
        tail_ = insert_tail;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Consumer functions.
    ////////////////////////////////////////////////////////////////////////////

    void clear() noexcept
    {
        auto last = before_head_.load(std::memory_order_relaxed);
        while (auto const x = last->next.load(std::memory_order_consume)) {
            node_alloc_traits::destroy(alloc_(), std::addressof(x->data));
            last = x;
        }
        before_head_.store(last, std::memory_order_release);
    }

    template<typename U>
    bool pop(U&& out)
    noexcept(std::is_nothrow_assignable_v<U, T&&>)
    {
        if (auto const x = head_()) {
            out = std::move(x->data);
            node_alloc_traits::destroy(alloc_(), std::addressof(x->data));
            before_head_.store(x, std::memory_order_release);
            return true;
        }
        return false;
    }

    pointer front() noexcept
    {
        auto const x = head_();
        return x ? std::addressof(x->data) : nullptr;
    }

    const_pointer front() const noexcept
    {
        auto const x = head_();
        return x ? std::addressof(x->data) : nullptr;
    }

    bool empty() const noexcept
    {
        return (head_() == nullptr);
    }

    template<typename F>
    void consume_all(F&& f)
    {
        for (auto x = head_(); x != nullptr; ) {
            std::invoke(f, x->data);
            node_alloc_traits::destroy(alloc_(), std::addressof(x->data));
            before_head_.store(x, std::memory_order_release);
            x = x->next.load(std::memory_order_consume);
        }
    }

    bool is_lock_free() const noexcept
    { return before_head_.is_lock_free(); }

    allocator_type get_allocator() const
    { return allocator_type(alloc_()); }

private:
    static node_type* advance_(node_type*& x) noexcept
    { return std::exchange(x, x->next.load(std::memory_order_relaxed)); }

    node_type* cache_head_() const& noexcept
    { return cache_head_and_alloc_.first(); }

    node_type*& cache_head_() & noexcept
    { return cache_head_and_alloc_.first(); }

    node_allocator& alloc_() & noexcept
    { return cache_head_and_alloc_.second(); }

    node_allocator const& alloc_() const& noexcept
    { return cache_head_and_alloc_.second(); }

    node_type* head_() const noexcept
    {
        auto const x = before_head_.load(std::memory_order_relaxed);
        assert(x && "before_head can never be null");
        return x->next.load(std::memory_order_consume);
    }

    template<typename... Args>
    node_type* make_node_(Args&&... args)
    {
        auto x = cache_head_();
        if (cache_tail_ == x) {
            cache_tail_ = before_head_.load(std::memory_order_consume);
        }
        if (cache_tail_ != x) {
            // Recycle a cached node.
            node_alloc_traits::construct(alloc_(), std::addressof(x->data),
                                         std::forward<Args>(args)...);
            // Remove our node from the cache.
            spsc_queue::advance_(cache_head_());
        }
        else {
            // Node cache is empty, so allocate and construct a new node.
            x = node_alloc_traits::allocate(alloc_(), 1);
            try {
                node_alloc_traits::construct(alloc_(), std::addressof(x->data),
                                             std::forward<Args>(args)...);
            }
            catch (...) {
                node_alloc_traits::deallocate(alloc_(), x, 1);
                throw;
            }
        }
        node_alloc_traits::construct(alloc_(), std::addressof(x->next),
                                     nullptr);
        return x;
    }
};

}       // namespace lockfree

#endif  // LOCKFREE_INCLUDED_90B692D8_37D0_413D_AD7B_D839780C74F6
