#ifndef BALANCED_TREE_H_INCLUDED
#define BALANCED_TREE_H_INCLUDED

#include <iterator>

template <typename T>
struct default_comparer final
{
    template <typename U>
    int operator()(const T &l, const U &r) const
    {
        if(l < r)
        {
            return -1;
        }

        if(r < l)
        {
            return 1;
        }

        return 0;
    }
};

template <typename T, typename Compare = default_comparer<T>>
class balanced_tree final
{
private:
    struct Node
    {
        T value;
        unsigned depth;
        Node *left, *right;
        Node *prev, *next;
        Node(const T &value)
            : value(value), depth(0)
        {
        }
        Node(T  &&value)
            : value(move(value)), depth(0)
        {
        }
        void calcDepth()
        {
            unsigned newDepth = 0;

            if(left)
            {
                newDepth = 1 + left->depth;
            }

            if(right && right->depth >= newDepth) // equivalent to right->depth + 1 > newDepth
            {
                newDepth = 1 + right->depth;
            }

            depth = newDepth;
        }
    };
    Node *root, *head, *tail;
    void removeNodeFromList(Node *node)
    {
        if(node->prev == nullptr)
        {
            head = node->next;
        }
        else
        {
            node->prev->next = node->next;
        }

        if(node->next == nullptr)
        {
            tail = node->prev;
        }
        else
        {
            node->next->prev = node->prev;
        }
    }
    Compare compare;
    static void rotateLeft(Node  *&node)
    {
        assert(node && node->right);
        Node *tree1 = node->left;
        Node *tree2 = node->right->left;
        Node *tree3 = node->right->right;
        Node *newNode = node->right;
        newNode->left = node;
        node = newNode;
        node->left->left = tree1;
        node->left->right = tree2;
        node->right = tree3;
        node->left->calcDepth();
        node->calcDepth();
    }
    static void rotateRight(Node  *&node)
    {
        assert(node && node->left);
        Node *tree1 = node->left->left;
        Node *tree2 = node->left->right;
        Node *tree3 = node->right;
        Node *newNode = node->left;
        newNode->right = node;
        node = newNode;
        node->left = tree1;
        node->right->left = tree2;
        node->right->right = tree3;
        node->right->calcDepth();
        node->calcDepth();
    }
    static void balanceNode(Node  *&node)
    {
        assert(node);
        unsigned lDepth = 0;

        if(node->left)
        {
            lDepth = node->left->depth + 1;
        }

        unsigned rDepth = 0;

        if(node->right)
        {
            rDepth = node->right->depth + 1;
        }

        if(lDepth > rDepth + 1)
        {
            rotateRight(node);
        }
        else if(rDepth > lDepth + 1)
        {
            rotateLeft(node);
        }
    }
    void insertNode(Node *&tree, Node *newNode, Node *&head, Node *&tail)
    {
        assert(newNode);

        if(tree == nullptr)
        {
            tree = newNode;
            tree->depth = 0;
            tree->left = nullptr;
            tree->right = nullptr;
            tree->next = head;
            tree->prev = tail;
            head = tail = tree;
            return;
        }

        int cmpV = compare(tree->value, newNode->value);

        if(cmpV == 0)
        {
            delete newNode;
            return;
        }
        else if(cmpV < 0)
        {
            insertNode(tree->right, newNode, tree->next, tail);
        }
        else
        {
            insertNode(tree->left, newNode, head, tree->prev);
        }

        tree->calcDepth();
        balanceNode(tree);
    }
    static Node *removeInorderPredecessorH(Node  *&node)
    {
        assert(node != nullptr);

        if(node->right == nullptr)
        {
            Node *retval = node;
            node = node->left;

            if(node != nullptr)
            {
                node->calcDepth();
                balanceNode(node);
            }

            retval->left = retval->right = nullptr;
            retval->depth = 0;
            return retval;
        }

        Node *retval = removeInorderPredecessorH(node->right);
        node->calcDepth();
        balanceNode(node);
        return retval;
    }
    static Node *removeInorderPredecessor(Node *node)
    {
        assert(node != nullptr);
        return removeInorderPredecessorH(node->left);
    }
    template <typename ComparedType>
    Node *removeNode(Node *&tree, ComparedType searchFor)
    {
        if(tree == nullptr)
        {
            return nullptr;
        }

        int cmpV = compare(tree->value, searchFor);
        Node *retval;

        if(cmpV == 0) // found it
        {
            if(tree->left == nullptr && tree->right == nullptr)
            {
                retval = tree;
                tree = nullptr;
                removeNodeFromList(retval);
                return retval;
            }

            if(tree->left == nullptr)
            {
                retval = tree;
                tree = tree->right;
                removeNodeFromList(retval);
                return retval;
            }

            if(tree->right == nullptr)
            {
                retval = tree;
                tree = tree->left;
                removeNodeFromList(retval);
                return retval;
            }

            retval = tree;
            Node *replaceWith = removeInorderPredecessor(tree);
            replaceWith->left = tree->left;
            replaceWith->right = tree->right;
            tree = replaceWith;
            tree->calcDepth();
            balanceNode(tree);
            removeNodeFromList(retval);
            return retval;
        }
        else
        {
            if(cmpV < 0)
            {
                retval = removeNode(tree->right, searchFor);
            }
            else
            {
                retval = removeNode(tree->left, searchFor);
            }

            tree->calcDepth();
            balanceNode(tree);
            return retval;
        }
    }
    template <typename Function, typename ComparedType>
    void forEachNodeInRange(Function &fn, ComparedType min, ComparedType max, Node *tree)
    {
        if(tree == nullptr)
        {
            return;
        }

        bool fits = true;

        if(compare(tree->value, min) >= 0)
        {
            forEachNodeInRange(fn, min, max, tree->left);
        }
        else
        {
            fits = false;
        }

        if(compare(tree->value, max) <= 0)
        {
            if(fits)
            {
                fn(tree->value);
            }

            forEachNodeInRange(fn, min, max, tree->right);
        }
    }
    template <typename ComparedType>
    Node *find(ComparedType value, Node *tree)
    {
        if(tree == nullptr)
        {
            return nullptr;
        }

        int cmpV = compare(tree->value, value);

        if(cmpV == 0)
        {
            return tree;
        }
        else if(cmpV < 0)
        {
            return find(value, tree->right);
        }
        else
        {
            return find(value, tree->left);
        }
    }
    template <typename ComparedType>
    const Node *find(ComparedType value, const Node *tree)
    {
        if(tree == nullptr)
        {
            return nullptr;
        }

        int cmpV = compare(tree->value, value);

        if(cmpV == 0)
        {
            return tree;
        }
        else if(cmpV < 0)
        {
            return find(value, (const Node *)tree->right);
        }
        else
        {
            return find(value, (const Node *)tree->left);
        }
    }
    template <typename Function>
    static void forEachNode(Function &fn, Node *tree)
    {
        if(tree == nullptr)
        {
            return;
        }

        forEachNode(fn, tree->left);
        fn(tree->value);
        forEachNode(fn, tree->right);
    }
    static Node *cloneTree(const Node *tree)
    {
        if(tree == nullptr)
        {
            return nullptr;
        }

        Node *retval = new Node(tree->value);
        retval->left = cloneTree(tree->left);
        retval->right = cloneTree(tree->right);
        retval->depth = tree->depth;
        return retval;
    }
    static void freeTree(Node *tree)
    {
        if(tree == nullptr)
        {
            return;
        }

        freeTree(tree->left);
        freeTree(tree->right);
        delete tree;
    }
    static void constructList(Node *tree, Node *&head, Node *&tail)
    {
        if(tree == nullptr)
        {
            return;
        }

        tree->prev = tail;
        tree->next = head;
        head = tail = tree;
        constructList(tree->left, head, tree->prev);
        constructList(tree->right, tree->next, tail);
    }
public:
    friend class iterator;
    friend class const_iterator;
    class const_iterator final : public std::iterator<std::bidirectional_iterator_tag, T>
    {
        friend class balanced_tree;
        friend class iterator;
    private:
        const Node *node;
    public:
        const_iterator()
            : node(nullptr)
        {
        }
        const_iterator(const Node *node)
            : node(node)
        {
        }
        const const_iterator &operator ++()
        {
            node = node->next;
            return *this;
        }
        const_iterator operator ++(int)
        {
            auto retval = *this;
            node = node->next;
            return retval;
        }
        const const_iterator &operator --()
        {
            node = node->prev;
            return *this;
        }
        const_iterator operator --(int)
        {
            auto retval = *this;
            node = node->prev;
            return retval;
        }
        friend bool operator ==(const_iterator a, const_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(const_iterator a, const_iterator b)
        {
            return a.node != b.node;
        }
        const T &operator *() const
        {
            return node->value;
        }
        const T *operator ->() const
        {
            return &node->value;
        }
    };
    class iterator final : public std::iterator<std::bidirectional_iterator_tag, T>
    {
        friend class balanced_tree;
    private:
        Node *node;
    public:
        iterator()
            : node(nullptr)
        {
        }
        iterator(Node *node)
            : node(node)
        {
        }
        operator const_iterator() const
        {
            return const_iterator(node);
        }
        const iterator &operator ++()
        {
            node = node->next;
            return *this;
        }
        iterator operator ++(int)
        {
            auto retval = *this;
            node = node->next;
            return retval;
        }
        const iterator &operator --()
        {
            node = node->prev;
            return *this;
        }
        iterator operator --(int)
        {
            auto retval = *this;
            node = node->prev;
            return retval;
        }
        friend bool operator ==(iterator a, iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(iterator a, iterator b)
        {
            return a.node != b.node;
        }
        friend bool operator ==(const_iterator a, iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(const_iterator a, iterator b)
        {
            return a.node != b.node;
        }
        friend bool operator ==(iterator a, const_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(iterator a, const_iterator b)
        {
            return a.node != b.node;
        }
        T &operator *() const
        {
            return node->value;
        }
        T *operator ->() const
        {
            return &node->value;
        }
    };
    friend class reverse_iterator;
    friend class const_reverse_iterator;
    class const_reverse_iterator final : public std::iterator<std::bidirectional_iterator_tag, T>
    {
        friend class balanced_tree;
        friend class reverse_iterator;
    private:
        const Node *node;
    public:
        const_reverse_iterator()
            : node(nullptr)
        {
        }
        const_reverse_iterator(const Node *node)
            : node(node)
        {
        }
        const const_reverse_iterator &operator ++()
        {
            node = node->prev;
            return *this;
        }
        const_reverse_iterator operator ++(int)
        {
            auto retval = *this;
            node = node->prev;
            return retval;
        }
        const const_reverse_iterator &operator --()
        {
            node = node->next;
            return *this;
        }
        const_reverse_iterator operator --(int)
        {
            auto retval = *this;
            node = node->next;
            return retval;
        }
        friend bool operator ==(const_reverse_iterator a, const_reverse_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(const_reverse_iterator a, const_reverse_iterator b)
        {
            return a.node != b.node;
        }
        const T &operator *() const
        {
            return node->value;
        }
        const T *operator ->() const
        {
            return &node->value;
        }
    };
    class reverse_iterator final : public std::iterator<std::bidirectional_iterator_tag, T>
    {
        friend class balanced_tree;
    private:
        Node *node;
    public:
        reverse_iterator()
            : node(nullptr)
        {
        }
        reverse_iterator(Node *node)
            : node(node)
        {
        }
        operator const_reverse_iterator() const
        {
            return const_reverse_iterator(node);
        }
        const reverse_iterator &operator ++()
        {
            node = node->prev;
            return *this;
        }
        reverse_iterator operator ++(int)
        {
            auto retval = *this;
            node = node->prev;
            return retval;
        }
        const reverse_iterator &operator --()
        {
            node = node->next;
            return *this;
        }
        reverse_iterator operator --(int)
        {
            auto retval = *this;
            node = node->next;
            return retval;
        }
        friend bool operator ==(reverse_iterator a, reverse_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(reverse_iterator a, reverse_iterator b)
        {
            return a.node != b.node;
        }
        friend bool operator ==(const_reverse_iterator a, reverse_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(const_reverse_iterator a, reverse_iterator b)
        {
            return a.node != b.node;
        }
        friend bool operator ==(reverse_iterator a, const_reverse_iterator b)
        {
            return a.node == b.node;
        }
        friend bool operator !=(reverse_iterator a, const_reverse_iterator b)
        {
            return a.node != b.node;
        }
        T &operator *() const
        {
            return node->value;
        }
        T *operator ->() const
        {
            return &node->value;
        }
    };
    balanced_tree()
        : root(nullptr), head(nullptr), tail(nullptr), compare()
    {
    }
    explicit balanced_tree(const Compare &compare)
        : root(nullptr), head(nullptr), tail(nullptr), compare(compare)
    {
    }
    explicit balanced_tree(Compare  &&compare)
        : root(nullptr), head(nullptr), tail(nullptr), compare(move(compare))
    {
    }
    balanced_tree(const balanced_tree &rt)
        : root(cloneTree(rt)), head(nullptr), tail(nullptr), compare(rt.compare)
    {
        constructList(root, head, tail);
    }
    balanced_tree(balanced_tree  &&rt)
        : root(rt.root), head(rt.head), tail(rt.tail), compare(rt.compare)
    {
        rt.root = nullptr;
        rt.head = nullptr;
        rt.tail = nullptr;
    }
    ~balanced_tree()
    {
        freeTree(root);
    }
    const balanced_tree &operator =(const balanced_tree &rt)
    {
        if(root == rt.root)
        {
            return *this;
        }

        clear();
        root = cloneTree(rt.root);
        constructList(root, head, tail);
        compare = rt.compare;
        return *this;
    }
    const balanced_tree &operator =(balanced_tree && rt)
    {
        swap(root, rt.root);
        swap(head, rt.head);
        swap(tail, rt.tail);
        swap(compare, rt.compare);
        return *this;
    }
    void clear()
    {
        freeTree(root);
        root = nullptr;
        head = nullptr;
        tail = nullptr;
    }
    template <typename Function>
    void forEach(Function &fn)
    {
        forEachNode(fn, root);
    }
    template <typename Function, typename ComparedType>
    void forEachInRange(Function &fn, ComparedType min, ComparedType max)
    {
        forEachNodeInRange(fn, min, max, root);
    }
    template <typename ComparedType>
    const_iterator find(ComparedType value) const
    {
        return iterator(find(value, (const Node *)root));
    }
    template <typename ComparedType>
    iterator get(ComparedType value)
    {
        return iterator(find(value, root));
    }
    void insert(const T &value)
    {
        insertNode(root, new Node(value), head, tail);
    }
    void insert(T  &&value)
    {
        insertNode(root, new Node(move(value)), head, tail);
    }
    template <typename ComparedType>
    bool erase(ComparedType searchFor)
    {
        Node *node = removeNode(root, searchFor);

        if(node == nullptr)
        {
            return false;
        }

        delete node;
        return true;
    }
    iterator erase(iterator iter)
    {
        iterator retval = iter;
        retval++;
        erase<const T &>(*iter);
        return retval;
    }
    const_iterator erase(const_iterator iter)
    {
        const_iterator retval = iter;
        retval++;
        erase<const T &>(*iter);
        return retval;
    }
    iterator begin()
    {
        return iterator(head);
    }
    const_iterator begin() const
    {
        return const_iterator(head);
    }
    const_iterator cbegin() const
    {
        return const_iterator(head);
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(tail);
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(tail);
    }
    const_reverse_iterator crbegin() const
    {
        return const_iterator(tail);
    }
    iterator end()
    {
        return iterator(nullptr);
    }
    const_iterator end() const
    {
        return const_iterator(nullptr);
    }
    const_iterator cend() const
    {
        return const_iterator(nullptr);
    }
    reverse_iterator rend()
    {
        return reverse_iterator(nullptr);
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(nullptr);
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(nullptr);
    }
    template <typename CompareType>
    const_iterator rangeCBegin(CompareType searchFor) const
    {
        const Node *node = root, *lastNode = root;

        while(node != nullptr)
        {
            lastNode = node;
            int cmpV = compare(node->value, searchFor);

            if(cmpV == 0)
            {
                break;
            }

            if(cmpV > 0)
            {
                node = node->left;
            }
            else
            {
                node = node->right;

                if(node == nullptr)
                {
                    lastNode = lastNode->next;
                }
            }
        }

        return const_iterator(lastNode);
    }
    template <typename CompareType>
    const_iterator rangeCEnd(CompareType searchFor) const
    {
        const Node *node = root, *lastNode = root;

        while(node != nullptr)
        {
            lastNode = node;
            int cmpV = compare(node->value, searchFor);

            if(cmpV == 0)
            {
                return const_iterator(lastNode->next);
            }

            if(cmpV > 0)
            {
                node = node->left;
            }
            else
            {
                node = node->right;

                if(node == nullptr)
                {
                    lastNode = lastNode->next;
                }
            }
        }

        return const_iterator(lastNode);
    }
    template <typename CompareType>
    iterator rangeBegin(CompareType searchFor)
    {
        Node *node = root, *lastNode = root;

        while(node != nullptr)
        {
            lastNode = node;
            int cmpV = compare(node->value, searchFor);

            if(cmpV == 0)
            {
                break;
            }

            if(cmpV > 0)
            {
                node = node->left;
            }
            else
            {
                node = node->right;

                if(node == nullptr)
                {
                    lastNode = lastNode->next;
                }
            }
        }

        return iterator(lastNode);
    }
    template <typename CompareType>
    iterator rangeEnd(CompareType searchFor)
    {
        Node *node = root, *lastNode = root;

        while(node != nullptr)
        {
            lastNode = node;
            int cmpV = compare(node->value, searchFor);

            if(cmpV == 0)
            {
                return iterator(lastNode->next);
            }

            if(cmpV > 0)
            {
                node = node->left;
            }
            else
            {
                node = node->right;

                if(node == nullptr)
                {
                    lastNode = lastNode->next;
                }
            }
        }

        return iterator(lastNode);
    }
};

#endif // BALANCED_TREE_H_INCLUDED
