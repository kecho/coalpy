#pragma once
#include <coalpy.window/Keys.h>

namespace coalpy
{

class WindowInputState
{
public:
    WindowInputState()
    {
        reset();
    }

    void reset()
    {
        m_mouseX = 0;
        m_mouseY = 0;
        for (auto& s : m_keyStates)
            s = 0;
    }

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }

    void setMouseXY(int mouseX, int mouseY)
    {
        m_mouseX = mouseX;
        m_mouseY = mouseY;
    }

    void setKeyState(Keys key, bool state)
    {
        int bucket, mask;
        keyLocation(key, bucket, mask);
        if (state)
            m_keyStates[bucket] |= mask;
        else
            m_keyStates[bucket] &= ~mask;
    }

    bool keyState(Keys key) const
    {
        int bucket, mask;
        keyLocation(key, bucket, mask);
        return (m_keyStates[bucket] & mask) != 0;
    }

private:
    enum 
    {
        KeyStateBatchCount = (((int)Keys::Count + 31) / 32)
    };
    
    void keyLocation(Keys key, int& bucket, int& mask) const
    {
        bucket = ((int)key) >> 5;
        mask = 1 << ((int)key & 31);
    }

    int m_mouseX;
    int m_mouseY;
    int m_keyStates[KeyStateBatchCount];
    
};


}
