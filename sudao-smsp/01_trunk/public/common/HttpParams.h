#ifndef HTTPPARAMS_H_
#define HTTPPARAMS_H_

#include <string>
#include <map>
#include <vector>

using namespace std;
namespace web
{
    class HttpParams
    {
    public:
        HttpParams();
        virtual ~HttpParams();
        void Parse(const string& query);
        string getValue(string key);
        map<string,string> _map;
    private:
        void splitEx(const string& src, string separate_character,vector<string>& strs);

    };
}
#endif /* HTTPPARAMS_H_ */
