#include <process>
#include <system_error>

int main()
{
    try 
    {
        std::process proc("does-not-exist", {});
    }
    catch (process_error & pe)
    {
        std::cout << "Process launch error for file: " << pe.path() << std::endl;
    }
}
