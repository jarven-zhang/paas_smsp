#ifndef SYSKEYWORDCLASSIFY_H_
#define SYSKEYWORDCLASSIFY_H_

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <stdlib.h>
#include "searchTree.h"
#include "keywordClassify.h"

class sysKeywordClassify :public KeywordClassify
{
public:
	sysKeywordClassify(string name);
	virtual ~sysKeywordClassify();
	void initParam();

};

















#endif
















