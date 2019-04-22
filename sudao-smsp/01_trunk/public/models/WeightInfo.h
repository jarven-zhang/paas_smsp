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
    //TODO::���obj.m_dataû������operator<< ?
    os << obj.m_dataName << ":[" << obj.m_data  << "]"
        << " config_weight:[" << obj.m_iWeight << "]"
        << " current_weight:[" << obj.m_iCurrentWeight << "]"
        << " effective_weight:[" << obj.m_iEffectiveWeight << "]";

    return os;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/*
 *   Ȩ����Ϣ��
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
        // ʵ������
        T m_data;
        std::string m_dataName;

    public:
        int m_iWeight;           // ����Ȩ��
        int64_t m_iCurrentWeight;     // ��̬�仯�ĵ�ǰȨ��
        int m_iEffectiveWeight;   // ��ЧȨ��
};

/////////////////////////////////////////////////////////////////////////////////////////////
 /*
  *  �������Ȩ����Ϣ������
  *  ע�⣬���̳е�vector������ָ�룬���Դ��WeightInfo��ʵ�����ָ��
  */
template<typename T>
class WeightInfoGroup : public std::vector<boost::shared_ptr<WeightInfo<T> > >{
    public:
        typedef boost::shared_ptr<WeightInfo<T> > weightinfo_ptr_t;

    public:
        WeightInfoGroup();
        virtual ~WeightInfoGroup(){ }

        /*
         * Ȩ��������
         * Params:
         * @calculator: ����ʵ��ҵ����صļ��㣬����ЧȨ�صĸ���
         * @selectMayFail: bool��������Ϊtrue�������ʱȨ������δ����ҵ����ȷ�ģ����㷨�в������´���
         */
        bool sortDataByWeight(WeightCalculatorBase<T>& calculator, bool selectMayFail);

        /*
         * ��sortDataByWeight()���ʹ�á�������ѡ��Ԫ�غ���ô˺���������CurrentWeightֵ
         */
        bool updateSelectedCurrentWeight(typename WeightInfoGroup::iterator iter);

        /*
         * ��Ȩ����������µ�Ȩ����Ϣ
         */
        void addWeightInfo(const T& data, int iWeight);
        void addWeightInfo(weightinfo_ptr_t);

        /*
         * ����ͨ������ͨ�����õ���Ȩ�غ�
         */
        int getTotalConfigWeight() const;

        void dumpAllWeightInfo(std::string& strDump, const std::string& strSplit = "\n");

    public:

        /* ����������  */
        // ��CurrentWeight����
        class SortByCurrWeight {
            public:
                bool operator()(const weightinfo_ptr_t& l, const weightinfo_ptr_t& r) {
                    return l->m_iCurrentWeight > r->m_iCurrentWeight;
                }
        };
        // ��EffectiveWeight����
        class SortByEffWeight {
            public:
                bool operator()(const weightinfo_ptr_t& l, const weightinfo_ptr_t& r) {
                    return l->m_iEffectiveWeight > r->m_iEffectiveWeight;
                }
        };

    private:
        // ����ЧȨ��. ÿ������ʱ����
        int m_iTotalEffectWeight;

        // ������Ȩ�ء�ÿ���һ��weightInfo������
        int m_iTotalConfigWeight;
};


/*
 * Ȩ����������� sortDataByWeight() ʹ�õļ�����
 */
template<typename T>
class WeightCalculatorBase 
{
    public:
#define ONE_WAN 10000

        //  ��ʼ��������Ҫ�Ĳ���
        virtual bool init();
        // ʵ�ʵļ������ 
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
    // m_dataҪ��ȷ��ֵ����TҪʵ��operator=(const T&)
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
    // Ϊ�ջ�ֻ��һ���������sortDataByWeight������CurrentWeight����
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
    // Ϊ�ջ�ֻ��һ������û��Ҫ��Ȩ��ѡ��
    if (this->size() == 0
            || this->size() == 1) {
        return true;
    }

    m_iTotalEffectWeight = 0;

    typename WeightInfoGroup::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter) 
    {

        // �ɼ�����ʵ����ЧȨ�صļ������
        if (calculator.updateEffectiveWeight(*iter) == false)
        {
            return false;
        }

        // ����ЧȨ�ظ��µ�ǰȨ��
        (*iter)->m_iCurrentWeight += (*iter)->m_iEffectiveWeight;

        m_iTotalEffectWeight += (*iter)->m_iEffectiveWeight;
    }

    // ����ǰȨ������,�ߵ���ǰ��
    std::sort(this->begin(), this->end(), SortByCurrWeight());

    // ���ѡ���Ľ�����ܲ�����ҵ���������򲻸���Ȩ����ߵ�ѡ���currentWeight
    // ���ɵ����ߵ���updateSelectedCurrentWeight()����������
    if (!selectMayFail)
    {
        iter = this->begin();
        // Ȩ����ߵļ�ȥtotal���Ա��´�ѡ��ʱ��������ѡ�߻���
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
