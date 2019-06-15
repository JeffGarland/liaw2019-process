#include <process>
#include <pstream>
#include <executor>
#include <buffer>
#include <string>
#include <iostream>

int main()
{
    std::net::executor exec;
    std::net::streambuf buffer;
  
    std::async_pipe ap(exec);

    std::process p(
        std::this_process::environment::find_executable("test.exe"),
        std::process_io(ap, {}, {});
        

    auto read_future = std::net::async_read(ap, buffer, std::use_future_t());
    auto exec_future = p.async_wait(exec, std::use_future_t());

    exec.context().run();
    
    std::cout << "Read " << read_future.get() << bytes << std::endl;
    
    return exec_future.get();
}
