#pragma once

#include "intrusive_set.h"

#include <cstddef>
#include <stdexcept>

template <typename Left, typename Right, typename CompareLeft = std::less<Left>,
          typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;

  struct node_t;

private:
  struct left_tag;
  struct right_tag;

  using left_fake_t = implementation::set_fake_t<left_tag>;
  using right_fake_t = implementation::set_fake_t<right_tag>;

  struct fake_t : left_fake_t, right_fake_t {};

  using left_set_element_t = implementation::intrusive_set_element<left_tag>;
  using right_set_element_t = implementation::intrusive_set_element<right_tag>;

  template <typename T, typename K>
  struct super : implementation::intrusive_set_element<T> {
    explicit super(const K& key) : m_key(key) {}

    explicit super(K&& key) : m_key(std::move(key)) {}

    const K& key() const {
      return m_key;
    }

  private:
    K m_key;
  };

  using left_super = super<left_tag, left_t>;
  using right_super = super<right_tag, right_t>;

  template <typename T, typename U, typename K>
  struct key_getter {
    static const K& get(const T* e) {
      return static_cast<const U*>(e)->key();
    }
  };

  using left_getter = key_getter<left_set_element_t, left_super, left_t>;
  using right_getter = key_getter<right_set_element_t, right_super, right_t>;

  using left_set_t =
      implementation::intrusive_set<left_tag, left_t, CompareLeft, left_getter>;
  using right_set_t = implementation::intrusive_set<right_tag, right_t,
                                                    CompareRight, right_getter>;

  using left_set_it_t = typename left_set_t::iterator;
  using right_set_it_t = typename right_set_t::iterator;

public:
  struct node_t : super<left_tag, left_t>, super<right_tag, right_t> {
    template <typename L, typename R>
    node_t(L&& l, R&& r)
        : left_super(std::forward<L>(l)), right_super(std::forward<R>(r)) {}
  };

private:
  template <bool is_left>
  struct base_iterator {
    using paired_t = base_iterator<!is_left>;

    template <bool B>
    using set_it_t = std::conditional_t<B, left_set_it_t, right_set_it_t>;

    using this_set_it_t = set_it_t<is_left>;
    using that_set_it_t = set_it_t<!is_left>;

    this_set_it_t it;

    base_iterator(this_set_it_t it) : it(it){};

    node_t* get_node() const {
      return static_cast<node_t*>(it.m_node);
    }

    const auto& operator*() const {
      return *it;
    }
    auto const* operator->() const {
      return &(*it);
    }

    base_iterator& operator++() {
      ++it;
      return *this;
    }
    base_iterator operator++(int) {
      auto copy = *this;
      ++it;
      return copy;
    }

    base_iterator& operator--() {
      --it;
      return *this;
    }
    base_iterator operator--(int) {
      auto copy = *this;
      --it;
      return copy;
    }

    friend bool operator==(base_iterator const& lhs, base_iterator const& rhs) {
      return lhs.it == rhs.it;
    }

    friend bool operator!=(base_iterator const& lhs, base_iterator const& rhs) {
      return !(lhs == rhs);
    }

    paired_t flip() const {
      if (!it.is_end()) {
        return paired_t(that_set_it_t(bimap::flip(it.m_node)));
      }
      auto fake_node = static_cast<fake_t*>(it.m_node);
      return paired_t(that_set_it_t(fake_node));
    }
  };

public:
  using left_iterator = base_iterator<true>;
  using right_iterator = base_iterator<false>;

  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : left_set(std::move(compare_left), &fake),
        right_set(std::move(compare_right), &fake) {}

  bimap(bimap const& other) : left_set(&fake), right_set(&fake) {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  }

  bimap(bimap&& other) noexcept : left_set(&fake), right_set(&fake) {
    swap(other);
  }

  bimap& operator=(bimap const& other) {
    if (this == &other) {
      return *this;
    }
    bimap(other).swap(*this);
    return *this;
  }
  bimap& operator=(bimap&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    swap(other);
    return *this;
  }

  void clear() {
    erase_left(begin_left(), end_left());
  }

  void swap(bimap& other) {
    std::swap(m_size, other.m_size);
    left_set.swap(other.left_set);
    right_set.swap(other.right_set);
  };

  ~bimap() {
    clear();
  }

  left_iterator insert(left_t const& left, right_t const& right) {
    return template_insert(left, right);
  }
  left_iterator insert(left_t const& left, right_t&& right) {
    return template_insert(left, std::move(right));
  }
  left_iterator insert(left_t&& left, right_t const& right) {
    return template_insert(std::move(left), right);
  }
  left_iterator insert(left_t&& left, right_t&& right) {
    return template_insert(std::move(left), std::move(right));
  }

private:
  template <typename L, typename R>
  left_iterator template_insert(L&& left, R&& right) {
    if (find_left(left) != end_left() || find_right(right) != end_right()) {
      return end_left();
    }
    auto node = new node_t(std::forward<L>(left), std::forward<R>(right));
    right_set.insert(node);
    left_set.insert(node);
    ++m_size;
    return find_left(std::forward<L>(left));
  }

private:
  void erase(left_iterator left, right_iterator right) {
    left_set.remove(left.get_node());
    right_set.remove(right.get_node());

    delete left.get_node();

    --m_size;

  }

public:
  left_iterator erase_left(left_iterator iterator) {
    auto it = iterator++;
    erase(it, it.flip());
    return iterator;
  }
  bool erase_left(left_t const& key) {
    auto it = find_left(key);
    if (it == end_left()) {
      return false;
    }
    erase_left(it);
    return true;
  }

  right_iterator erase_right(right_iterator iterator) {
    auto it = iterator++;
    erase(it.flip(), it);
    return iterator;
  }
  bool erase_right(right_t const& key) {
    auto it = find_right(key);
    if (it == end_right()) {
      return false;
    }
    erase_right(it);
    return true;
  }

  left_iterator erase_left(left_iterator first, left_iterator last) {
    auto it = first;
    while (it != last) {
      erase_left(it++);
    }
    return last;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    auto it = first;
    while (it != last) {
      erase_right(it++);
    }
    return last;
  }

  left_iterator find_left(left_t const& left) const {
    return left_iterator(typename left_set_t::iterator(left_set.find(left)));
  }
  right_iterator find_right(right_t const& right) const {
    return right_iterator(
        typename right_set_t::iterator(right_set.find(right)));
  }

  right_t const& at_left(left_t const& key) const {
    auto e = left_set.find(key);
    if (e == &fake) {
      throw std::out_of_range("bimap don't contains this key");
    }
    return right_getter::get(flip(e));
  }
  left_t const& at_right(right_t const& key) const {
    auto e = right_set.find(key);
    if (e == &fake) {
      throw std::out_of_range("bimap don't contains this key");
    }
    return left_getter::get(flip(e));
  }

  template <
      typename = std::enable_if<std::is_default_constructible_v<right_t>, void>>
  right_t const& at_left_or_default(left_t const& key) {
    left_iterator it = find_left(key);
    if (it != end_left()) {
      return *it.flip();
    }
    right_t default_v = right_t();
    erase_right(default_v);
    return *insert(key, std::move(default_v)).flip();
  }

  template <
      typename = std::enable_if<std::is_default_constructible_v<left_t>, void>>
  left_t const& at_right_or_default(right_t const& key) {
    right_iterator it = find_right(key);
    if (it != end_right()) {
      return *it.flip();
    }
    left_t default_v = left_t();
    erase_left(default_v);
    return *insert(std::move(default_v), key);
  }

  left_iterator lower_bound_left(const left_t& left) const {
    return left_iterator(left_set.lower_bound(left));
  }
  left_iterator upper_bound_left(const left_t& left) const {
    return left_iterator(left_set.upper_bound(left));
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_iterator(right_set.lower_bound(right));
  }
  right_iterator upper_bound_right(const right_t& right) const {
    return right_iterator(right_set.upper_bound(right));
  }

  left_iterator begin_left() const {
    return left_iterator(left_set.begin());
  }
  left_iterator end_left() const {
    return left_iterator(left_set.end());
  }

  right_iterator begin_right() const {
    return right_iterator(right_set.begin());
  }
  right_iterator end_right() const {
    return right_iterator(right_set.end());
  }

  bool empty() const {
    return m_size == 0;
  }

  std::size_t size() const {
    return m_size;
  }

private:
  bool left_equals(const left_t& lhs, const left_t& rhs) const {
    return !left_set(lhs, rhs) && !left_set(rhs, lhs);
  }
  bool right_equals(const right_t& lhs, const right_t& rhs) const {
    return !right_set(lhs, rhs) && !right_set(rhs, lhs);
  }

public:
  friend bool operator==(bimap const& lhs, bimap const& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    auto lit = lhs.begin_left();
    auto rit = rhs.begin_left();
    while (lit != lhs.end_left()) {
      if (!lhs.left_equals(*lit, *rit) ||
          !lhs.right_equals(*(lit++).flip(), *(rit++).flip())) {
        return false;
      }
    }
    return true;
  }
  friend bool operator!=(bimap const& lhs, bimap const& rhs) {
    return !(lhs == rhs);
  }

private:
  template <typename TSetElement, typename USetElement>
  static USetElement* template_flip(TSetElement* element) {
    if (!element) {
      return nullptr;
    }
    auto node = static_cast<node_t*>(element);
    return static_cast<USetElement*>(node);
  }
  static left_set_element_t* flip(right_set_element_t* e) {
    return template_flip<right_set_element_t, left_set_element_t>(e);
  }
  static right_set_element_t* flip(left_set_element_t* e) {
    return template_flip<left_set_element_t, right_set_element_t>(e);
  }

private:
  std::size_t m_size = 0;

  fake_t fake;

  left_set_t left_set;
  right_set_t right_set;
};
