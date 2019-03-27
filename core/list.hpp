#pragma once

template <typename T>
class List {
    struct Item {
        Item* next;
        T data;
    };

    struct ConstIter {
        Item* self;
        Item* prev;

        void operator++() {
            prev = self;
            self = self->next;
        }

        bool operator==(const ConstIter& other) const {
            return self == other.self;
        }

        bool operator!=(const ConstIter& other) const {
            return self != other.self;
        }

        const T& operator*() {
            return self->data;
        }
    };

    struct Iter {
        Item* self;
        Item* prev;
        Item** head;
        bool removed;

        void remove() {
            (prev ? prev->next : *head) = self->next;
            delete self;
            removed = true;
        }

        void insert(T value) {
            Item* it = new Item {self, value};
            if (removed) {
                it->next = (prev ? prev->next : *head);
            }
            (prev ? prev->next : *head) = it;
            self = it;
        }

        void operator++() {
            if (removed) {
                self = prev ? prev->next : *head;
                removed = false;
            } else {
                prev = self;
                self = self->next;
            }
        }

        bool operator==(const Iter& other) const {
            return self == other.self;
        }

        bool operator!=(const Iter& other) const {
            return self != other.self;
        }

        T& operator*() {
            return self->data;
        }

        T* operator->() {
            return &self->data;
        }
    };

    Item* m_head;
public:
    ConstIter begin() const {
        return ConstIter { m_head, nullptr };
    }

    ConstIter end() const {
        return ConstIter { nullptr, nullptr };
    }

    Iter begin() {
        return Iter { m_head, nullptr, &m_head, false };
    }

    Iter end() {
        return Iter { nullptr, nullptr, &m_head, false };
    }
};
