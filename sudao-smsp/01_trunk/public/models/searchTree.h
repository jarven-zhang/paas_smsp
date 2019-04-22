#ifndef __SEARCHTREE__
#define __SEARCHTREE__

#include <string>
#include <list>
#include <set>
#include <map>
#include <iostream>
#include "platform.h"
using namespace std;

typedef struct searchNode
{
	searchNode(char c)
	{
		m_c = c;
		m_flag = false;
	}

	char m_c;
	bool m_flag;

	map<char,searchNode*> m_mapSet;
}searchNode;


class searchTree
{
public:
	searchTree();
	~searchTree();

	void initTree(list<string>& listSet);
	void InitIgnoreMap(map<string,set<string> >& mapKeywordIgnore);
	void DelTree();
	void DelSet();

	bool search(string& strIn,string& strOut,const string& strIgnoreMapKey = "");
	bool searchSign(string& strIn,string& strOut,const string& strIgnoreMapKey = "");
	bool searchIgnoreKeyword(const string& strDateIn,const string& strIgnoreMapKey);

private:

	void DelNode(map<char,searchNode*>& mapSet);

	map<char,searchNode*> m_mapSet;
	set<string> m_signSet;
	map<string,set<string> > m_mapKeywordIgnore;
	
};




#endif ///__SEARCHTREE__