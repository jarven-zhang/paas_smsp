#ifndef UNIVERSALCHANNEL_H_
#define UNIVERSALCHANNEL_H_
#include "StateReport.h"
#include <vector>
#include <string>
#include "Channellib.h"
#include "boost/regex.hpp"
#include "boost/multi_array.hpp"

using namespace std;  

#define	XML_ELEMENT_ENCAP_FMT					"encap_fmt"
#define	XML_ELEMENT_RT_MO_SEGMENT_TAG			"rt_mo_segment_tag"
#define	XML_ELEMENT_MT_RSP_FIELD_PATTERN		"mt_rsp_field_pattern"
#define	XML_ELEMENT_RT_FIELD_PATTERN			"rt_field_pattern"
#define	XML_ELEMENT_MO_FIELD_PATTERN			"mo_field_pattern"


/* 
*	Triplet tuple implement 
*/
#ifdef __GXX_EXPERIMENTAL_CXX0X__
#if 0
template <typename... Rest> struct Triplet;
template <> struct Triplet<> {};

template <typename First, typename ... Rest>
struct Triplet<First, Rest...> : public Triplet<Rest...> {
    Triplet() : value() {}
    Triplet(First &&first, Rest&&... rest)
        : value(std::forward<First>(first))
        , Triplet<Rest...>(std::forward<Rest>(rest)...)
    {}
    First value;
};

template <size_t N, typename TP> struct Tuple_Element;

template <typename T, typename ... Rest>
struct Tuple_Element<0, Triplet<T, Rest...>> {
    typedef T type;
    typedef Triplet<T, Rest...> TPType;
};

template <size_t N, typename T, typename ... Rest>
struct Tuple_Element<N, Triplet<T, Rest...>>
    : public Tuple_Element<N - 1, Triplet<Rest...>>
{};

template <size_t N, typename ... Rest>
typename Tuple_Element<N, Triplet<Rest...>>::type& get(Triplet<Rest...> &tp) {
    typedef typename Tuple_Element<N, Triplet<Rest...>>::TPType type;
    return ((type &)tp).value;
} 
#endif
#else
#if 1
struct VoidType;
  
template <typename T1, typename T2, typename T3>
struct Triplet;

template <>
struct Triplet<VoidType, VoidType, VoidType> {
};

template <typename T1>
struct Triplet<T1, VoidType, VoidType> {
    Triplet()
    {}
  
    Triplet(const T1 &v1)
        : value(v1)
    {}
  
    T1 value;
};

template <typename T1, typename T2>
struct Triplet<T1, T2, VoidType>
    : public Triplet<T2, VoidType, VoidType>
{
    Triplet()
    {}

    Triplet(const T1 &v1, const T2 &v2)
        : value(v1)
        , Triplet<T2, VoidType, VoidType>(v2)
    {}

    T1 value;
};
  
template <typename T1 = VoidType, typename T2 = VoidType, typename T3 = VoidType>
struct Triplet
    : public Triplet<T2, T3, VoidType>{
    	Triplet(const T1 &v1, const T2 &v2, const T3 &v3)
        : value(v1), Triplet<T2, T3, VoidType>(v2, v3)
    	{}
  	
    T1 value;
};

template <size_t N, typename T1, typename T2, typename T3>
struct Tuple_Element;

template <>
struct Tuple_Element<0, VoidType, VoidType, VoidType> {
};

template <typename T1, typename T2, typename T3>
struct Tuple_Element<0, T1, T2, T3> {
    typedef T1 type;
    typedef Triplet<T1, T2, T3> TPType;
};

template <size_t N, typename T1, typename T2, typename T3>
struct Tuple_Element
    : Tuple_Element<N - 1, T2, T3, VoidType>{
	};
  
template <size_t N, typename T1, typename T2, typename T3>
typename Tuple_Element<N, T1, T2, T3>::type& get(Triplet<T1, T2, T3> &tp) {
    typedef typename Tuple_Element<N, T1, T2, T3>::TPType type;
    return ((type &)tp).value;
}
#endif
#endif
	
/* protocol option */
enum EncapsulationFormat{
	ENCAP_FORMAT_JSON = 0,
	ENCAP_FORMAT_XML,
	ENCAP_FORMAT_URLENCODED,
	ENCAP_FORMAT_CUSTOM
};
	
/* essential field */
enum KeyField{
	FIELD_SMSID = 0,
	FIELD_STATUS,
	FIELD_EXTCODE,
	FIELD_DESTCODE,	
	FIELD_MOBILES,
	FIELD_MSG
	//,FIELD_ERRMSG
};

/* <field name,  field regex pattern, spesfic check key pattern  */
typedef Triplet<std::string, std::string, std::string> 		field_triplet;
typedef std::map<int, field_triplet>						field_pattern;
typedef std::map<int, std::string>							delimiter_config;

namespace smsp {

class UniversalChannel : public Channellib{
public:
	UniversalChannel();
	virtual ~UniversalChannel();
	bool Init(const std::string config_path);
	bool parseSend(std::string respone, std::string& smsid, std::string& status, std::string& strReason);
	bool parseReport(std::string respone, std::vector<smsp::StateReport>& reportRes, std::vector<smsp::UpstreamSms>& upsmsRes, std::string& strResponse);
	virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead);
	//virtual int adaptEachChannel(smsp::SmsContext* sms,std::string* url ,std::string* data, std::vector<std::string>* vhead)=0;
	//int replace_all_distinct(string& str, const string& old_value, const string& new_value);
		
private:
	/* 	
	* 	Encapsulation
	*/		
	int mt_rsp_encap_fmt;
	int rt_mo_encap_fmt;
	
	/* 	
	*	the tag of report/upstream set 
	*/	
	std::string rt_segment_tag;
	std::string mo_segment_tag;
		
	/*	
	*	field pattern
	*/
	field_pattern	mt_rsp_field_pattern;
	field_pattern	rt_field_pattern;
	field_pattern	mo_field_pattern;
	
	/* 
	*	delimiter defination
	*/
	delimiter_config m_deli_config;
	
};

} /* namespace smsp */
#endif //// WTPARSER_H_ 
