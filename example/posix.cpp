#include <process>

int main()
{
    struct chroot
    {
        const char * root_to;
        
        template<ProcessLauncher Launcher>
        void on_exec_setup(Launcher&)
        { 
            ::chroot(root_to); 
        };
        
    };
    
    std::process proc("./test", {}, chroot("/new/root/directory/"));
    proc.wait();
    
    return proc.exit_code();
}
