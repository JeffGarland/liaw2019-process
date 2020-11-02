# A proposal to add environment function utlities to the standard library

C++  lacks proper handling of environment variables. The only standardized function so far is `getenv` to read a single environment variable, which cannot be consindered sufficient. The main motivation for this paper is connected to process mangagement as proposed in P1750. In order to keep the proposals mangageable this content was moved to a different paper, to be evaluated independently.


## Other implementation
### Other languages
Other languages generally do provide proxy-types to access the environment, e.g. `os.environ` in python, that can be used as `os.environ["PATH"] = "/bin"`. Node.js operates similarly with `process.env`. The main difference is, that those languages are garbage collected, so the ownership issues we'll discuss later don't arise. Secondly, those languages have a runtime that can enforce using extra globals, which can become difficult in C++, due to it's access to C-APIs. 

Note that a C++ program might use a third party library, that access the system environment. Should an environment library add additional buffering to the environment, data races can lead to major bugs.

### Qt
The Qt framework, ships an implementation of an environment, [`QProcessEnvironment`](https://doc.qt.io/qt-5/qprocessenvironment.html#QProcessEnvironment). It does not come with functionality to modify the environment of the current process, but is meant to be used to build an environment for a launched processed. The environment of the current process can be obtained as a copy by calling `QProcessEnvironment::systemEnvironment`.

```cpp
Q_CORE_EXPORT QByteArray qgetenv(const char *varName);
// need it as two functions because QString is only forward-declared here
Q_CORE_EXPORT QString qEnvironmentVariable(const char *varName);
Q_CORE_EXPORT QString qEnvironmentVariable(const char *varName, const QString &defaultValue);
Q_CORE_EXPORT bool qputenv(const char *varName, const QByteArray& value);
Q_CORE_EXPORT bool qunsetenv(const char *varName);Q_CORE_EXPORT bool qEnvironmentVariableIsEmpty(const char *varName) noexcept;
Q_CORE_EXPORT bool qEnvironmentVariableIsSet(const char *varName) noexcept;
Q_CORE_EXPORT int  qEnvironmentVariableIntValue(const char *varName, bool *ok=nullptr) noexcept;
```

The distinction between `QString` (UTF-16) and `char` is very important. You must address that in your paper too.



### boost.process

`boost.process` provides two environment types, the `boost::process::environment`, representing an environment passed to a process to be launched. Secondly the `native_environment ` that acts as a proxy type for the environment of the current process and can be obtained by calling `boost::this_process::envrionment()`.  An `environment` can be constructed from a `native_environment` so that a user can use the environment of the current process as a basis for the one he wants to pass into the child process.

In addition, there are `wchar_t` versions for both.

As the author of this library and maintainer, my experience caused me to consider some of the design decisions problematic. The proxy type obfuscates what is actually going on, while introducing possible data races, since holding several instances of a `native_environment` can cause them to be out of sync.

Secondly, the flexibility given by having two versions of the environment for both used char-types just causes too many char conversions. The model chosen by `std::filesystem` is clearly superior.


## Current APIs

### std::getenv

Currently, the C++ standard has one function related to environment variables:

```cpp
char* getenv( const char* env_var ); //get an environment variable
```

This function allows the user to query a single environment variable, which returns a pointer to it's value. Writing to the pointed to memory is undefined behavior, as it might modify the process environment. Furthermore, a subsequent call to `getenv` might change the content of a previously aquired value, since `getenv` is allowed to use static memory acquisition.

### System APIs

The first thing to discuss is what the environment actually is. It is a list of variables, all with a name and value. The value is a list of string, separated by either `;` (on windows) or `:` (on posix). This find difference is important for proper usage of environment variables.

```
PATH=/bin:/usr/bin
Path=C:\Python37\;C:\mingw\bin
```

Both of the above are path variables, which can be interpreted either as a `string`, as a key-`string` pair, or as a `key`-`range<string>` pair.

The posix functions & globals for accessing the environment are:

```cpp
extern char ** environ; // the global variable that stores the environment
int setenv(const char *envname, const char *envval, int overwrite); //Set or overwrite an environment variable
int unsetenv(const char *name); //Remove an environment variable
int putenv(char *string); // Set an environment variable with a full string e.g. "PATH=/bin"
```

None of those functions are thread-safe. That is a value acquired by `getenv` can be invalidated by a subsequence call to `setenv`.

On windows the functions to be used are:

```cpp
LPCH GetEnvironmentStrings();
BOOL FreeEnvironmentStrings(LPCH penv); //to be used to free the memory aquired by GetEnvironmentStrings
DWORD GetEnvironmentVariable(LPCTSTR lpName, LPTSTR  lpBuffer, DWORD   nSize);
BOOL SetEnvironmentVariable(LPCTSTR lpName, LPCTSTR lpValue);
```

It is very important to note, posix allows the pointer to point to a global piece of memory, meaning that a change in the environment variables will be noticed by a previously acquired pointer to `environ`. This is not the case on windows.  A change done by `SetEnvironmentVariable` will not be noticed by an environment pointer acquired by `GetEnvironmentStrings` previously. The same is true for an environment acquired through the non-standard third argument of `main`.

It is important to note that a pointer to the environment variables acquired through `GetEnvironmentStrings ` are owning, yet it does not allow write operations. At the same time, the `env` passed into `main` is a non-owning pointer on a windows machine.

Thus the life-time of the pointers accessing the environment is an issue, in addition to the different encodings.

Another thing to note is that environment variable names are case-sensitive on posix, but not on windows.

### Non-standard `main`

Another supported, non-standard version to get a reference to the environment is the before mentioned non-standard option to get a handle to the environment:

```cpp
int main(int argc, char*argv[], char *env[])
{
    return 0;
}
```

This is not standard C++ or C, but is typically supported by unix-like systems and officially by windows. Since this is a common pattern, it should be taken into account by our implementation.

Do note, that the `env` pointer will be the same as `extern char **environ;` on posix, while it holds a copy on windows. 

## Design 

### env::ref

The first problem that arises is pointer ownership, when one obtains a reference to a full `environment`, either by using the the `environ` variable or the `GetEnvironmentStrings` function.

To keep this as flexible as possible, this paper proposes the introduction of  `ref`, a reference to an environment. This reference, may or may not be owning, depending on the implementation and may or may not be pointing to a globally shared memory space. It shall be movable, but not copy-able and have a `native_handle`, returning the actual underlying handle to the environment. 

By putting the environment in a separate handle, it makes ownership questions much more obvious.

### Variable, key and Value

For purposes of this porposal, a variable shall be a string-value of an implementation defined encoding, that contains at least one `=`. The part before the the `=` is considered the key, the part behind that the value, which is a list of string, with an implementation defined separator. 

The purpose of this library is to allow generating string representing environment variables as well as parsing them.

### Functions

The API is modeled to reflect the posix api, but to keep it different enough, the functions shall be as follows:

```cpp
env::get(); // similar to ::getenv
env::set(); // similar to ::setenv
env::unset(); // similar to ::unsetenv
```

## Specification

### std::env::key_traits

The first class to be defined is the class template `key_traits`, which shall implement the same functions as `std::char_traits`, and overload the `compare`, `eq` and `lt` functions to fit the semantics of environment variables names of the system. That is those functions are implementation-defined. Possible implementation:

```cpp
namespace std::env 
{
template<typename CharT>
struct key_traits : std::char_traits<CharT> 
{
    static constexpr int compare( const char_type* s1, const char_type* s2, std::size_t count );
    static constexpr bool lt( char_type a, char_type b ) noexcept;
    static constexpr bool eq( char_type a, char_type b ) noexcept;
};
}
```

The template shall be defined for `char`, `wchar_t`, `char16_t`, `char32_t`, `char8_t`.

Based on this the following aliases shall be declared:

```cpp
namespace std::env 
{
template<typename CharT>
using basic_key = std::basic_string<CharT, key_traits<char>>;
using    key = basic_key<    char>;
using  u8key = basic_key< char8_t>;
using u16key = basic_key<char16_t>;
using u32key = basic_key<char32_t>;
using   wkey = basic_key< wchar_t>;

using    key_view = std::basic_string_view<    char, key_traits<char>>;
using  u8key_view = std::basic_string_view< char8_t, key_traits<char8_t>>;
using u16key_view = std::basic_string_view<char16_t, key_traits<char16_t>>;
using u32key_view = std::basic_string_view<char32_t, key_traits<char32_t>>;
using   wkey_view = std::basic_string_view< wchar_t, key_traits<wchar_t>>;
}
```

### std::env::variable_view

A value holding an environment variable can be interpreted as such with the `variable_view`:

```cpp
namespace std::env
{

class variable_view
{
public:
    using value_type = /* implementation defined, probably char on posix, wchar_t on windows */ ;
    using key_type      = basic_key<value_type>;
    using key_view_type = basic_key_view<value_type>;

    using string_type      = basic_string<value_type, key_traits<value_type>>;
    using string_view_type = basic_string_view<value_type, key_traits<value_type>>;
    
    //construct it 
    constexpr variable_view() noexcept;
    constexpr variable_view(const variable &) noexcept;
    constexpr variable_view(const variable_view& other) noexcept = default;
    constexpr variable_view(const value_type* s, size_type count);
    constexpr variable_view(const value_type* s);

    constexpr variable_view& operator=( const variable_view& view ) noexcept = default;

    
    //get the underlying data
    const value_type* c_str() const noexcept;
    const string_view& native() const noexcept;
    operator string_view_type() const;
    
	//get the entire value as a string
    template< class CharT, class Traits = char_traits<CharT>, class Alloc = allocator<CharT> >
	basic_string<CharT,Traits,Alloc> string( const Alloc& a = Alloc() ) const;
	string       string() const;
	wstring     wstring() const;
	u16string u16string() const;
	u32string u32string() const;
	u8string   u8string() const;
    
    //get the key as view
    key_view_type native_key() const; 

    //get the key as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    basic_string<CharT,Traits,Alloc> key_string( const Alloc& a = Alloc() ) const;
    key       key_string() const;
    wkey     wkey_string() const;
    u8key   u8key_string() const;
    u16key u16key_string() const;
    u32key u32key_string() const;

    //get the value as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    basic_string<CharT,Traits,Alloc> value( const Alloc& a = Alloc() ) const;
    key       value() const;
    wkey     wvalue() const;
    u8key   u8value() const;
    u16key u16value() const;
    u32key u32value() const;

    //get the value as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    std::vector<basic_string<CharT,Traits,Alloc>> values( const Alloc& a = Alloc() ) const;
    std::vector<key>       values() const;
    std::vector<wkey>     wvalues() const;
    std::vector<u8key>   u8values() const;
    std::vector<u16key> u16values() const;
    std::vector<u32key> u32values() const;
    
    //bidirectional iterator for the value-list
    using iterator = /* implementation defined */;
    using const_iterator = iterator;
    
    iterator begin() const;
    iterator end() const;
}
    
}
```

### std::env::variable

```cpp
namespace std::env
{

class variable
{
public:
    using value_type = /* implementation defined, probably char on posix, wchar_t on windows */ ;
    using key_type      = basic_string<value_type, key_traits<value_type>>;
    using key_view_type = basic_string_view<value_type, key_traits<value_type>>;

    using string_type      = basic_string<value_type, key_traits<value_type>>;
    using string_view_type = basic_string_view<value_type, key_traits<value_type>>;
    
    //construct it 
    variable() noexcept;
    variable(const variable &) noexcept;
    variable(const variable_view& other) noexcept = default;
    variable(const value_type* s, size_type count);
    variable(const value_type* s);

    template<class It, class End>
    variable(It first, End last);

    template< class Source >
	variable( const Source& source, const std::locale& loc);
	template< class It, class End >
	variable( It first, End last, const std::locale& loc);
    
    //construct a variable from a key & values
    template<class ValueSource>
    constexpr variable(key_view_type key, const ValueSource & value);
    
    template<class ValueSource>
    constexpr variable(key_view_type key, const ValueSource & value, const std::locale& loc);

    template< class It, class End >
    constexpr variable(key_view_type key, It valueFirst, End valueLast);

    template< class It, class End >
    constexpr variable(key_view_type key, It valueFirst, End valueLast, const std::locale& loc);

    
    
    constexpr variable_view& operator=( const variable_view& view ) noexcept = default;

    variable& assign( string_type&& source );
    template< class Source >
    variable& assign( const Source& source );
	
    template< class InputIt >
    variable& assign( InputIt first, InputIt last );
    
    //get the underlying data
    const value_type* c_str() const noexcept;
    const string& native() const noexcept;
    operator string_type() const;
    
	//get the entire value as a string
    template< class CharT, class Traits = char_traits<CharT>, class Alloc = allocator<CharT> >
	basic_string<CharT,Traits,Alloc> string( const Alloc& a = Alloc() ) const;
	string       string() const;
	wstring     wstring() const;
	u16string u16string() const;
	u32string u32string() const;
	u8string   u8string() const;
    
    //get the key as view
    key_view_type native_key() const; 

    //get the key as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    basic_string<CharT,Traits,Alloc> key_string( const Alloc& a = Alloc() ) const;
    key       key_string() const;
    wkey     wkey_string() const;
    u8key   u8key_string() const;
    u16key u16key_string() const;
    u32key u32key_string() const;

    //get the value as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    basic_string<CharT,Traits,Alloc> value( const Alloc& a = Alloc() ) const;
    key       value() const;
    wkey     wvalue() const;
    u8key   u8value() const;
    u16key u16value() const;
    u32key u32value() const;

    //get the value as a string
    template< class CharT, class Traits = key_traits<CharT>, class Alloc = allocator<CharT> >
    std::vector<basic_string<CharT,Traits,Alloc>> values( const Alloc& a = Alloc() ) const;
    std::vector<key>       values() const;
    std::vector<wkey>     wvalues() const;
    std::vector<u8key>   u8values() const;
    std::vector<u16key> u16values() const;
    std::vector<u32key> u32values() const;
    
    //bidirectional iterator for the value-list
    using iterator = /* implementation defined */;
    using const_iterator = iterator;
    
    iterator begin() const;
    iterator end() const;
}
    
}
```

### std::env::get

Since there is no portable to way to obtain a pointer to the environment the return type of the main environment reference shall be implementation defined. It has to fulfill the following concept & avoid memory leaks:

```cpp
namespace std::env {
	bidirectional_range<variable_view> get();
} 
```

### get, set, unset

```cpp
namespace std::env {
    //get a single variable
    template<typename Char>  
    variable get(basic_key_view<Char>);
    template<typename Char>  
    variable get(basic_key_view<Char>, const std::locale &);

    //Set a variable
    void set(variable_view);
    
    //Reset a variablet
    template<typename Char>  
    variable unset(basic_key_view<Char>);
    template<typename Char>  
    variable unset(basic_key_view<Char>, const std::locale &);
}
```

## Usage examples

```cpp
std::filesystem::path search_path(std::string_view name) 
{
    auto paths = std::env::get("PATH");
    auto itr = std::ranges::find(paths,
                                [&](auto st) { return std::filesystem::is_regular_file(st / std::filesystem::path(name);});
    if (itr != paths.end())
        return *itr;
	else
        return {};
}
```

```cpp
std::vector<std::env::variable> pick_from_env(std::vector<std::env::key_view> keys)
{
    auto filtered = std::env::get() | std::views::filter(
        [&](auto key){
           return std::ranges::find(keys, key) != keys.end();
        });
    
    return {std::begin(filtered), std::end(filtered)};
}
```

```cpp
void add_to_path(std::filesystem::path pt)
{
    auto values = std::env::get("PATH").values();
    values.push_back(pt.native_string());
    std::env::set({"PATH", values});
}
```

