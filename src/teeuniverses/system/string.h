/*
 * Some parts of this file comes from other projects.
 * These parts are itendified in this file by the following block:
 * 
 * FOREIGN CODE BEGIN: ProjectName *************************************
 * 
 * FOREIGN CODE END: ProjectName ***************************************
 * 
 * If ProjectName is "TeeWorlds", then this part of the code follows the
 * TeeWorlds licence:
 *      (c) Magnus Auvinen. See licence.txt in the root of the
 *      distribution for more information. If you are missing that file,
 *      acquire a complete release at teeworlds.com.
 */

#ifndef __SHARED_SYSTEM_STRING__
#define __SHARED_SYSTEM_STRING__

#include <cstdlib>
/* BEGIN EDIT *********************************************************/
#include <base/math.h>
/* END EDIT ***********************************************************/
#include <base/system.h>

//String contained in a fixed length array
template<int SIZE>
class _fixed_string_core
{
private:
	char m_aBuffer[SIZE];
	
	 //throw a compilation error if the object is copied
	_fixed_string_core(const _fixed_string_core&);
	_fixed_string_core& operator=(const _fixed_string_core&);
	
public:
	_fixed_string_core()
	{
		m_aBuffer[0] = 0;
	}

	inline char* buffer() { return m_aBuffer; }
	inline const char* buffer() const { return m_aBuffer; }
	inline int maxsize() const { return SIZE; }
	
	inline void copy(const char* pBuffer)
	{
		str_copy(m_aBuffer, pBuffer, SIZE);
	}
	
	inline void transfert(_fixed_string_core& String)
	{
		copy((const char*) String.buffer());
	}
	
	inline void append_at(int Pos, const char* pBuffer)
	{
		str_append(m_aBuffer+Pos, pBuffer, SIZE-Pos);
	}
	
	inline void append_at_num(int Pos, const char* pBuffer, int num)
	{
		str_append_num(m_aBuffer+Pos, pBuffer, SIZE-Pos, num);
	}
};

//String contained in a rezisable array
template<int INITIALSIZE>
class _dynamic_string_core
{
private:
	char* m_pBuffer;
	int m_MaxSize;

private:	
	 //throw a compilation error if the object is copied
	_dynamic_string_core(const _dynamic_string_core&);
	_dynamic_string_core& operator=(const _dynamic_string_core&);

public:
	_dynamic_string_core() :
		m_pBuffer(NULL),
		m_MaxSize(0)
	{
		resize_buffer(INITIALSIZE);
	}
	
	~_dynamic_string_core()
	{
		if(m_pBuffer)
			delete[] m_pBuffer;
	}
	
	void resize_buffer(int Size)
	{
		if(m_pBuffer)
		{
			char* pBuffer = new char[Size];
			str_copy(pBuffer, m_pBuffer, Size);
			delete[] m_pBuffer;
			m_pBuffer = pBuffer;
			m_MaxSize = Size;
		}
		else
		{
			m_MaxSize = Size;
			m_pBuffer = new char[m_MaxSize];
			m_pBuffer[0] = 0;
		}
	}

	inline char* buffer() { return m_pBuffer; }
	inline const char* buffer() const { return m_pBuffer; }
	inline int maxsize() const { return m_MaxSize; }
	
	inline void copy(const char* pBuffer)
	{
		int Size = str_length(pBuffer)+1;
		if(Size > m_MaxSize)
			resize_buffer(Size);
		
		str_copy(m_pBuffer, pBuffer, m_MaxSize);
	}
	
	inline void transfert(_dynamic_string_core& String)
	{
		if(m_pBuffer)
			delete[] m_pBuffer;
		
		m_pBuffer = String.m_pBuffer;
		m_MaxSize = String.m_MaxSize;
		String.m_pBuffer = NULL;
		String.m_MaxSize = 0;
	}
	
	inline int append_at(int Pos, const char* pBuffer)
	{
		int BufferSize = str_length(pBuffer);
		int Size = Pos+BufferSize+1;
		if(Size > m_MaxSize)
		{
			int NewSize = m_MaxSize*2;
			while(Size > NewSize)
				NewSize *= 2;
			
			resize_buffer(NewSize);
		}
		
		str_append(m_pBuffer+Pos, pBuffer, m_MaxSize-Pos);
		
		return minimum(Pos + BufferSize, m_MaxSize-1);
	}
	
	inline int append_at_num(int Pos, const char* pBuffer, int Num)
	{
		int Size = Pos+Num+1;
		if(Size > m_MaxSize)
		{
			int NewSize = m_MaxSize*2;
			while(Size > NewSize)
				NewSize *= 2;
			
			resize_buffer(NewSize);
		}
		
		str_append_num(m_pBuffer+Pos, pBuffer, m_MaxSize-Pos, Num);
		
		return minimum(Pos + Num, m_MaxSize-1);
	}
};

template<typename BASE>
class string : public BASE
{
public:
	string() :
		BASE()
	{
		
	}
	
	string(const char* pBuffer) :
		BASE()
	{
		BASE::copy(pBuffer);
	}
	
	inline int length() const { return str_length(BASE::buffer()); }
	inline void clear() { BASE::buffer()[0] = 0; }
	inline bool empty() const { return (BASE::buffer()[0] == 0); }
	
	inline void copy(const char* pBuffer) { BASE::copy(pBuffer); }
	
	template<typename STR>
	inline void copy(const STR& str)
	{
		BASE::copy(str.buffer());
	}
	
	inline void append(const char* pBuffer) { BASE::append_at(length(), pBuffer); }
	
	template<typename STR>
	inline void append(const STR& str)
	{
		BASE::append_at(length(), str.buffer());
	}
	
	inline void append_num(const char* pBuffer, int num) { BASE::append_at_num(length(), pBuffer, num); }
	
	template<typename STR>
	inline void append_num(const STR& str, int num)
	{
		BASE::append_at_num(length(), str.buffer(), num);
	}
	
	bool operator<(const char* buffer) const
	{
		return (str_comp(BASE::buffer(), buffer) < 0);
	}
	
	template<typename STR>
	bool operator<(const STR& str) const
	{
		return (str_comp(BASE::buffer(), str.buffer()) < 0);
	}
	
	bool operator>(const char* buffer) const
	{
		return (str_comp(BASE::buffer(), buffer) > 0);
	}

	template<typename STR>
	bool operator>(const STR& str) const
	{
		return (str_comp(BASE::buffer(), str.buffer()) > 0);
	}
	
	bool operator==(const char* buffer) const
	{
		return (str_comp(BASE::buffer(), buffer) == 0);
	}

	template<typename STR>
	bool operator==(const STR& str) const
	{
		return (str_comp(BASE::buffer(), str.buffer()) == 0);
	}
	
	bool operator!=(const char* buffer) const
	{
		return (str_comp(BASE::buffer(), buffer) != 0);
	}

	template<typename STR>
	bool operator!=(const STR& str) const
	{
		return (str_comp(BASE::buffer(), str.buffer()) != 0);
	}

	int comp_num(const char* str, int num) const
	{
		return (str_comp_num(BASE::buffer(), str, num) != 0);
	}

	template<typename STR>
	int comp_num(const STR& str, int num) const
	{
		return (str_comp_num(BASE::buffer(), str.buffer(), num) != 0);
	}
};

typedef string<_fixed_string_core<12> > fixed_string12;
typedef string<_fixed_string_core<16> > fixed_string16;
typedef string<_fixed_string_core<64> > fixed_string64;
typedef string<_fixed_string_core<128> > fixed_string128;
typedef string<_fixed_string_core<256> > fixed_string256;

typedef string<_dynamic_string_core<128> > dynamic_string;

//Operations on strings

#endif
