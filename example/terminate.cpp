#include <process>

int main()
{
    std::process p(std::this_process::environment::find_executable("test.exe"));
    c.terminate();
}
