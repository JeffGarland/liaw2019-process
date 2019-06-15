#include <process>

int main()
{
    std::process p(std::this_process::environment::find_executable("test.exe"), {"--foo", "/bar"});

    p.wait();
    
    return p.exit_code();
}
