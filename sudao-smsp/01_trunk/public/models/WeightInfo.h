#ifndef __MODEL_WEIGHT_INFO_H__
#define __MODEL_WEIGHT_INFO_H__

#include <string>
#include <vector>
#include <algorithm>

#include <iostream>
#include <sstream>

#include "boost/shared_ptr.hpp"

namespace models
{


template<typename T>
class WeightInfo ;

template<typename T>
class WeightInfoGroup;

template<typename T>
class WeightCalculatorBase ;

template<typename T>
std::ostream& operator<<(std::ostream& os, WeightInfo<T>& obj) 
{
    //TODO::如果obj.m_data没有重载operator<< ?
    os << obj.m_dataName << ":[" << obj.m_data  << "]"
        << " config_weight:[" << obj.m_iWeight << "]"
        << " current_weight:[" << obj.m_iCurrentWeight << "]"
        << " effective_weight:[" << obj.m_iEffectiveWeight << "]";

    return os;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/*
 *   权重信息类
 */
template<typename T>
class WeightInfo 
{
    friend class WeightInfoGroup<T>;
    friend std::ostream& operator<< <> (std::ostream& os, WeightInfo<T>& obj) ;
    public:
        WeightInfo(const T& data, int iWeight);
        WeightInfo& operator=(const WeightInfo<T>& other);
        WeightInfo(const WeightInfo<T>& other);
        T getData() const;
        T& getData();

        int getWeight() const;
        int getCurrentWeight() const;
        int getEffectiveWeight() const;
        void setWeight(int w);
        void setCurrentWeight(int cw);
        void setEffectiveWeight(int ew);

        virtual ~WeightInfo(){}

    public:
        // 实际数据
        T m_data;
        std::string m_dataName;

    public:
        int m_iWeight;           // 配置权重
        int64_t m_iCurrentWeight;     // 动态变化的当前权重
        int m_iEffectiveWeight;   // 有效权重
};

/////////////////////////////////////////////////////////////////////////////////////////////
 /*
  *  包含多个权重信息的组类
  *  注意，它继承的vector里存的是指针，可以存放WeightInfo的实现类的指针
  */
template<typename T>
class WeightInfoGroup : public std::vector<boost::shared_ptr<WeightInfo<T> > >{
    public:
        typedef boost::shared_ptr<WeightInfo<T> > weightinfo_ptr_t;

    public:
        WeightInfoGroup();
        virtual ~WeightInfoGroup(){ }

        /*
         * 权重排序函数
         * Params:
         * @calculator: 用于实现业务相关的计算，如有效权重的更新
         * @selectMayFail: bool参数，如为true则表明此时权重最大的未必是业务正确的，则算法中不做更新处理
         */
        bool sortDataByWeight(WeightCalculatorBase<T>& calculator, bool selectMayFail);

        /*
         * 与sortDataByWeight()配合使用。在真正选定元素后调用此函数更新其CurrentWeight值
         */
        bool updateSelectedCurrentWeight(typename WeightInfoGroup::iterator iter);

        /*
         * 往权重组中添加新的权重信息
         */
        void addWeightInfo(const T& data, int iWeight);
        void addWeightInfo(weightinfo_ptr_t);

        /*
         * 返回通道组里通道配置的总权重和
         */
        int getTotalConfigWeight() const;

        void dumpAllWeightInfo(std::string& strDump, const std::string& strSplit = "\n");

    public:

        /* 组内排序函数  */
        // 按CurrentWeight逆序
        class SortByCurrWeight {
            public:
                bool operator()(const weightinfo_ptr_t& l, const weightinfo_ptr_t& r) {
                    return l->m_iCurrentWeight > r->m_iCurrentWeight;
                }
        };
        // 按EffectiveWeight逆序
        class SortByEffWeight {
            public:
                bool operator()(const weightinfo_ptr_t& l, const weightinfo_ptr_t& r) {
                    return l->m_iEffectiveWeight > r->m_iEffectiveWeight;
                }
        };

    private:
        // 总有效权重. 每次排序时更新
        int m_iTotalEffectWeight;

        // 总配置权重。每添加一个weightInfo就增加
        int m_iTotalConfigWeight;
};


/*
 * 权重组的排序函数 sortDataByWeight() 使用的计算类
 */
template<typename T>
class WeightCalculatorBase 
{
    public:
#define ONE_WAN 10000

        //  初始化计算需要的参数
        virtual bool init();
        // 实际的计算过程 
        virtual bool updateEffectiveWeight(typename WeightInfoGroup<T>::weightinfo_ptr_t weightInfoPtr);
};


/////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
WeightInfo<T>::WeightInfo(const T& data, int iWeight)
    : m_data(data),
    m_dataName("data"),
    m_iWeight(iWeight),
    m_iCurrentWeight(0),
    m_iEffectiveWeight(iWeight)
{
}

template<typename T>
WeightInfo<T>::WeightInfo(const WeightInfo<T>& other)
    : m_data(other.m_data),
    m_dataName(other.m_dataName),
    m_iWeight(other.m_iWeight),
    m_iCurrentWeight(other.m_iCurrentWeight),
    m_iEffectiveWeight(other.m_iEffectiveWeight)
{
}

template<typename T>
WeightInfo<T>& WeightInfo<T>::operator=(const WeightInfo<T>& other)
{
    // m_data要正确赋值，则T要实现operator=(const T&)
    this->m_data = other.m_data; 
    this->m_dataName = other.m_dataName; 
    this->m_iWeight = other.m_iWeight; 
    this->m_iCurrentWeight = other.m_iCurrentWeight; 
    this->m_iEffectiveWeight = other.m_iEffectiveWeight; 

    return *this;
}

template<typename T>
T WeightInfo<T>::getData() const
{
    return m_data;
}

template<typename T>
T& WeightInfo<T>::getData() 
{
    return m_data;
}

template<typename T>
int WeightInfo<T>::getWeight() const
{
    return m_iWeight;
}
template<typename T>
void WeightInfo<T>::setWeight(int w) 
{
    this->m_iWeight = w;
}

template<typename T>
int WeightInfo<T>::getCurrentWeight() const
{
    return m_iCurrentWeight;
}
template<typename T>
void WeightInfo<T>::setCurrentWeight(int cw) 
{
    this->m_iCurrentWeight = cw;
}

template<typename T>
int WeightInfo<T>::getEffectiveWeight() const
{
    return m_iEffectiveWeight;
}
template<typename T>
void WeightInfo<T>::setEffectiveWeight(int ew) 
{
    this->m_iEffectiveWeight = ew;
}

/////////////////////////////////////// WeightInfoGroup<T> ////////////////////////////////////////
template<class T>
WeightInfoGroup<T>::WeightInfoGroup()
    : m_iTotalConfigWeight(0)
{
}

template<class T>
bool WeightInfoGroup<T>::updateSelectedCurrentWeight(typename WeightInfoGroup<T>::iterator iter)
{
    // 为空或只有一个，则配合sortDataByWeight，不做CurrentWeight更新
    if (this->size() == 0
            || this->size() == 1) {
        return true;
    }

    (*iter)->m_iCurrentWeight -= m_iTotalEffectWeight;

    return true;
}

template<class T>
void WeightInfoGroup<T>::addWeightInfo(weightinfo_ptr_t ptrNewWeightInfo)
{
    this->push_back(ptrNewWeightInfo);
    m_iTotalConfigWeight += ptrNewWeightInfo->getWeight();
}

template<class T>
void WeightInfoGroup<T>::addWeightInfo(const T& data, int iWeight)
{
    this->push_back(weightinfo_ptr_t(new WeightInfo<T>(data, iWeight)));
    m_iTotalConfigWeight += iWeight;
}

template<class T>
int WeightInfoGroup<T>::getTotalConfigWeight() const
{
    return m_iTotalConfigWeight;
}

template<class T>
bool WeightInfoGroup<T>::sortDataByWeight(WeightCalculatorBase<T>& calculator, bool selectMayFail)
{
    // 为空或只有一个，则没必要做权重选择
    if (this->size() == 0
            || this->size() == 1) {
        return true;
    }

    m_iTotalEffectWeight = 0;

    typename WeightInfoGroup::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter) 
    {

        // 由计算类实现有效权重的计算过程
        if (calculator.updateEffectiveWeight(*iter) == false)
        {
            return false;
        }

        // 以有效权重更新当前权重
        (*iter)->m_iCurrentWeight += (*iter)->m_iEffectiveWeight;

        m_iTotalEffectWeight += (*iter)->m_iEffectiveWeight;
    }

    // 按当前权重排序,高的在前面
    std::sort(this->begin(), this->end(), SortByCurrWeight());

    // 如果选出的结果可能不符合业务条件，则不更新权重最高的选项的currentWeight
    // 而由调用者调用updateSelectedCurrentWeight()来主动更新
    if (!selectMayFail)
    {
        iter = this->begin();
        // 权重最高的减去total，以便下次选择时给其它候选者机会
        (*iter)->m_iCurrentWeight -= m_iTotalEffectWeight;
    }

    return true;
}

template<class T>
void WeightInfoGroup<T>::dumpAllWeightInfo(std::string& strDump, const std::string& strSplit)
{
    std::ostringstream oss;
    typename WeightInfoGroup::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter) 
    {  
        oss << **iter << strSplit;
    }
    strDump = oss.str();
}

//////////////////////////////////////
template<typename T>
bool WeightCalculatorBase<T>::init()
{
    return true;
}

template<typename T>
bool WeightCalculatorBase<T>::updateEffectiveWeight(typename WeightInfoGroup<T>::weightinfo_ptr_t weightInfoPtr)
{
    weightInfoPtr->m_iEffectiveWeight = weightInfoPtr->m_iWeight;
    return true;
}

}

#endif
