#pragma once

#include <coalpy.core/Assert.h>

//Original implementation by Karolyn Boulanger, October 2013.

namespace coalpy
{

//! Pointer to a reference counted object of class C
//! (intrusive approach, the pointed object needs to have a predefined set of functions).
//! \warning The pointed class must have a reference counter initialized to 0
//!          and provide the following private functions (with a friend access):
//!          void AddRef();             -> Adds 1 to the reference counter
//!          void Release();            -> Decreases the reference counter, and if it reaches 0, self-destroy
template <class C>
class SmartPtr
{
public:
    //! Default constructor, equivalent to nullptr
    SmartPtr() : mObject(nullptr)
    {
    }

    //! Constructor using a weak pointer
    //! \param ptr Weak pointer to the object for which the reference becomes the owner
    //! \note Increases the reference counter of the pointed object if defined
    //! \warning The input pointer is considered as a weak pointer,
    //!          meaning that the object's reference counter should be 0
    SmartPtr(C * ptr) : mObject(nullptr)
    {
        *this = ptr;
    }

    //! Copy constructor
    //! \param ref SmartPtrerence to copy
    //! \note Increases the reference counter of the pointed object if defined
    SmartPtr(const SmartPtr<C> & ref) : mObject(nullptr)
    {
        *this = ref;
    }

    //! Destructor
    //! \note Decreases the reference counter of the currently pointed object if defined
    ~SmartPtr()
    {
        if (mObject != nullptr)
        {
            mObject->Release();
        }
    }

    //! Assignment operator with a weak pointer
    //! \param ptr Pointer to copy
    //! \note Decreases the reference counter of the currently pointed object if defined
    //!       and increases the reference counter of the new pointed object if defined
    //! \warning The input pointer is considered as a weak pointer,
    //!          meaning that the object's reference counter should be 0
    SmartPtr<C> & operator=(C * ptr)
    {
        if (mObject != ptr)
        {
            if (mObject != nullptr)
            {
                mObject->Release();
            }
            if (ptr != nullptr)
            {
                // In most cases, this assertion test makes sense,
                // but in the cast of a cast (see operator below), the counter can be > 0
                //CPY_ASSERT_MSG(ptr->GetSmartPtrCount() == 0, "Invalid object given to the reference, the object has a reference count of %d, but it should 0", ptr->GetSmartPtrCount());

                ptr->AddRef();
            }
            mObject = ptr;
        }
        return *this;
    }

    //! Assignment operator with another reference
    //! \param ref SmartPtrerence to copy
    //! \note Decreases the reference counter of the currently pointed object if defined
    //!       and increases the reference counter of the new pointed object if defined
    SmartPtr<C> & operator=(const SmartPtr<C> & ref)
    {
        if (mObject != ref.mObject)
        {
            if (mObject != nullptr)
            {
                mObject->Release();
            }
            if (ref.mObject != nullptr)
            {
                ref.mObject->AddRef();
            }
            mObject = ref.mObject;
        }
        return *this;
    }


    //! Dereference operator
    //! \return SmartPtrerence to the pointed object
    C & operator*()
    {
        CPY_ASSERT_MSG(mObject != nullptr, "Trying to dereference a null SmartPtr<>");
        return *mObject;
    }

    //! Dereference operator (const version)
    //! \return SmartPtrerence to the pointed object
    const C & operator*() const
    {
        CPY_ASSERT_MSG(mObject != nullptr, "Trying to dereference a null SmartPtr<>");
        return *mObject;
    }

    //! Pointer member access operator
    //! \return Pointed to the pointed object
    C * operator->()
    {
        CPY_ASSERT_MSG(mObject != nullptr, "Trying to access a member of a null SmartPtr<>");
        return mObject;
    }

    //! Pointer member access operator (const version)
    //! \return Pointed to the pointed object
    const C * operator->() const
    {
        CPY_ASSERT_MSG(mObject != nullptr, "Trying to access a member of a null SmartPtr<>");
        return mObject;
    } 

  
    //! SmartPtrerence cast operator, with C2 as the destination class
    //! \warning C2 must be a class derived from C
    //! \return SmartPtrerence to the object cast to a different type
    template <class C2>
    inline operator SmartPtr<C2>() const { return SmartPtr<C2>(static_cast<C2 *>(mObject)); }

    //! Weak pointer cast operator, with C2 as the destination class
    //! \warning C2 must be a class derived from C
    //! \warning The returned pointer is weak, meaning that no reference counting is handled for it
    //! \return Pointer to the object cast to a different pointer type
    template <class C2>
    inline operator C2 *() { return static_cast<C2 *>(mObject); }

    //! Constant weak pointer cast operator, with C2 as the destination class
    //! \warning C2 must be a class derived from C
    //! \warning The returned pointer is weak, meaning that no reference counting is handled for it
    //! \return Pointer to the object cast to a different pointer type
    template <class C2>
    inline operator const C2 *() const { return static_cast<const C2 *>(mObject); }

    //! Comparison operator with a weak pointer
    //! \warning Compares the pointers, not the objects themselves
    //! \param ptr Pointer to compare with
    //! \return True if the pointers are equal (and not the pointed objects)
    //! \note Comparing a null reference to a null input pointer is considered valid and returns true
    bool operator==(C * ptr) const { return (mObject == ptr); }

    //! Comparison operator with another reference
    //! \warning Compares the pointers, not the objects themselves
    //! \param ref SmartPtrerence to compare with
    //! \return True if the pointers are equal (and not the pointed objects)
    //! \note Comparing a null reference to another null reference is considered valid and returns true
    bool operator==(const SmartPtr<C> & ref) const { return (mObject == ref.mObject); }

    //! Difference operator with a weak pointer
    //! \warning Compares the pointers, not the objects themselves
    //! \param ptr Pointer to compare with
    //! \return True if the pointers are different (and not the pointed objects)
    //! \note Comparing a null reference to a null input pointer is considered valid and returns false
    bool operator!=(C * ptr) const { return (mObject != ptr); }

    //! Difference operator with another reference
    //! \warning Compares the pointers, not the objects themselves
    //! \param ref SmartPtrerence to compare with
    //! \return True if the pointers are different (and not the pointed objects)
    //! \note Comparing a null reference to another null reference is considered valid and returns false
    bool operator!=(const SmartPtr<C> & ref) const { return (mObject != ref.mObject); }

    //! Inversion operator, typically used when testing if a reference is empty (!ref)
    //! \return True if the reference is empty
    bool operator!() const { return (mObject == nullptr); }


private:
    //! Pointer to the reference counted object, nullptr when undefined
    C * mObject;
};


}
