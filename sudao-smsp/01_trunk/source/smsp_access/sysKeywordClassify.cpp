#include "sysKeywordClassify.h"
#include "keywordClassify.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <stdlib.h>
#include "searchTree.h"
#include <iostream>
#include <algorithm>
#include "main.h"
#include "Comm.h"



sysKeywordClassify::sysKeywordClassify(string name):KeywordClassify(name)
{
	
}


sysKeywordClassify::~sysKeywordClassify()
{


}


void sysKeywordClassify::initParam()
{

	map<UInt32,list<string> > mapKeywordList;
	mapKeywordList.clear();
    g_pRuleLoadThread->getSysKeywordMap(mapKeywordList);
	initKeywordSearchTree(mapKeywordList);
	
    g_pRuleLoadThread->getSysKgroupRefCategoryMap(m_keywordGrpRefCategoryMap);
    g_pRuleLoadThread->getSysClientGroupMap(m_clientGrpRefKeywordGrpMap, m_uDefaultGroupId);
    g_pRuleLoadThread->getSysCGroupRefClientMap(m_clientGrpRefClientMap);
	
	return;
}














