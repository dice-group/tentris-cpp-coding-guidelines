# Tentris Coding Guidelines

This document summarizes the agreed upon coding convention within the tentris library ecosystem.

## Return-value-optimization and Named-RVO (RVO, NRVO)

RVO (and NRVO) are compiler optimizations that can elide a copy or move on function return, resulting in zero-cost value return from a function.

There are however certain conditions that must be met, for this optimization to be applied. 

```cpp
// 1: don't move function result into variable
auto x = std::move(func()); // bad, forces a move, prevents RVO

// 2: prefer direct return of value vs naming the value first
std::string f_bad(bool b) {
    std::string ret;
	
    if (b) {
        ret = "Hello";
    } else {
        ret = "World";
    }
	
    return ret;
}

std::string f_good(bool b) {
    if (b) {
        return "Hello";
    } else {
        return "World";
    }
}

// 3: do not make (non-trivial) objects you intend to return const
std::vector<int> g_bad() {
    std::vector<int> const x{1, 2, 3}; // the const here prevents NRVO, don't make (non-trivial) things you want to return later const
    return x;
}

// 4: do not move the return value inside the function
std::vector<int> h_bad() {
    std::vector<int> x{1, 2, 3};
    return std::move(x); // just as bad as 2, the move prevents NRVO
}

// 5: avoid naming things (if it doesn't make the code unreadable)
struct S {
    std::string s;
};

void i_worst() {
    S x;
    x.s = "Hello";
	
    S y;
    y.s = "World";
	
    std::array<S, 2> arr{std::move(x), std::move(y)}; // not optimized, requires move
}

void i_best() {
    std::array<S, 2> arr{S{.s = "Hello"}, S{.s = "World"}}; // this is optimized
}

void i_if_best_is_not_possible() {
    auto create_S = [](char const *s) {
        S x;
        x.s = s;
        return x; // NRVO applies here
    };
	
    std::array<S, 2> arr{create_S("Hello"), create_S("World")}; // this is still optimized because of NRVO
}
```

## Forwarding constructors
Sometimes when you have a wrapper type around another object you maybe be tempted to write a constructor like the following:
```c++
struct Wrapper {
private:
    Inner inner_;

public:
    template<typename ...Args>
    explicit Wrapper(Args &&...args) : inner_{std::forward<Args>(args)...} {
    }

    Wrapper(Wrapper const &other) = default;
    Wrapper(Wrapper &&other) = default;
};
```

The issue with this construct is that the forwarding constructor **swallows all constructor calls**, even those that would normally go to the copy or move constructors of `Wrapper`.
To fix this we can utilize `std::in_place_t` like so:

```c++
struct Wrapper {
private:
    Inner inner_;

public:
    template<typename ...Args>
    explicit Wrapper(std::in_place_t, Args &&...args) : inner_{std::forward<Args>(args)...} {
    }

    Wrapper(Wrapper const &other) = default;
    Wrapper(Wrapper &&other) = default;
};
```

The tag type resolves this issue.

## Casing of Entities

### global type names

`PascalCase`

### function names, local variable names, parameter names

`snake_case`

### member type names (types inside `struct`s; `typedefs` and `struct` definitions)

`snake_case` to conform to standard library and make `std::` algorithms usable, 

even if the types have nothing to do with stdlib we use `snake_case` for consistency.

### global variables, constants (= `constexpr`) at any scope (including those inside `structs`)

`snake_case` to conform to standard library and make `std::` algorithms usable, 

even if the values have nothing to do with stdlib we use `snake_case` for consistency.

### enum variants

`PascalCase`

### global `typedef`s

`PascalCase` to stay consistent with other types

### template parameters

- type parameters: `PascalCase` to stay consistent with global type names
- value parameters: `snake_case` to stay consistent with global constants

### concepts

- `PascalCase`

## Qualifiers

### `const` and `volatile` qualifier location

Right, as in `int const &x` to keep consistency of the entity the `const` applies to.

For example of inconsistency: `const int *const x` is inconsistent because the first `const` applies to the `int` on its **right** but the second `const` applies to the pointer on its **left**.

### `noexcept` and allocations

- If implementing fundamental functionality (e.g. if implementing a simple, reusable datastructure): correctly mark everything, i.e. we **do not** consider an allocation as `noexcept`
- otherwise: we consider allocation infallible and therefore `noexcept`

## Miscellaneous

### Curly Braces

Curly braces are **required** for all control flow constructs (e.g. `if`, `for`, `while`, etc.)

even if C++ allows to omit them.

### `class` vs `struct` (including `enum struct`)

Always use `struct` because:

- `class` is a redundant keyword in C++, there is no meaningful difference between a `class` and a `struct`
- mismatching `class` and `struct` causes errors on some compilers (e.g. MSVC)
- if we assign some arbitrary meaning to `class` and `struct` and a type that was a `struct` before changes to become a `class`
    
    all forward declarations are suddenly wrong
    

### `class` vs `typename` in template parameters

Always use `typename` because:

- `class` is redundant here as well
- `class` doesn’t make any sense in this context because `int` is not a `class` but you can pass it into `class` template parameters

### Symbolic vs alternative operators (`&&` / `and`, etc.)

Always use the symbolic operator because words (like `and`) are easily missed in long conditions.

The symbolic versions will always stand out in a condition filled with words because they are symbols and not just more words.

Additionally, the operand association of alternative operators is hard to mentally parse.

Example:

```cpp
bool const x = !a && b; // clear, easy association between operators and operands
bool const y = not a and b; // hard to parse visually, would need parentheses to make association clear

// funny trivia of how they are implemented: the following declarations are equivalent
void f(int &x);
void f(int bitand x);
```

### Member function declaration style (legacy vs explicit) (C++23)

Example:

```cpp
// legacy
struct S {
    int x;
	
    void f() const {
        do_stuff(this->x);
    }
};

// explicit
struct S {
    int x;
	
    void f(this S const &self) {
        do_stuff(self.x);
    }
};
```

Guidance suggestion:

Always use explicit style. Reasons:

1. explicit style can pass object by value (just leave out the reference)
2. explicit style supports universal/forwarding reference this (just make it a template with && self param)
3. taking the address of a new-style member function is actually just a regular function pointer, not a member-function-pointer. Just like it should be.
