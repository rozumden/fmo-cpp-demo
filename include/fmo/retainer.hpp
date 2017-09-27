#ifndef FMO_RETAINER_HPP
#define FMO_RETAINER_HPP

#include <array>
#include <vector>

namespace fmo {
    /// A container for objects of type T, which must be default constructible and also must have a
    /// method named clear() which reverts the object to a state equivalent to that after default
    /// construction.
    ///
    /// The first Count elements are never destroyed, clear() is called on them instead.
    template <typename T, size_t Count>
    struct Retainer {
    private:
        struct ConstIterator {
            const Retainer* instance;
            size_t pos;

            ConstIterator operator++() {
                pos++;
                return *this;
            }
            ConstIterator operator++(int) {
                auto prev = *this;
                pos++;
                return prev;
            }

            const T& operator*() const { return (*instance)[pos]; }

            const T* operator->() const { return &operator*(); }

            bool operator==(ConstIterator rhs) { return pos == rhs.pos; }
            bool operator!=(ConstIterator rhs) { return pos != rhs.pos; }
        };

    public:
        Retainer() = default;
        Retainer(const Retainer&) = default;
        Retainer(Retainer&&) = default;
        Retainer& operator=(const Retainer&) = default;
        Retainer& operator=(Retainer&&) = default;

        void swap(Retainer& rhs) {
            mArr.swap(rhs.mArr);
            mVec.swap(rhs.mVec);
            std::swap(mSz, rhs.mSz);
        }
        friend void swap(Retainer& lhs, Retainer& rhs) { lhs.swap(rhs); }

        bool empty() const { return mSz == 0; }
        size_t size() const { return mSz; }

        void emplace_back() {
            if (++mSz > Count) { mVec.emplace_back(); }
        }

        void pop_back() {
            if (mSz-- > Count) {
                mVec.pop_back();
            } else {
                mArr[mSz].clear();
            }
        }

        T& back() {
            if (mSz > Count) return mVec.back();
            return mArr[mSz - 1];
        }

        const T& back() const {
            if (mSz > Count) return mVec.back();
            return mArr[mSz - 1];
        }

        void clear() {
            if (mSz > Count) {
                mVec.clear();
                mSz = Count;
            }
            while (!empty()) pop_back();
        }

        T& operator[](size_t pos) {
            if (pos < Count) {
                return mArr[pos];
            } else {
                return mVec[pos - Count];
            }
        }

        const T& operator[](size_t pos) const {
            if (pos < Count) {
                return mArr[pos];
            } else {
                return mVec[pos - Count];
            }
        }

        ConstIterator begin() const { return {this, 0}; }
        ConstIterator end() const { return {this, mSz}; }
        friend ConstIterator begin(const Retainer& r) { return r.begin(); }
        friend ConstIterator end(const Retainer& r) { return r.end(); }

    private:
        std::array<T, Count> mArr;
        std::vector<T> mVec;
        size_t mSz = 0;
    };
}

#endif // FMO_RETAINER_HPP
