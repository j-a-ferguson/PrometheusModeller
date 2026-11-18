#include "slot_pool.hpp"

#include "schema.hpp"

#include <stdexcept>

namespace prometheus::topology {

template <typename T>
Handle<T>::Handle(SlotPool<T> &pool, Index index, Generation gen)
    : pool_{&pool}, index_{index}, gen_{gen} {}

template <typename T> auto Handle<T>::get() -> T & {
    if (!pool_)
        throw std::logic_error("Handle is null");
    auto opt_val = pool_->get(*this);
    if (!opt_val) {
        throw std::logic_error("Handle is stale");
    }
    return opt_val->get();
}

template <typename T> auto Handle<T>::get() const -> const T & {
    if (!pool_)
        throw std::logic_error("Handle is null");
    auto opt_val = pool_->get(*this);
    if (!opt_val) {
        throw std::logic_error("Handle is stale");
    }
    return opt_val->get();
}

template <typename T> auto Handle<T>::index() const -> const Index & { return index_; }

template <typename T> auto Handle<T>::gen() const -> const Generation & { return gen_; }

template <typename T> void Handle<T>::nullify() {
    pool_ = nullptr;
    index_ = NULL_INDEX;
    gen_ = NULL_GEN;
}

template <typename T> bool Handle<T>::is_null() const {
    bool pool_null = pool_ == nullptr;
    bool index_null = index_ == NULL_INDEX;
    bool gen_null = gen_ == NULL_GEN;
    bool all_null = pool_null && index_null && gen_null;
    if (all_null)
        return true;
    bool all_not_null = !pool_null && !index_null && !gen_null;
    if (all_not_null)
        return false;
    throw std::logic_error("Corrupted internal state");
}

template <typename T> auto SlotPool<T>::Slot::is_live() const -> bool { return gen & 1; }

template <typename T> SlotPool<T>::Slot::Slot() : next{no_slot} {}

template <typename T>
SlotPool<T>::Slot::Slot(Slot &&o) noexcept(std::is_nothrow_move_constructible_v<T>) : gen(o.gen) {
    if (is_live())
        std::construct_at(&value, std::move(o.value));
    else
        next = o.next;
}

template <typename T> SlotPool<T>::Slot::~Slot() {
    if (is_live())
        std::destroy_at(&value);
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::skip_free() -> void {
    while (cur_ != end_ && !cur_->is_live())
        ++cur_;
}

template <typename T>
template <bool Const>
SlotPool<T>::Iter<Const>::Iter(SlotPoolPtr pool, SlotPtr start, SlotPtr cur, SlotPtr end)
    : pool_{pool}, start_{start}, cur_{cur}, end_{end} {
    skip_free();
}

template <typename T>
template <bool Const>
SlotPool<T>::Iter<Const>::operator Iter<true>() const
    requires(!Const)
{
    return {pool_, start_, cur_, end_};
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator*() const -> reference {
    return cur_->value;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator->() const -> pointer {
    return &cur_->value;
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::operator++() -> Iter & {
    ++cur_;
    skip_free();
    return *this;
}

template <typename T> template <bool Const> auto SlotPool<T>::Iter<Const>::operator++(int) -> Iter {
    Iter out = *this;
    ++cur_;
    skip_free();
    return out;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::operator==(const Iter &other) const -> bool {
    return cur_ == other.cur_;
}

template <typename T>
template <bool Const>
auto SlotPool<T>::Iter<Const>::handle() const -> Handle<T>
    requires(!Const)
{
    return Handle<T>{*pool_, static_cast<Index>(cur_ - start_), cur_->gen};
}

template <typename T> auto SlotPool<T>::find(Handle<T> h) -> Slot * {
    require_same_pool(h);
    if (h.index_ >= slots_.size())
        return nullptr;
    Slot &slot = slots_[h.index_];
    return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
}

template <typename T> auto SlotPool<T>::find(Handle<T> h) const -> const Slot * {
    require_same_pool(h);
    if (h.index_ >= slots_.size())
        return nullptr;
    const Slot &slot = slots_[h.index_];
    return h.gen_ == slot.gen && slot.is_live() ? &slot : nullptr;
}

template <typename T> auto SlotPool<T>::acquire_index() -> Index {
    if (head_ == no_slot) {
        // free list empty, add new element
        slots_.emplace_back();
        return slots_.size() - 1;
    }
    Index idx = head_;
    head_ = slots_[idx].next;
    return idx;
}

template <typename T> auto SlotPool<T>::release_index(Index i) -> void {
    slots_[i].next = head_;
    head_ = i;
}

template <typename T> auto SlotPool<T>::with_capacity(std::size_t capacity) -> SlotPool<T> {
    SlotPool<T> out;
    out.slots_.reserve(capacity);
    return out;
}

template <typename T> auto SlotPool<T>::begin() -> iterator {
    return iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
}

template <typename T> auto SlotPool<T>::end() -> iterator {
    return iterator{this, slots_.data(), slots_.data() + slots_.size(),
                    slots_.data() + slots_.size()};
}

template <typename T> auto SlotPool<T>::cbegin() const -> const_iterator {
    return const_iterator(this, slots_.data(), slots_.data(), slots_.data() + slots_.size());
}

template <typename T> auto SlotPool<T>::cend() const -> const_iterator {
    return const_iterator{this, slots_.data(), slots_.data() + slots_.size(),
                          slots_.data() + slots_.size()};
}

template <typename T>
auto SlotPool<T>::emplace_with(const std::function<void(T *)> &construct) -> Handle<T> {
    const auto idx = acquire_index();
    Slot &s = slots_[idx];
    try {
        construct(&s.value);
    } catch (...) {
        release_index(idx);
        throw;
    }
    ++s.gen;
    return Handle<T>{*this, idx, s.gen};
}

template <typename T> auto SlotPool<T>::erase(Handle<T> handle) -> bool {
    Slot *s = find(handle);
    if (s == nullptr)
        return false;

    std::destroy_at(&(s->value));
    ++s->gen;
    release_index(handle.index_);
    return true;
}

template <typename T> auto SlotPool<T>::get(Handle<T> handle) -> OptRef<T> {
    Slot *slot = find(handle);
    if (slot == nullptr)
        return std::nullopt;
    return std::ref(slot->value);
}

template <typename T> auto SlotPool<T>::get(Handle<T> handle) const -> ConstOptRef<T> {
    const Slot *slot = find(handle);
    if (slot == nullptr)
        return std::nullopt;
    return std::cref(slot->value);
}

template <typename T> auto SlotPool<T>::is_stale(Handle<T> handle) const -> bool {
    const Slot *slot = find(handle);
    return slot == nullptr;
}

template <typename T> auto SlotPool<T>::require_same_pool(Handle<T> handle) const -> void {
    if (handle.pool_ != this) {
        throw std::logic_error("Handle from wrong slot pool");
    }
}

#define INSTANTIATE_SLOT_POOL(Type)                                                                \
    template class Handle<Type>;                                                                   \
    template class SlotPool<Type>;                                                                 \
    template class SlotPool<Type>::Iter<false>;                                                    \
    template class SlotPool<Type>::Iter<true>

INSTANTIATE_SLOT_POOL(Vertex);
INSTANTIATE_SLOT_POOL(Edge);
INSTANTIATE_SLOT_POOL(Fin);
INSTANTIATE_SLOT_POOL(Loop);
INSTANTIATE_SLOT_POOL(Face);
INSTANTIATE_SLOT_POOL(Shell);
INSTANTIATE_SLOT_POOL(Region);
INSTANTIATE_SLOT_POOL(Body);
INSTANTIATE_SLOT_POOL(Vec3);
INSTANTIATE_SLOT_POOL(PointNode);
INSTANTIATE_SLOT_POOL(CurveNode);
INSTANTIATE_SLOT_POOL(SurfaceNode);
INSTANTIATE_SLOT_POOL(int);

#undef INSTANTIATE_SLOT_POOL

} // namespace prometheus::topology
