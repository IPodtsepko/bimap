#pragma once

#include <cassert>

namespace implementation {

template <typename Tag, typename Key, typename Compare, typename Getter>
struct intrusive_set;

template <typename Tag>
class intrusive_set_element {
  using self = intrusive_set_element;

  template <typename, typename, typename Compare, typename Getter>
  friend struct intrusive_set;

  void set_left(self* child) noexcept {
    this->left = child;
    set_parent(child, this);
  }
  void set_right(self* child) noexcept {
    this->right = child;
    set_parent(child, this);
  }
  void set_children(self* left_child, self* right_child) {
    set_left(left_child);
    set_right(right_child);
  }
  void update_parent(self * replacement) const {
    if (is_left()) {
      parent->set_left(replacement);
    } else {
      parent->set_right(replacement);
    }
  }

  bool is_left() const noexcept {
    return parent->left == this;
  }

  int difference() const {
    return get_signed_height(right) - get_signed_height(left);
  }

  self* rotate_right() {
    auto previous_left = left;

    set_left(previous_left->right);
    previous_left->set_right(this);

    update(this);
    update(previous_left);

    return previous_left;
  }

  self* rotate_left() {
    auto previous_right = right;

    set_right(previous_right->left);
    previous_right->set_left(this);

    update(this);
    update(previous_right);

    return previous_right;
  }

  self* balance() {
    update(this);
    if (difference() == 2) {
      if (right->difference() < 0) {
        set_right(right->rotate_right());
      }
      return rotate_left();
    }
    if (difference() == -2) {
      if (left->difference() > 0) {
        set_left(left->rotate_left());
      }
      return rotate_right();
    }
    return this;
  }

  static self* find_minimum(self* node) {
    if (!node) {
      return nullptr;
    }
    return node->left ? find_minimum(node->left) : node;
  }

  static self* find_maximum(self* node) {
    if (!node) {
      return nullptr;
    }
    return node->right ? find_maximum(node->right) : node;
  }

  self* remove_minimum() {
    if (!left) {
      return right;
    }
    set_left(left->remove_minimum());
    return balance();
  }

private:
  static void update(self* e) noexcept {
    update_height(e);
  }

  static std::size_t get_height(self* e) noexcept {
    return e ? e->height : 0;
  }

  static int get_signed_height(self* e) noexcept {
    return static_cast<int>(get_height(e));
  }

  static void update_height(self* e) noexcept {
    if (e) {
      e->height = std::max(get_height(e->left), get_height(e->right)) + 1;
    }
  }

  static void set_parent(self* e,
                         self* p) noexcept {
    if (e) {
      e->parent = p;
    }
  }

protected:
  std::size_t height = 0;

  self * parent = nullptr;
  self* left = nullptr;
  self* right = nullptr;
};

template <typename Tag>
struct set_fake_t : intrusive_set_element<Tag> {};

} // namespace implementation
