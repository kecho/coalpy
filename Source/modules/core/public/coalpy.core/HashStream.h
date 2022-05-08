#pragma once

namespace coalpy
{

class HashStream
{
public:
    HashStream() : m_val(0) {}

    template<typename T>
    HashStream& operator << (T& o)
    {
        m_val = fnvHash((const char*)&o, (unsigned)sizeof(T), m_val);
        return *this;
    }

    int val() const { return m_val; }

private:
    static int fnvHash(const char* b, unsigned bytes, unsigned prevHash)
    {
        const unsigned int fnvPrime = 0x811C9DC5;
	    unsigned int hash = prevHash;
	    unsigned int i = 0;
        while (i < bytes)
	    {
	    	hash *= fnvPrime;
	    	hash ^= *b++;
            ++i;
	    }

	    return hash;
    }

    unsigned int m_val;
};

}
