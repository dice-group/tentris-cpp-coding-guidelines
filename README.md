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

Guidance:
Use legacy declarations for all normal things.
Use explicit declarations for deducing this.

Reason: the explicit version is just a little too much boilerplate for regular usage. 


## `std::ranges` from hell
This section is about quirks in `std::ranges` that will come to haunt you if you ever need to write
custom views.

### What even are `std::ranges::owning_view`, `std::ranges::ref_view` and `std::views::all`?
In ranges pipelines it is only possible to use views. Not all ranges are views.
For instance, `std::vector` is a range, but not a view; makes sense.

So how is it possible to use a `std::vector` in a ranges pipeline?
Through the magic of `std::views::all`, the ranges library implicitly wraps the vector
in `std::views::all_t` (via `std::views::all`) to make it a view.

`std::views::all_t<R>` is basically just:
- the range itself if it is already a view
- `std::ranges::ref_view<R>` if `R` is an l-value but **not** a view yet
- `std::ranges::owning_view<R>` if `R` is an r-value but **not** a view yet

`std::ranges::ref_view<R>` just wraps a reference to the original range and makes it a view.  
`std::ranges::owning_view<R>` takes ownership of the original range and makes it a **move only** view.

The quirk is that, counterintuitively, there are actually non-copyable views. This is fine in principle, you just need to think about
it occasionally.


### Single-Use Ranges
They don't exist. There is no such thing as a single-use range, it is assumed that you can call `begin()` as often as you want
on any range.

The following is **not** a valid range, even though it could technically be one.
```c++
struct R {
    auto begin() &&;
    static auto end();
};
```

### Reversible Ranges
You probably know `std::views::reverse`, it is a handy tool to reverse any range. Except that it isn't always.

You might think, that all you need to reverse a range is:
```c++
template<typename R>
concept reverse_range = requires (R &range) {
    std::ranges::rbegin(range);
    std::ranges::rend(range);
};
```

And you would be right, but this is **not** what `std::views::reverse` requires.
`std::views::reverse` actually requires a `std::ranges::bidirectional_range`,  which is
```c++
template<typename R>
concept bidirectional_range = std::ranges::range<R> 
    && std::ranges::bidirectional_iterator<std::ranges::iterator_t<R>>;
```

So, according to the standard committee, to be able to reverse a range, the iterator must actually be able to iterate
in both directions, regardless of the range providing `rbegin()` and `rend()`.

This is because they just always create a reversed-range by wrapping the `begin()` and `end()` iterators in `std::reverse_iterator`
instead of using the `reverse_iterator` of the range itself.


### Random access ranges
For some reason random access strictly requires bidirectional access. This is probably an artifact from the
legacy iterator concepts that they didn't bother to change.

You cannot have a forward-range with forward-random-access and fulfill `std::ranges::random_access_range`.
Also, there is no `forward_random_access_range`.

Algorithms that would be perfectly fine with just forward-random-access still require full-random-access with bidirectional-random-access-iterators.

### Storing temporaries from operator*
If you need to perform multiple operations on the result of operator*, e.g. when you are implementing a pipeline-view,
you might be tempted to write something like the following:

```c++
auto operator*() {
    auto v = *base_iter_;
    
    if (pred(v)) {
        return op1(v);
    } else {
        return op2(v);
    }
}
```

This however, is inefficient in some cases.

Example 1:
- the return-type of the base iterator is `std::vector<int> const &`.
- the predicate takes the argument by `const &`
- op1 and op2 just fetch the size of the vector (they also take `const &`)

Now we have the scenario where we copy the whole vector even when we don't have to.
Putting `op1(std::move(v))` does not help here (because `std::vector<int> const &` is not movable).
It would help in some cases where `op1` and `op2` want to take ownership, but here we are lost.

So now we try to change the function to avoid this issue:
```c++
auto operator*() {
    decltype(auto) v = *base_iter_;  // change here decltype(auto) vs auto
    
    if (pred(v)) {
        return op1(v);
    } else {
        return op2(v);
    }
}
```

Nice, now we don't unnecessarily copy the vector. But there is an issue.

Example 2:
- the return-type of the base iterator is `std::vector<int>` (plain value)
- pred takes its argument by `const &`
- op1 and op2 want to take ownership of the vector to transform it, so they take it by value

Now, we again copy the vector, this time at the `op1` and `op2` calls.

Ok, surely now we just put `std::move(v)` into the `op1` and `op2` calls, right?

No, wrong again.

Example 3:
In the case of `std::vector<std::vector<int>>` (non-const) the outer-vector-iterator returns `std::vector<int> &`,
and our view would always move all values out of the outer vector. This is certainly wrong any not what
our caller expected, if they had wanted that they would have used `std::views::as_rvalue`, but they did not.

And now we are stuck writing repetitive `if constexpr (std::is_reference_v<decltype(v)>)` branches everywhere
and basically have two implementations of exactly the same function, one for references and one for values.

The solution is `DICE_MOVE_IF_VALUE` from dice-template-library. We, once again, rewrite the function

```c++
decltype(auto) operator*() {
    decltype(auto) v = *base_iter_;
    
    if (pred(v)) {
        return op1(DICE_MOVE_IF_VALUE(v));
    } else {
        return op2(DICE_MOVE_IF_VALUE(v));
    }
}
```

Finally, we have arrived at a solution that is as efficient as it can be. No necessary copies.

And we are left wondering... Why is C++ this way? Somehow all these problems only exist in C++, rust has **ZERO** of them.
