#ifndef ECHOGEOMESHFIELD_H_
#define ECHOGEOMESHFIELD_H_

#include <vector>

namespace echo
{

namespace geo{

class MeshFieldBase
{
public:
    virtual ~MeshFieldBase() {}
    template<class T> const T& get() const; //to be implimented after Parameter
    template<class T, class U> void setValue(const U& rhs); //to be implimented after Parameter
};

template <typename T>
class MeshField : public MeshFieldBase
{
public:

    MeshField() {}

    MeshField(std::string & name_) {
        this->name = name_;
    }

    MeshField(const char * name_) {
        this->name = std::string(name_);
    }

    MeshField(const std::vector<T> & rhs) {
        this->data.resize(rhs.size());
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit = rhs.begin(), it = this->data.begin(); cit != rhs.end(); ++cit, ++it){
            *it = *cit;
        }
        name = std::string("default");
    }

    MeshField(const std::vector<T> & rhs, std::string & name_) {
        this->values.resize(rhs.size());
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit = rhs.begin(), it = this->data.begin(); cit != rhs.end(); ++cit, ++it){
            *it = *cit;
        }
        this->name = name_;
    }

    const T& get(unsigned int i) const {
        return this->values[i];
    }
    void setValue(unsigned int i, const T& rhs) {
        this->data[i]=rhs;
    }

    /// It would be maybe more elegant if this was private:
    std::vector<T> data;
    std::string name;

private:

};

//Here's the trick: dynamic_cast rather than virtual
template<class T> const T& MeshFieldBase::get() const
{
    return dynamic_cast<const MeshField<T>&>(*this).get();
}
template<class T, class U> void MeshFieldBase::setValue(const U& rhs)
{
    return dynamic_cast<MeshField<T>&>(*this).setValue(rhs);
}


} /// end namespace geo

} /// end namespace echo

#endif //ECHOGEOMESHFIELD_H_
