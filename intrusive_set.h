#pragma once

#include "intrusive_set_element.h"

#include <functional>
#include <iostream>

namespace implementation {

template <typename Tag, typename Key, typename Compare, typename Getter>
struct intrusive_set : Compare {
  using fake_t = set_fake_t<Tag>;
  using node_t = intrusive_set_element<Tag>;

  intrusive_set(fake_t* fake) : fake(fake) {}

  intrusive_set(Compare&& comparator, fake_t* fake)
      : Compare(std::move(comparator)), fake(fake) {}

  struct iterator;

  node_t* not_null(node_t* p) const {
    return p ? p : fake;
  }

  node_t* find(node_t* node, const Key& key) const {
    if (!node) {
      return nullptr;
    }
    if (Compare::operator()(key, Getter::get(node))) {
      return find(node->left, key);
    }
    if (Compare::operator()(Getter::get(node), key)) {
      return find(node->right, key);
    }
    return node;
  }
  node_t* find(const Key& key) const {
    return not_null(find(root(), key));
  }

  node_t* insert(node_t* node, node_t* inserted) {
    if (!node) {
      return inserted;
    }
    if (Compare::operator()(Getter::get(inserted), Getter::get(node))) {
      node->set_left(insert(node->left, inserted));
    } else {
      node->set_right(insert(node->right, inserted));
    }
    return node->balance();
  }
  void insert(node_t* node) {
    fake->set_left(insert(root(), node));
  }

  node_t* get_minimum() const {
    return not_null(node_t::find_minimum(root()));
  }

  void remove(node_t* node) {
    if (!node) {
      return;
    }
    auto left = node->left;
    auto right = node->right;

    node->set_children(nullptr, nullptr);

    auto subtree_root = left;
    if (right) {
      subtree_root = node_t::find_minimum(right);
      subtree_root->set_children(left, right->remove_minimum());
    }

    node->update_parent(subtree_root);
    up_balance(subtree_root);
  }

  void up_balance(node_t* node) {
    while (node) {
      if (node->left) {
        node->set_left(node->left->balance());
      }
      if (node->right) {
        node->set_right(node->right->balance());
      }
      node = node->parent;
    }
  }

  node_t* root() const noexcept {
    return fake->left;
  }

  iterator lower_bound(const Key& key) const {
    node_t* equal = find(key);
    if (equal != fake) {
      return iterator(equal);
    }
    return upper_bound(key);
  }
  iterator upper_bound(const Key& key) const {
    node_t* bound = fake;
    node_t* current = root();
    while (current) {
      if (Compare::operator()(key, Getter::get(current))) {
        bound = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }
    return iterator(bound);
  }

  struct iterator {
    node_t* m_node;

    explicit iterator(node_t* node) : m_node(node) {}

    iterator(const iterator& other) = default;
    iterator(iterator&& other) = default;

    iterator& operator=(iterator const& other) = default;
    iterator& operator=(iterator&& other) = default;

    Key const& operator*() const {
      return Getter::get(m_node);
    }

    Key const* operator->() const {
      return &Getter::get(m_node);
    }

    iterator& operator++() {
      if (m_node->right) {
        m_node = node_t::find_minimum(m_node->right);
        return *this;
      }

      while (m_node->parent && m_node == m_node->parent->right) {
        m_node = m_node->parent;
      }
      m_node = m_node->parent;

      return *this;
    }

    iterator operator++(int) {
      auto it = *this;
      ++this;
      return it;
    }

    iterator& operator--() {
      if (m_node->left) {
        m_node = node_t::find_maximum(m_node->left);
        return *this;
      }

      while (m_node->parent && m_node == m_node->parent->right) {
        m_node = m_node->parent;
      }
      m_node = m_node->parent;

      return *this;
    }

    iterator operator--(int) {
      auto it = *this;
      --this;
      return it;
    }

    friend bool operator==(iterator const& lhs, iterator const& rhs) {
      return lhs.m_node == rhs.m_node;
    }

    friend bool operator!=(iterator const& lhs, iterator const& rhs) {
      return !(lhs == rhs);
    }

    bool is_end() const {
      return m_node->parent == nullptr;
    }
  };

  iterator end() const {
    return iterator(fake);
  }
  iterator begin() const {
    return iterator(not_null(get_minimum()));
  }

  void swap(intrusive_set& other) {
    std::swap(fake->left, other.fake->left);
    fake->set_left(root());
    other.fake->set_left(other.root());
  }

private:
  fake_t* fake;
};

} // namespace implementation
