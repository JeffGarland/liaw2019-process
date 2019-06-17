#include <process>
#include <pstream>

int main()
{
    std::ipstream pipe_stream;
    std::process c("/usr/bin/gcc" {"--version",} std::process_io({} ,pipe_stream, {}));

    std::string line;

    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty())
        std::cerr << line << std::endl;

    c.wait();
    return c.exit_code();
}
