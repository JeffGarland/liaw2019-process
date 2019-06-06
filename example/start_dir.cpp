#include <process>
#include <filesystem>

int main()
{
    std::filesystem::path exe = "test.exe";
    std::process proc(
        std::filesystem::absolute(exe),
        {},
        std::process_start_dir="../foo"
    );
    
    proc.wait();
    return proc.exit_code();
}
