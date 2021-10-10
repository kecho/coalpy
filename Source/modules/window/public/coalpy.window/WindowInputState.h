#pragma once

namespace coalpy
{

enum class SpecialKeys
{
    LAlt,
    RAlt,
    LShift,
    RShift,    
    Space,
    Esc,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    MouseLeft,
    MouseRight,
    MouseLeftDouble,
    MouseRightDouble,
};
               
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
        m_asciiKeyStates = 0;
        m_specialKeyStates = 0;
    }

    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }

    bool keyPressed(char key) const
    {
        if (key < 'a' || key > 'z')
            return false;

        return (m_asciiKeyStates & (1 << (int)(key - 'a'))) != 0;
    }

    void setKeyState(char key, bool state)
    {
        if (key < 'a' || key > 'z')
            return;
        
        int mask = (1 << (key - 'a'));
        if (state)
            m_asciiKeyStates |= mask;
        else
            m_asciiKeyStates &= ~mask;
    }

    bool specialKeyPressed(SpecialKeys key) const
    {
        return (m_specialKeyStates & (1 << (int)key)) != 0;
    }

    void setKeyState(SpecialKeys key, bool state)
    {
        int mask = 1 << (int)key;
        if (state)
            m_specialKeyStates |= mask;
        else
            m_specialKeyStates &= ~mask;
    }

private:
    int m_mouseX;
    int m_mouseY;
    int m_asciiKeyStates;
    int m_specialKeyStates;
    
};


}
