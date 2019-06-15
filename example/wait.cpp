#include <process>
#include <executor>
#include <environment>

int main()
{
    {
        std::process c(std::this_environment::find_executable("test.exe"), {});
        c.wait();
        auto exit_code = c.exit_code();
    }

    {
        std::executor::executor executor;

        std::process c(
            std::this_environment::find_executable("test.exe"),
            executor,
            bp::on_exit([&](int exit, const std::error_code& ec_in){})
        );

        executor.context().run();
    }
}
