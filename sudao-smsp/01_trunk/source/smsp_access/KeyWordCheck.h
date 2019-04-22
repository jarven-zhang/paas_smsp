#ifndef KEYWORDCHECK_H_
#define KEYWORDCHECK_H_
#include<string>
#include<vector>
#include <map>
#include <list>
#include <set>
#include <stdlib.h>
#include "searchTree.h"
#include "CommClass.h"


using namespace std;

class KeyWordCheck
{
public:
    KeyWordCheck(Int32 iKeywordCovRegular=0);
    virtual ~KeyWordCheck();
	void initParam();
	void initKeyWordRegular(Int32 iKeywordCovRegular);
    void initAuditKeyWordSearchTree(map<UInt32,list<string> >& mapSetIn);
	void initIgnoreAuditKeyWord(map<string,set<string> >& mapKeywordIgnore);
	bool GetAuditKeyWordCategoryIdSet(UInt32 uKGroupId,set<UInt32>& CategoryIdSet);
    UInt32 GetAuditKGroupId(string strClient);
	bool KeyWordSearch(searchTree* pTree,string& strContent,string& strSign,string& strOut,const string& clientid="");
	bool AuditKeywordCheck(string strClientId,string strContent,string strSign,string& strOut, string& strIgnoreMapKey);
    

	std::map<string,UInt32>             m_mapAuditCGroupRefClient;//t_sms_audit_cgroup_ref_client
	map<UInt32, UInt32>					m_mapAuditClientGroup; //t_sms_audit_client_group
	std::map<UInt32,set<UInt32> >       m_mapAuditKgroupRefCategory;//t_sms_audit_kgroup_ref_category
	std::map<UInt32,searchTree*>        m_mapAuditKeyWord;//t_sms_audit_keyword_list
	UInt32                              m_uDefaultGroupId;  //Ä¬ÈÏÉóºË¹Ø¼ü×é
	Int32                               m_iAuditKeywordCovRegular;
	map<string,set<string> >            m_mapKeywordIgnore;
};

#endif /* CHANNELSELECT_H_ */
