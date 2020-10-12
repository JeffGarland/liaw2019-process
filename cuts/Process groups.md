# Process groups

A process group is a set of processes that can be managed together, i.e. terminated, waited for or queried for it's status.

The process group is based on operating system level features (job objects on windows and process groups on posix). This means that the operating system can also be leveraged to manage the subprocesses as a single unit.

It can be useful for worker pools or combination of different processes which perform a single task into a single handle.

## Usage example

A combination of several workers.

```cpp
std::process_group grp;
grp.emplace("server-worker", {}); //start some worker
grp.emplace("server-gateway", {}); // start a gateway that talks to the worker
grp.wait(); //wait for both
```

A worker pool.

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

    process_group(native_handle_type);
    
    // The destructor terminates all processes in the group
    ~process_group();
    
    // Check if at least one process of the group is running
    bool running() const;
    
    // Process management functions
    // Emplace a process into the group, i.e. launch it attached to the group
    template<ranges::InputRange Args, ProcessInitializer... Inits>
   		requires ConvertibleTo<iter_value_t<Args>, string>
    pid_type emplace(const filesystem::path& exe, const Args& args, Inits&&...inits);

    // Emplace a process into the group, i.e. launch it attached to the group with
    // a custom process launcher
    template<ranges::InputRange Args, ProcessInitializer... Inits, ProcessLauncher Launcher>
	    requires convertible_to<iter_value_t<Args>, string>
    		pid_type emplace(const filesystem::path& exe,
					        const Args& args, Inits&&...inits,
						    Launcher&& launcher);
    
    // Attach an existing process to the group. The process object will be empty afterwards.
    // This functions might fail if there are setting at startup that need to be set.
    pid_type attach(process&& proc);
    
    // Detach a process group -- let it run after this handle destructs
    void detach();

    // Terminate the child processes (child processes will unconditionally and
    // immediately exit). Implemented with SIGKILL on POSIX and TerminateProcess
    // on Windows
    void terminate();
    
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
    };
}
```

## Reference implementation

The `process_group` has been partially implemented with the exception of `async_wait` and `async_wait_one`, mainly for time constraints. The mechanics of overlapped, which are used to wait (`signal` and `overlapped`) can most definitely be implemented using `asio`.