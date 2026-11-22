#ifndef CHAINS_HPP_
#define CHAINS_HPP_

#include "slot_pool.hpp"

namespace prometheus::topology {

enum class ChainShape { linear, circular };

template<class Node, Handle<Node> Node::*Next, ChainShape Shape>
struct ChainTraversal {
    template<class Visitor>
        requires std::invocable<Visitor&, Handle<Node>>
    static void for_each(Handle<Node> head, Visitor&& visitor) {
        auto current = head;
        while (!current.is_null()) {
            std::invoke(visitor, Handle<Node>{current});
            current = current.get().*Next;

            if constexpr (Shape == ChainShape::circular) {
                if (current == head)
                    break;
            }
        }
    }
};

template<class Node,
         Handle<Node> Node::*Next,
         Handle<Node> Node::*Prev,
         ChainShape Shape>
struct DoublyLinked: ChainTraversal<Node, Next, Shape> {
    using H = Handle<Node>;

    // item must be detached from this particular chain.
    static void push_front(H& head, H item) {
        Node& node = item.get();

        if (head.is_null()) {
            H end{};
            if constexpr (Shape == ChainShape::circular)
                end = item;

            node.*Prev = end;
            node.*Next = end;
        } else {
            Node& first = head.get();
            H previous = first.*Prev;  // Tail for a ring, null for a linear chain.

            if (!previous.is_null())
                previous.get().*Next = item;

            node.*Prev = previous;
            node.*Next = head;
            first.*Prev = item;
        }
        head = item;
    }

    // item must belong to the chain identified by head.
    static void unlink(H& head, H item) {
        Node& node = item.get();
        H previous = node.*Prev;
        H next = node.*Next;

        // Resolve both neighbors before changing either link.
        Node* before = previous.is_null() ? nullptr : &previous.get();
        Node* after = next.is_null() ? nullptr : &next.get();

        if (before)
            before->*Next = next;
        if (after)
            after->*Prev = previous;

        if (head == item)
            head = (next == item) ? H{} : next;  // A singleton ring becomes empty.

        node.*Prev = H{};
        node.*Next = H{};
    }
};

template<class Node, Handle<Node> Node::*Next>
struct SinglyLinked:  ChainTraversal<Node, Next, ChainShape::linear> {
    using H = Handle<Node>;

    // item must be detached from this chain.
    static void push_front(H& head, H item) {
        item.get().*Next = head;
        head = item;
    }

    // incoming is either the head or a predecessor's next field.
    // Returns the removed item, or a null handle if incoming is null.
    static H unlink_at(H& incoming) {
        H item = incoming;
        if (item.is_null())
            return {};

        H& next = item.get().*Next;
        incoming = next;
        next.nullify();
        return item;
    }

    // O(n). Returns false if item is absent.
    static bool unlink(H& head, H item) {
        H* incoming = &head;
        while (!incoming->is_null()) {
            if (*incoming == item) {
                unlink_at(*incoming);
                return true;
            }
            incoming = &(incoming->get().*Next);
        }
        return false;
    }
};

}

#endif