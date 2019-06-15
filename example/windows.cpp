#include <process>
#include <iostream>

#include <windows.h>

int main()
{
    const auto exe = std::this_process::environment::find_executable("test.exe");
    struct start_fullscreeen
    {
        template<std::ProcessLauncher Launcher>
        void on_setup(Launcher & l)
        {
            l.startup_info.wShowWindow  = SHOW_WINDOW; // < implementation defined.
        }
    };

    struct show_window
    {
        template<std::ProcessLauncher Launcher>
        void on_setup(Launcher & l)
        {
            l.startup_info.dwFlags = STARTF_RUNFULLSCREEN; // < implementation defined.
        }
    };
        
    
    std::process proc1(exe, {}, show_window());
    std::process proc2(exe, {}, start_fullscreeen());
}
