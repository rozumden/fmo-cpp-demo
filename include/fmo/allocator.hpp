#ifndef FMO_ALLOCATOR_HPP
#define FMO_ALLOCATOR_HPP

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <type_traits>

namespace fmo {
    namespace detail {
        inline constexpr bool is_pow2(size_t x) { return x && (x & (x - 1)) == 0; }

        template <typename T, size_t Align>
        struct aligned_allocator;
        template <typename T, size_t Align = alignof(T), bool Switch = (Align > alignof(double))>
        struct alloc;

        template <typename T, size_t Align>
        struct alloc<T, Align, false> {
            static T* malloc(size_t bytes) { return (T*)std::malloc(bytes); }
            static void free(T* ptr) { std::free(ptr); }
            using allocator = std::allocator<T>;
            using deleter = std::default_delete<T>;
        };

        template <typename T, size_t Align>
        struct alloc<T, Align, true> {
            static_assert(detail::is_pow2(Align), "alignment must be a power of 2");
            static_assert(Align <= 128, "alignment is too large");
            static_assert(Align > alignof(double), "alignment is too small -- use malloc");

            static T* malloc(size_t bytes) {
                auto orig = (uintptr_t)std::malloc(bytes + Align);
                if (orig == 0) return nullptr;
                auto aligned = (orig + Align) & ~(Align - 1);
                auto offset = int8_t(orig - aligned);
                ((int8_t*)aligned)[-1] = offset;
                return (T*)aligned;
            }

            static void free(T* aligned) {
                if (aligned == nullptr) return;
                auto offset = ((int8_t*)aligned)[-1];
                auto orig = uintptr_t(aligned) + offset;
                std::free((void*)orig);
            }

            using allocator = aligned_allocator<T, Align>;

            struct deleter {
                template <typename S>
                void operator()(S* ptr) {
                    free(ptr);
                }
            };
        };

        template <typename T, size_t Align>
        struct aligned_allocator {
            using value_type = T;
            using alloc_t = alloc<T, Align>;

            static_assert(std::is_trivially_destructible<T>::value,
                "allocated type must be trivial");

            aligned_allocator() = default;

            template <typename S>
            aligned_allocator(const aligned_allocator<S, Align>&) {}

            T* allocate(size_t count) const noexcept { return alloc_t::malloc(sizeof(T) * count); }

            void deallocate(T* ptr, size_t) const noexcept { alloc_t::free(ptr); }

            void destroy(T*) const noexcept {}

            void construct(T*) const noexcept {}

            template <typename A1, typename... A>
            void construct(T* ptr, A1&& a1, A&&... a2) const {
                new (ptr) T(std::forward<A1>(a1), std::forward<A...>(a2)...);
            }

            // default rebind should do just this, doesn't seem to work in MSVC though
            template <typename S>
            struct rebind {
                using other = aligned_allocator<S, Align>;
            };
        };

        template <typename T, typename U, size_t TS, size_t US>
        inline bool operator==(const aligned_allocator<T, TS>&, const aligned_allocator<U, US>&) {
            return true;
        }
        template <typename T, typename U, size_t TS, size_t US>
        inline bool operator!=(const aligned_allocator<T, TS>&, const aligned_allocator<U, US>&) {
            return false;
        }
    }
}

#endif // FMO_ALLOCATOR_HPP
