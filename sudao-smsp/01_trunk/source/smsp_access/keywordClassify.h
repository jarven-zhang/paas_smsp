#ifndef KEYWORDCLASSIFY_H_
#define KEYWORDCLASSIFY_H_

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <stdlib.h>
#include "searchTree.h"


using namespace std;

class KeywordClassify
{
public:
    KeywordClassify(string childName);
    virtual ~KeywordClassify();
    virtual void initParam();
    void initKeywordSearchTree(map<UInt32,list<string> >& mapSetIn);
    bool getKeywordCategoryIdSet(UInt32 uKGroupId,set<UInt32>& CategoryIdSet);
    UInt32 getKGroupId(string strClient);
    bool keywordSearch(searchTree* pTree,string& strContent,string& strSign,string& strOut);
    bool keywordCheck(string strClientId, const string &strContent, string strSign, string& strOut, int& keywordFilRegl, bool bIncludeChinese = true);
    void setClientGrpRefClient(map<string,UInt32>& clientGrpRefClientMap);
    void setCgrpRefKeywordGrp(map<UInt32,UInt32>& cgrpRefKeywordGrp, UInt32  defKgrpId);
    void setkeyGrpRefCategory(map<UInt32,set<UInt32> >& keywordGrpRefCategoryMap);
protected:
    map<UInt32,searchTree*>  m_keywordMap;  //categoryID + keyword
    map<UInt32,set<UInt32> >  m_keywordGrpRefCategoryMap;//keywordGrpID + categoryID
    map<UInt32,UInt32>        m_clientGrpRefKeywordGrpMap;//clientGrpID<->keywordGrpID
    UInt32                    m_uDefaultGroupId;//in t_sms_sys_client_group  kgrpid
    map<string,UInt32>        m_clientGrpRefClientMap;//clientid<->clientGrpID
private:
    string m_childName;

};

#endif
