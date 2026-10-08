#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <generator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#define FORMAT_EXAMPLE_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error{msg}; \
        } \
    } while (false)

#define FORMAT_EXAMPLE_MULTILINE_MACRO(name, type) \
    struct name { \
        type value; \
    }

namespace dice::format_example {

    namespace detail {
        const int magic_number = 42;
    } // namespace detail

    enum struct Color : std::uint8_t {
        Red,
        Green,
        Blue,
    };

    enum struct LongEnumeration : std::uint32_t {
        FirstVariantWithAVeryLongName = 1,
        SecondVariantWithAVeryLongName = 2,
        ThirdVariantWithAVeryLongName = 4,
    };

    template<typename T>
    concept Hashable = requires (const T& value) {
        { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
    };

    template<typename T>
    concept NodeLike = std::regular<T> && Hashable<T> && requires (T node) {
        node.id();
        node.is_blank();
    };

    struct Empty {};

    struct Point {
        double x;
        double y;
    };

    struct [[nodiscard]] Config {
        std::string name;
        std::uint32_t num_threads = 4;
        std::optional<std::string> log_file;
        bool verbose = false;
    };

    struct Base {
        virtual ~Base() = default;
        virtual void run() = 0;
        [[nodiscard]] virtual std::string_view name() const noexcept {
            return "base";
        }
    };

    struct Derived final : public Base, private std::enable_shared_from_this<Derived> {
    private:
        std::vector<int> values_;
        std::map<std::string, std::vector<std::pair<int, double>>> lookup_table_;
        std::unique_ptr<Config> config_;

    protected:
        int counter_ = 0;

    public:
        Derived() = default;
        explicit Derived(std::vector<int> values) : values_(std::move(values)) {}
        Derived(
            std::vector<int> values,
            std::map<std::string, std::vector<std::pair<int, double>>> lookup_table,
            std::unique_ptr<Config> config
        ) :
            values_(std::move(values)),
            lookup_table_(std::move(lookup_table)),
            config_(std::move(config)) {}

        void run() override {
            for (const auto& value : values_) {
                counter_ += value;
            }
        }

        [[nodiscard]] std::string_view name() const noexcept override {
            return "derived";
        }

        [[nodiscard]] int counter() const noexcept {
            return counter_;
        }

        void set_counter(int counter) noexcept {
            counter_ = counter;
        }

        Derived& operator+=(const Derived& other) {
            counter_ += other.counter_;
            return *this;
        }

        friend bool operator==(const Derived& lhs, const Derived& rhs) noexcept {
            return lhs.counter_ == rhs.counter_;
        }
    };

    template<
        typename Key,
        typename Value,
        typename Hash = std::hash<Key>,
        typename Allocator = std::allocator<std::pair<const Key, Value>>
    >
    struct Cache {
        using key_type = Key;
        using value_type = Value;

        template<typename K>
        requires std::convertible_to<K, Key>
        [[nodiscard]] std::optional<Value> get(const K& key) const {
            if (auto it = entries_.find(key); it != entries_.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        void insert(Key key, Value value)
        requires std::copy_constructible<Value>
        {
            entries_.insert_or_assign(std::move(key), std::move(value));
        }

    private:
        std::map<Key, Value> entries_;
    };

    static int short_function(int x) {
        return x * 2;
    }

    static void empty_function() {}

    inline std::uint64_t function_with_a_long_signature(
        std::string_view first_parameter,
        const std::vector<std::uint64_t>& second_parameter,
        std::optional<Config> third_parameter
    ) {
        return first_parameter.size() + second_parameter.size() + (third_parameter.has_value() ? 1 : 0);
    }

    [[nodiscard]] static constexpr auto trailing_return_type(int a, int b) noexcept -> std::pair<int, int> {
        return {a, b};
    }

    template<typename T>
    [[nodiscard]] const T& pointer_and_reference_alignment(
        const T* pointer,
        const T& reference,
        T&& forwarding,
        T** double_pointer
    ) {
        (void) forwarding;
        (void) double_pointer;
        if (pointer != nullptr) {
            return *pointer;
        }
        return reference;
    }

    int control_flow(int value, const std::vector<int>& values) {
        if (value < 0) {
            return -1;
        } else if (value == 0) {
            return 0;
        } else {
            value += 1;
        }

        if (value > 10) {
            value = 10;
        }

        for (int i = 0; i < value; ++i) {
            if (i % 2 == 0) {
                continue;
            }
            value += i;
        }

        for (const auto& v : values) {
            value += v;
        }

        while (value > 100) {
            value /= 2;
        }

        do {
            value -= 1;
        } while (value > 50);

        switch (value) {
            case 0: return 0;
            case 1:
            case 2: value += 1; break;
            case 3: {
                const int tmp = value * 2;
                value = tmp;
                break;
            }
            default: break;
        }

        try {
            if (value == 13) {
                throw std::runtime_error{"unlucky"};
            }
        } catch (const std::runtime_error& e) {
            value = 0;
        } catch (...) {
            throw;
        }

        return value;
    }

    bool long_boolean_expression(int first_value, int second_value, int third_value, int fourth_value) {
        if (first_value > second_value
            && second_value > third_value
            && third_value > fourth_value
            && fourth_value > 0
            && first_value < 1000)
        {
            return true;
        }
        const auto result = first_value * second_value
            + third_value * fourth_value
            - first_value / (second_value + 1)
            + third_value % (fourth_value + 1);
        return result > 0
            || (first_value == second_value && third_value == fourth_value)
            || first_value + second_value + third_value + fourth_value == 0;
    }

    int ternaries(int a, int b) {
        const int short_ternary = a > b ? a : b;
        const int long_ternary = a > b ? function_with_a_long_signature("some argument", {1, 2, 3}, std::nullopt)
                                       : function_with_a_long_signature("another argument", {4, 5, 6}, std::nullopt);
        return short_ternary + long_ternary;
    }

    void function_calls() {
        Derived derived;
        derived.run();
        const auto value = function_with_a_long_signature("a short string", {}, std::nullopt);
        const auto another_value = function_with_a_long_signature(
            "a much longer string that pushes things over",
            std::vector<std::uint64_t>{1, 2, 3, 4, 5},
            Config{.name = "config"}
        );
        const auto nested = function_with_a_long_signature(
            std::string_view{"nested"},
            std::vector<std::uint64_t>{
                static_cast<std::uint64_t>(short_function(1)),
                static_cast<std::uint64_t>(short_function(2))
            },
            std::nullopt
        );
        (void) value;
        (void) another_value;
        (void) nested;
    }

    void initializers() {
        const Point p{1.0, 2.0};
        const Point q = {.x = 3.0, .y = 4.0};
        const Config short_config{.name = "short"};
        const Config long_config{
            .name = "a configuration with a long name",
            .num_threads = 16,
            .log_file = "/var/log/some/long/path/to/a/file.log",
            .verbose = true
        };
        const std::vector<int> numbers{1, 2, 3, 4, 5};
        const std::vector<std::string> words{
            "alpha",
            "beta",
            "gamma",
            "delta",
            "epsilon",
            "zeta",
            "eta",
            "theta",
            "iota",
            "kappa",
            "lambda",
            "mu"
        };
        const std::map<std::string, int> mapping{{"one", 1}, {"two", 2}, {"three", 3}};
        const std::vector<Point> points{
            {1.0, 2.0},
            {3.0, 4.0},
        };
        int raw_array[] = {1, 2, 3};
        const auto* const c_string = "hello";
        (void) p, (void) q, (void) short_config, (void) long_config, (void) numbers, (void) words, (void) mapping,
            (void) points, (void) raw_array, (void) c_string;
    }

    void lambdas() {
        const auto add = [](int a, int b) { return a + b; };
        const auto add_with_capture = [offset = 10](int a) noexcept -> int { return a + offset; };
        const auto multi_statement = [&](int a) {
            const auto doubled = add(a, a);
            return doubled + add_with_capture(a);
        };
        const auto generic = []<typename T>(const T& value)
        requires std::integral<T>
        { return value * 2; };

        std::vector<int> values{5, 3, 1, 4, 2};
        std::ranges::sort(values, [](int lhs, int rhs) { return lhs > rhs; });
        std::ranges::sort(values, [](int lhs, int rhs) {
            const auto lhs_mod = lhs % 3;
            const auto rhs_mod = rhs % 3;
            return lhs_mod < rhs_mod;
        });

        const auto result = std::invoke(
            [&values, captured_function = multi_statement, another_capture = generic](int seed) mutable {
                int sum = seed;
                for (const auto v : values) {
                    sum += captured_function(v) + another_capture(v);
                }
                return sum;
            },
            42
        );

        const auto nested = [](int a) { return [a](int b) { return [a, b](int c) { return a + b + c; }; }; };

        (void) result;
        (void) nested;
    }

    void method_chains(const std::vector<int>& values) {
        // clang-format prefers breaking inside a trailing lambda over breaking at `|`, so a range pipeline gets
        // squashed onto one line. An empty `//` after the first operand forces one stage per line.
        auto evens = values //
            | std::views::filter([](int v) { return v % 2 == 0; })
            | std::views::transform([](int v) { return v * v; })
            | std::views::take(10);
        auto squares = values //
            | std::views::filter([](int v) { return v % 2 == 0; })
            | std::views::transform([](int v) {
                  const auto square = v * v;
                  return square;
              })
            | std::views::take(10);

        const auto total = std::ranges::fold_left(
            values | std::views::filter([](int v) { return v > 0; }),
            0,
            std::plus<>{}
        );

        std::string text{"hello"};
        text.append(" world")
            .append(", this is a long chain of appends")
            .append(" that will not fit on a single line")
            .append("!");
    }

    std::generator<int> coroutine(int limit) {
        for (int i = 0; i < limit; ++i) {
            co_yield i;
        }
    }

    template<typename... Args>
    auto fold_expressions(Args&&... args) {
        return (std::forward<Args>(args) + ...);
    }

    template<typename T>
    struct Visitor;

    template<>
    struct Visitor<int> {
        int operator()(int x) const {
            return x;
        }
    };

    std::string variant_visit(const std::variant<int, double, std::string>& variant) {
        return std::visit(
            []<typename T>(const T& value) -> std::string {
                if constexpr (std::is_same_v<T, std::string>) {
                    return value;
                } else {
                    return std::to_string(value);
                }
            },
            variant
        );
    }

    void comments() {
        int a = 1; // trailing comment
        int bb = 2; // another trailing comment, misaligned
        /* block comment */
        int ccc = 3;

        // This is a very long comment line that goes on and on and on and on and on and on and on and past one hundred twenty characters.
        (void) a, (void) bb, (void) ccc;
    }

    void too_many_blank_lines() {
        int x = 1;

        (void) x;
    }

    using Callback = std::function<void(int, std::string_view, const std::optional<Config>&)>;
    using LongTypeAlias = std::map<std::string, std::vector<std::pair<std::uint64_t, std::optional<std::string>>>>;

    static_assert(sizeof(Point) == 2 * sizeof(double), "Point must be two doubles");

    extern "C" {
        int c_function(int x);
    }

} // namespace dice::format_example
