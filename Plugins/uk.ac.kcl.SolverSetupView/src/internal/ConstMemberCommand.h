#pragma once

#include <itkCommand.h>

namespace crimson {

// A version of itk Command class which calls the callback function 
// with a const object regardless of whether const or non-const
// callback is requested.

template< typename T >
class ConstMemberCommand :public itk::Command
{
public:
    /** pointer to a member function that takes a Object* and the event */
    typedef  void (T::*TMemberFunctionPointer)(const itk::Object*);

    /** Standard class typedefs. */
    typedef ConstMemberCommand Self;
    typedef itk::SmartPointer< Self >  Pointer;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(ConstMemberCommand, Command);

    /**  Set the callback function along with the object that it will
    *  be invoked on. */
    void SetCallbackFunction(T *object,
        TMemberFunctionPointer memberFunction)
    {
        m_This = object;
        m_MemberFunction = memberFunction;
    }

    /**  Invoke the member function. */
    virtual void Execute(itk::Object * obj, const itk::EventObject &)
    {
        if (m_MemberFunction)
        {
            ((*m_This).*(m_MemberFunction))(obj);
        }
    }

    /**  Invoke the member function with a const object */
    virtual void Execute(const itk::Object *obj, const itk::EventObject &)
    {
        if (m_MemberFunction)
        {
            ((*m_This).*(m_MemberFunction))(obj);
        }
    }

protected:
    T *                    m_This;
    TMemberFunctionPointer m_MemberFunction;
    ConstMemberCommand() :m_MemberFunction(0) {}
    virtual ~ConstMemberCommand() {}

private:
    ConstMemberCommand(const Self &) = delete;
    void operator=(const Self &) = delete;
};

} // namespace crimson