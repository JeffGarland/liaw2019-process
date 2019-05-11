# Design Proposal

# Concepts

For extensibility, we have two core concepts

## ProcessLauncher

The process Launcher is the class that starts the actual process: 

```cpp
template<typename T>
concept bool ProcessLauncher() {
    return requires(T launcher) {
        {launcher.set_error(std::error_code())} -> void; //so initializers can post internal errors
        {launcher()} -> process; //refine that so check tha parameter list, good enough for now.
    };
}
```

The reason to make this a concept is so replacement is possible. The use case in mind is `vfork` -> vfork is built into the posix-executor (renamed to launcher here) in boost-process and can be activated by a parameter. This is ugly and should not be in the standard; yet vfork as explicitly requested, and the possibility I see is to allow users to provide their own launcher OR the vendor to provide a `std::ext::vfork_launcher`. 

Having this customization point does however allow more things, like security or virtualization.

## ProcessInitializer

The process initializer is a class that does some work to set up a process.

```cpp
template<typename Init, typename Launcher = process_launcher>
concept bool ProcessInitializer = requires(Init initializer, Launcher launcher) {
        {initializer.on_setup(launcher)}   -> void;
        {initializer.on_success(launcher)} -> void;
        {initializer.on_error(launcher, std::error_code())} -> void;
    };
```

A vendor implementation may provide more customization points, such as handling fork etc. Note however, that it is required to invoke the three given one from the launching process.

## ProcessStream

Anything that contains native_handle (HANDLE* on windows & int on linux) that can be used as a stream. Includes TCP Ports btw.

# Launching a process

## Core function

Launching a process is done by default through the destructor of `std::process`. There are two modes to launch a process in theory: providing a `path` & arguments, OR to provide a command string. The latter will be interpreted by a shell (cmd.exe or /bin/sh). This is considered unsafe (because shell injection), and `std::system` does exactly that at the moment. Point of discussion: does that make sense to provide? Or should be move to deprecate `std::system` since we then would have a process library?

In effect the two are the same in boost::process:

```cpp
system("foo && bar > file");
system("/bin/sh", {"/c", "foo && bar > file"});
```

I will leave out the cmd-syntax for brevity sake, so here's how launching would look:

```cpp
process::process(const std::filesystem::path &path, Container args, ProcessInitializer... init);
process::process(const std::filesystem::path &path, Container args, ProcessLauncher customLauncher, ProcessInitializer... init);
```

## Other functions

In boost.process there are more functions to launcha process. After further consideration I would move to remove them all, since they are easily done by just using the process object.

```cpp
system(...) => process(...).wait()
spawn(...) => process(...).detach()
async_system(..., Handler, io_context) = async_wait(process(...), Handler, io_context)
```

# Classes

## std::process

Largely the same as `boost::process::child`.

## std::process_group

Similar to `boost::process::group`, BUT not used in initialization (necessarily at least). There is however a slight difference between launching a process in attached mode or attaching it later, so here's the idea:

```cpp
//launch a process attached
process_group::emplace(const std::filesystem::path &path, Container args, ProcessInitializer... init); //returns either void or pid_t -> point of discussion
process_group::emplace(process::process(const std::filesystem::path &path, Container args, ProcessLauncher customLauncher, ProcessInitializer... init);

//attach an existing process
process_group::attach(process && p); //meaning p won't have a handle on the subprocess anymore

//detach a process who's pid I know
process_group::detach(pid_t pid) -> child; 
```

## Pipe & async_pipe & pstream

Similar to boost::process, I think. We should get Chris Kohlhoff involved on the issue though.

# Null-device

I would like to have a null-device, i.e. either a stream (or something similar) pointing to `/dev/null`. I mainly want it for IO redirection, but it's generic enough to be discussed.

Could be 
	- std::this_process::null
	- std::cnull
	- std::null_dev

# Asio

Besides async_pipe, I would like to have those two functions:

```
async_wait(process&,	   io_context, Handler); //Handler  = void(error_code, int) 
async_wait(process_group&, io_context, Handler); //Handler  = void(error_code) 
```

Could be member functions of process(_group)

# this_process

## environment

Class referring to the current process -> should have a `path` function, returning `vector<path>` AND a `find_exe` function. THe latter looks for an executable using `PATH` and (on windows) `PATHEXT`.

## handles

For limiting the handles we need to be able to iterate all the handles in a process.

```cpp
this_process::all_handles() -> vector<native_handle>;
```	

## stdio

Note that the streambuf of `cerr`, `cin` and `cout` can be changed without changing the actual `stdin`, `stdout` and `stderr`.

So wee need something like that:

```cpp
this_process::std_handles(); //custom class containing .in(), .out() & .err() 
```


## pwd/cwd

is given by `std::filesytem::current_path

# Initializers

Here with it's own namespace, not sure that's the right solution.

## IO

Group stdio, since I need to set all three (or none) on windows. Namespacing etc. up for debates.

```cpp
std::process_init::stdio(Stream in, Stream out, Stream err); 
std::process_init::stdio(decltype(std::this_process::std_handles()));
```

Usage:

```cpp
fstream some_filestream;
pipe some_pipe;

process(process_init::stdio(some_pipe, some_filestream, std::this_process::std_handles().err()); 

// OR

process_init::stdio pi = std::this_process::std_handles()
pi.err(some_pipe);

process p(..., pi);
```

Passing `nullptr` will close the stream.

## Environment:

```cpp
std::process_init::environment env = std::this_process::environment();
env["foo"] = "bar";

process p(..., env);

## StartDir

Inherits path

```cpp
std::process_init::start_in_dir(std::filesystem::path); 
std::process_init::start_in_dir(); //uses the current pwd.
```

```cpp
auto start_dir = std::process_init::start_in_dir(std::filesystem::path); 
process(..., start_dir);
```

```cpp
auto start_dir = std::process_init::start_in_dir(); 
start_dir = "/somewhere";
process(..., start_dir);
```
