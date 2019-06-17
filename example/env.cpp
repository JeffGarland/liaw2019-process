#include <process>
#include <filesystem>
#include <environment>
#include <string>
#include <cctype>

template<typename String>
std::string to_upper(String && str)
{
    std::string res = std::forward<String>(str);
    for (auto & c : res)
        c = std::toupper(c);

    return res;
}

int main()
{
    std::environment my_env = std::this_process::environment::native_environment();

    std::filesystem::path additional_path = "/foo";

    keys = my_env.keys();
    auto path_key = std::find_if(keys.begin(), keys.end(), [] (auto & val) {return to_upper<std::string>(val) == "PATH";});

    
    const paths = my_env.get(path_key).as_vector();

    paths.push_back(additional_path);
    
    my_env.set(path_key, paths);
    std::process proc(my_env.find_executable("test.exe"), my_env);
    proc.wait();
    
    return proc.exit_code();
}
