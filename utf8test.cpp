#include <fstream>
#include <sstream>
#include "utf8.h"

using namespace std;

int main()
{
    std::ifstream t("reference_text.utf8.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();
    string text = buffer.str();
    std::string temp = text.substr(0, 2000);
    std::string out;
    utf8::replace_invalid(temp.begin(), temp.end(), back_inserter(out));
}
