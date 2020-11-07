# Proposal to add process groups to the C++ standard library

# Revision History

## R0

* Initial Release

# Introduction

The current process proposal (P1750) has been reduced in the amount of proposed features, in order to make it a more managable addition to the standard. One of the features cut, are process groups which are the subject of this paper.

A process group is a set of processes that can be managed together, i.e. terminated, waited for or queried for it's status.

The process group is based on operating system level features (job objects on windows and process groups on posix). This means that the operating system can also be leveraged to manage the subprocesses as a single unit.

It's main usage is efficient organization of different processes, that perform a common task, into a single entity. This entity does not only exist in C++, but on the level of the operating system, so that process management tools by the OS reflect this grouping.

It is useful for worker pools or combination of different processes which perform a single task into a single handle.

## Usage example

The processes in the group should be emplaced with the same semantics as the process construction of a `process` as specified by P1750.

```cpp
std::process_group grp;
grp.emplace("server-worker", {}); //start some worker
grp.emplace("server-gateway", {}); // start a gateway that talks to the worker
grp.wait(); //wait for both
```

A `process_group` will be able to efficiently set up worker pools as shown by the following example.

```cpp
int proc_count = 4;
std::process_group grp;
for (auto i = 0; i < process_count;i++)
	std::clog << "Started worker with pid " << grp.emplace("worker", {}) << std::endl;
while (grp.running())
{
    auto [pid, exit_code] = grp.wait_one();
    std::clog << "Process " << pid << " stopped with exit code " << exit_code << std::endl;
    if (exit_code != 0)
    {
        std::clog << "Exited with error, restarting with pid " << grp.emplace("worker", {}) << std::endl;
    }
}
```

## Specified interface

Similar to a `std::process` a, `std::process_group` shall by default be in attached state. If the `process_group` object is destroyed while in attached state, it shall terminate all child processes in the group.

```cpp
namespace std {
class process_group {
public:
    
    // Provides access to underlying operating system facilities
    using native_handle_type = implementation-defined;
    native_handle_type native_handle() const;
    process_group();
    process_group(process_group&& lhs);
    process_group& operator=(process_group&& lhs);
    //Construct the process_group from a native_handle
    process_group(native_handle_type);
    
    // The destructor terminates all processes in the group
    ~process_group();
    
    // Check if at least one process of the group is running
    bool running() const;
    
    // Emplace a process into the group, i.e. launch it attached to the group with
    // a custom process launcher
    template<ranges::InputRange Args, ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
	    requires convertible_to<ranges::range_value_t<Args>, string>
    pid_type emplace(const filesystem::path& exe,
                     const Args& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    // Emplace a process into the group, i.e. launch it attached to the group with
    // a custom process launcher and a locale for the possible char conversion of args
    template<ranges::input_range Args, ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
	    requires convertible_to<ranges::range_value_t<Args>, string>
    pid_type emplace(const filesystem::path& exe,
                     const Args& args,  const std::locale& loc, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    
    /* The following 10 overloads are meant to enable text literals, e.g. 
    	my_group.emplace("exe", {"foo", "bar"}); 
    */
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::string_view>& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::string_view>& args, const std::locale& loc,
                     Inits&&...inits, Launcher&& launcher = default_process_launcher{});
    
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::wstring_view>& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::wstring_view>& args, const std::locale& loc,
                     Inits&&...inits, Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u8string_view >& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u8string_view >& args, const std::locale& loc,
                     Inits&&...inits, Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u16string_view>& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u16string_view>& args, const std::locale& loc,
                     Inits&&...inits, Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u32string_view>& args, Inits&&...inits,
                     Launcher&& launcher = default_process_launcher{});
    template<ProcessInitializer... Inits, ProcessLauncher Launcher = default_process_launcher>
    pid_type emplace(const filesystem::path& exe,
                     const initializer_list<std::u32string_view>& args, const std::locale& loc,
                     Inits&&...inits, Launcher&& launcher = default_process_launcher{});
    
    // Attach an existing process to the group. The process object will be empty afterwards.
    // This functions might fail if there are setting at startup that need to be set.
    pid_type attach(process&& proc);
    
    // Detach a process group -- let it run after this handle destructs
    void detach();
    // Terminate the child processes (child processes will unconditionally and
    // immediately exit). Implemented with SIGKILL on POSIX and TerminateJobObject
    // on Windows
    void terminate();
    
    //Check if a process with this pid is part of the group
    bool contains(pid_type pid) const;
    
    // Block until all processes exit
    void wait();
    
    // Block until one process exit. Return it's pid and exit_code
    std::pair<pid_type, int> wait_one();
        
    // The following is dependent on the networking TS. CompletionToken has
    //the signature (error_code) and waits for all processes to exit
    template<class CompletionToken>
    auto async_wait(net::Executor& ctx, CompletionToken&& token);
    
    // The following is dependent on the networking TS. CompletionToken has
    // the signature (error_code, pair<pid_type, int>) and waits for one process
    template<class CompletionToken>
    auto async_wait_one(net::Executor& ctx, CompletionToken&& token);
}
```

*Note that `process_group` can also be a type nested in `process` as `process::group`*