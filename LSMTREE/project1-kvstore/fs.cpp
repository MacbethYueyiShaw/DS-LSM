#include <string>
#include <filesystem>
using namespace std;
namespace fs = std::filesystem;

int main()
{
	fs::path dirctory="./data/level";
	fs::create_directories(dirctory);
	return 0;
}
